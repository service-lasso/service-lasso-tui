#!/usr/bin/env python3
"""Matching-host ordinary ConPTY smoke. No native profile/custody authority."""
import argparse
import ctypes
import importlib.metadata
import hashlib
import inspect
import json
import os
import pathlib
import platform
import select
import re
import sys
import stat
import time

try:
    from winpty.enums import Backend
    from winpty.ptyprocess import PtyProcess
except ImportError:
    Backend = PtyProcess = None


def terminal_text(body):
    # ConPTY may insert cursor controls and physical wraps inside a logical line.
    value = re.sub(rb'\x1b\[[0-?]*[ -/]*[@-~]', b'', bytes(body))
    value = re.sub(rb'\x1b\][^\x07]*?(?:\x07|\x1b\\)', b'', value)
    return value.replace(b'\r', b'').replace(b'\n', b'')


def exact_helper():
    return (os.name == 'nt' and sys.version_info[:2] == (3, 12)
            and ctypes.sizeof(ctypes.c_void_p) == 8
            and platform.machine().lower().replace('_', '') in ('amd64', 'x8664')
            and importlib.metadata.version('pywinpty') == '3.0.5'
            and hashlib.sha256(pathlib.Path(inspect.getfile(PtyProcess)).read_bytes()).hexdigest()
            == '52a9ab4b0e8de72d775a96587982530f9e70e009ab89d11faf703093eb1c8988')


def observed_exit(process):
    """Only call actual wait after the original owned terminal is non-live."""
    if process.isalive():
        return None
    waited = process.wait()
    code = process.exitstatus
    signaled = getattr(process, 'signalstatus', None)
    if (type(waited) is not int or type(code) is not int or waited != code
            or signaled not in (None, 0)):
        raise RuntimeError('actual terminal exit status unavailable')
    return code


def prepared_file(handle, file):
    actual, named = os.fstat(handle.fileno()), file.lstat()
    if (file.is_symlink() or not stat.S_ISREG(actual.st_mode) or actual.st_size != 0
            or getattr(named, 'st_file_attributes', 0) & 0x400
            or (actual.st_dev, actual.st_ino) != (named.st_dev, named.st_ino)):
        raise RuntimeError('parent-prepared empty private file required')


def write_prepared(handle, file, body):
    before = os.fstat(handle.fileno())
    handle.seek(0)
    handle.write(body)
    handle.truncate()
    handle.flush()
    os.fsync(handle.fileno())
    after = file.lstat()
    if file.is_symlink() or (before.st_dev, before.st_ino) != (after.st_dev, after.st_ino):
        raise RuntimeError('prepared output identity changed')


def probe(binary, connection, output, mode, observations, factory=None,
          select_fn=select.select, clock=time.monotonic, sleep=time.sleep):
    # Parent creates the protected stage directory and empty files through the
    # shared Core private writer before this token-bearing process is launched.
    if not output.is_dir() or output.is_symlink():
        raise RuntimeError('parent-prepared private output required')
    raw_file, result_file = output / 'terminal.private.ansi', output / 'result.json'
    with raw_file.open('r+b') as raw_handle, result_file.open('r+b') as result_handle:
        prepared_file(raw_handle, raw_file)
        prepared_file(result_handle, result_file)
        return run_probe(binary, connection, output, mode, observations,
                         raw_handle, result_handle, factory, select_fn, clock, sleep)


