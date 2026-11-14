#!/usr/bin/env python3
"""Matching-host ordinary ConPTY smoke. No native profile/custody authority."""
import argparse
import ctypes
import importlib.metadata
import json
import os
import pathlib
import platform
import select
import sys
import time

try:
    from winpty.enums import Backend
    from winpty.ptyprocess import PtyProcess
except ImportError:
    Backend = PtyProcess = None


def exact_helper():
    return (os.name == 'nt' and sys.version_info[:2] == (3, 12)
            and ctypes.sizeof(ctypes.c_void_p) == 8
            and platform.machine().lower().replace('_', '') in ('amd64', 'x8664')
            and importlib.metadata.version('pywinpty') == '3.0.5')


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


def probe(binary, connection, output, mode, observations, factory=None,
          select_fn=select.select, clock=time.monotonic, sleep=time.sleep):
    output.mkdir(mode=0o700, parents=False, exist_ok=False)
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
    process.fileobj.settimeout(.1)
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
            help_line = 'd dashboard • v services • i inbox • p connections • / search • n narrow • r reconnect • esc back • q quit'.encode()
            wait_for(lambda: help_line in data[before:], 3)
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
        for owned_socket in [process.fileobj, process._server]:
            try:
                owned_socket.close()
            except Exception:
                sockets_closed = False
        process._thread.join(timeout=2)
        reader_joined = not process._thread.is_alive()
        if not sockets_closed or not reader_joined:
            failure = failure or 'TransportCleanupError'
        raw = output / 'terminal.private.ansi'
        raw.write_bytes(data)  # Inherits the verified private evidence-root DACL.
    leaked = connection['token'].encode() in data
    result = {'schemaVersion': 1, 'stage': 'terminal', 'mode': mode, 'transport': 'ConPTY',
              'status': 'failed' if failure or overflow or leaked or not reaped or status != 0 or forced else ('blocked' if mode == 'v2' else 'passed'),
              'reason': failure or ('operator-v2-functional-journeys-not-qualified' if mode == 'v2' else None),
              'apiUnavailable': b'404 Not Found' in data, 'readyBeforeKeys': ready,
              'helpStateObserved': help_state, 'dashboardRestored': dashboard_returned,
              'actualOwnedWait': reaped, 'socketCleanup': sockets_closed, 'readerJoined': reader_joined, 'exitCode': status, 'forcedCleanup': forced,
              'interruptedForCleanup': interrupted, 'tokenLeaked': leaked,
              'captureOverflow': overflow, 'events': events,
              'qToReapSeconds': reaped_at - q_at if reaped and q_at is not None else None}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
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