def run_probe(binary, connection, output, mode, observations, raw_handle,
              result_handle, factory, select_fn, clock, sleep):
    config = output / 'config'
    config.mkdir()
    for name in ['roaming', 'local', 'tmp', 'home']:
        (config / name).mkdir()
    env = {name: os.environ[name] for name in ('PATH', 'SYSTEMROOT', 'WINDIR', 'COMSPEC', 'PATHEXT') if name in os.environ}
    env.update({'APPDATA': str(config / 'roaming'), 'LOCALAPPDATA': str(config / 'local'),
                'USERPROFILE': str(config / 'home'), 'HOME': str(config / 'home'),
                'TEMP': str(config / 'tmp'), 'TMP': str(config / 'tmp'),
                'SERVICE_LASSO_API_TOKEN': connection['token']})
    argv = [str(binary), '--api', connection['url']]
    if mode == 'compat':
        argv.append('--compat-v1')
    started = clock()
    if factory is None:
        process = PtyProcess.spawn(argv, cwd=str(binary.parent), env=env,
                                   dimensions=(32, 120), backend=Backend.ConPTY)
    else:
        process = factory(argv, env)
    # pywinpty3.0.5 exposes its actual reader socket as fileobj. Read bytes
    # directly to avoid PtyProcess.read's unbounded UTF-8 continuation recv.
    data = bytearray()
    events = []
    reaped, status, reaped_at, overflow = False, None, None, False

    def collect(deadline):
        nonlocal reaped, status, reaped_at, overflow
        while clock() < deadline and not reaped:
            if not process.isalive():
                status = observed_exit(process)
                reaped, reaped_at = True, clock()
                return
            readable, _, _ = select_fn([process], [], [], min(.05, max(0, deadline - clock())))
            if not readable:
                continue
            try:
                chunk = process.fileobj.recv(65536)
                if not chunk:
                    raise EOFError()
            except EOFError:
                # EOF can precede owned process exit; retain the original deadline.
                events.append({'streamEOFAtSeconds': clock() - started})
                sleep(min(.01, max(0, deadline - clock())))
                continue
            if len(data) + len(chunk) > 8 * 1024 * 1024:
                overflow = True
            else:
                data.extend(chunk)

    def wait_for(predicate, seconds):
        deadline = clock() + seconds
        while not reaped and not predicate() and clock() < deadline:
            collect(min(deadline, clock() + .05))
        if not predicate():
            raise RuntimeError('terminal expectation not observed')

    def key(value):
        process.write(value)
        events.append({'key': value, 'atSeconds': clock() - started})

    def counts():
        try:
            rows = json.loads(observations.read_text())
        except (OSError, ValueError):
            return None
        return tuple(sum(1 for row in rows if row.get('method') == 'GET'
                         and row.get('pathname') == route and row.get('statusCode') == 200)
                     for route in ['/api/health', '/api/services'])

    failure, ready, help_state, dashboard_returned = None, False, None, None
    q_at = None
    forced, interrupted = False, False
    try:
        process.fileobj.settimeout(.1)
        marker = b'Runtime: ok (API up)' if mode == 'compat' else b'Service Lasso  operator/v2'
        wait_for(lambda: marker in data, 20)
        if mode == 'v2':
            wait_for(lambda: b'Error:' in data, 10)
        ready = True
        events.append({'readyAtSeconds': clock() - started, 'inputMode': 'ConPTY',
                       'posixICANONApplicable': False})
        if mode == 'compat':
            before = len(data)
            key('?')
            wait_for(lambda: all(phrase in terminal_text(data[before:]) for phrase in [b'n narrow', b'esc back']), 3)
            help_state = True
            before = len(data)
            key('\x1b')
            wait_for(lambda: b'Services' in data[before:], 3)
            dashboard_returned = True
            for value in ['n', 'n', 'j', 'k']:
                key(value)
                collect(clock() + .15)
            wait_for(lambda: counts() is not None and all(value > 0 for value in counts()), 3)
            prior = counts()
            if prior is None:
                raise RuntimeError('request snapshot changed')
            key('r')
            wait_for(lambda: counts() is not None and all(after > before for after, before in zip(counts(), prior)), 8)
            events.append({'reconnectReadCountsBefore': prior, 'reconnectReadCountsAfter': counts()})
        process.setwinsize(24, 80)
        events.append({'resize': [80, 24], 'atSeconds': clock() - started})
        collect(clock() + .2)
        q_at = clock()
        key('q')
        collect(q_at + 8)
        if not reaped:
            raise RuntimeError('quit deadline expired')
    except BaseException as original:
        failure = type(original).__name__
    finally:
        if not reaped:
            interrupted = True
            try:
                process.write('\x03')
                collect(clock() + 5)
            except Exception:
                pass
        if not reaped:
            forced = True
            try:
                process.close(force=True)
                collect(clock() + 5)
            except Exception:
                pass
        if reaped:
            try:
                process.close(force=False)
            except Exception:
                failure = failure or 'TerminalCloseError'
        sockets_closed = True
        for socket_name in ['fileobj', '_server']:
            try:
                getattr(process, socket_name).close()
            except Exception:
                sockets_closed = False
        try:
            process._thread.join(timeout=2)
            reader_joined = not process._thread.is_alive()
        except Exception:
            reader_joined = False
        if not sockets_closed or not reader_joined:
            failure = failure or 'TransportCleanupError'
        raw = output / 'terminal.private.ansi'
        write_prepared(raw_handle, raw, data)
    leaked = connection['token'].encode() in data or connection['token'].encode() in terminal_text(data)
    result = {'schemaVersion': 1, 'stage': 'terminal', 'mode': mode, 'transport': 'ConPTY',
              'status': 'failed' if failure or overflow or leaked or not reaped or status != 0 or forced else ('blocked' if mode == 'v2' else 'passed'),
              'reason': failure or ('operator-v2-functional-journeys-not-qualified' if mode == 'v2' else None),
              'apiUnavailable': b'404 Not Found' in data, 'readyBeforeKeys': ready,
              'helpStateObserved': help_state, 'dashboardRestored': dashboard_returned,
              'actualOwnedWait': reaped, 'socketCleanup': sockets_closed, 'readerJoined': reader_joined, 'exitCode': status, 'forcedCleanup': forced,
              'interruptedForCleanup': interrupted, 'tokenLeaked': leaked,
              'captureOverflow': overflow, 'events': events,
              'helper': None if factory is not None else {'pythonVersion': platform.python_version(),
                  'pointerBits': ctypes.sizeof(ctypes.c_void_p) * 8, 'pywinptyVersion': importlib.metadata.version('pywinpty'),
                  'ptyprocessSourceSHA256': hashlib.sha256(pathlib.Path(inspect.getfile(PtyProcess)).read_bytes()).hexdigest()},
              'qToReapSeconds': reaped_at - q_at if reaped and q_at is not None else None}
    write_prepared(result_handle, output / 'result.json', (json.dumps(result, indent=2) + '\n').encode())
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', type=pathlib.Path, required=True)
    parser.add_argument('--connection-file', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--mode', choices=['compat', 'v2'], required=True)
    parser.add_argument('--observations', type=pathlib.Path, required=True)
    args = parser.parse_args()
    if not exact_helper():
        raise RuntimeError('matching pinned ConPTY helper required')
    connection = json.loads(args.connection_file.read_text())
    result = probe(args.binary.resolve(), connection, args.output.resolve(), args.mode, args.observations)
    print(json.dumps(result))
    return 0 if result['status'] == 'passed' else 2


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except Exception:
        print(json.dumps({'stage': 'terminal', 'status': 'failed', 'reason': 'ConPTY-helper-failed'}))
        raise SystemExit(2)
