#!/usr/bin/env python3
"""Ordinary POSIX terminal proof; neither native custody nor release qualification."""
import argparse, errno, fcntl, json, os, pathlib, pty, select, signal, struct, termios, time


def probe(binary, connection, output, mode):
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    config = output / 'config'
    config.mkdir(mode=0o700)
    argv = [str(binary), '--api', connection['url']]
    if mode == 'compat':
        argv.append('--compat-v1')
    started = time.monotonic()
    pid, master = pty.fork()
    if pid == 0:
        os.execve(argv[0], argv, {'PATH': '/usr/bin:/bin', 'TERM': 'xterm-256color',
            'LANG': 'C.UTF-8', 'XDG_CONFIG_HOME': str(config),
            'SERVICE_LASSO_API_TOKEN': connection['token']})
    data, events = bytearray(), []
    stream_closed, reaped, status, reaped_at = False, False, None, None

    def collect(deadline):
        nonlocal stream_closed, reaped, status, reaped_at
        while time.monotonic() < deadline and not reaped:
            observed, observed_status = os.waitpid(pid, os.WNOHANG)
            if observed:
                reaped, status, reaped_at = True, observed_status, time.monotonic()
                return
            remaining = max(0, deadline - time.monotonic())
            if stream_closed:
                time.sleep(min(.01, remaining))
                continue
            readable, _, _ = select.select([master], [], [], min(.05, remaining))
            if not readable:
                continue
            try:
                chunk = os.read(master, 65536)
            except OSError as original:
                if original.errno != errno.EIO:
                    raise
                chunk = b''
            if not chunk:
                stream_closed = True
                events.append({'streamClosedAtSeconds': time.monotonic() - started})
            else:
                data.extend(chunk)

    def wait_for(predicate, seconds):
        deadline = time.monotonic() + seconds
        while not reaped and not predicate() and time.monotonic() < deadline:
            collect(min(deadline, time.monotonic() + .05))
        if not predicate():
            raise RuntimeError('terminal expectation not observed')

    def key(value):
        os.write(master, value)
        events.append({'key': value.decode('ascii'), 'atSeconds': time.monotonic() - started})

    def resize(width, height):
        fcntl.ioctl(master, termios.TIOCSWINSZ, struct.pack('HHHH', height, width, 0, 0))
        os.kill(pid, signal.SIGWINCH)
        events.append({'resize': [width, height], 'atSeconds': time.monotonic() - started})

    failure = None
    ready, help_state, dashboard_returned = False, None, None
    q_at = None
    try:
        resize(120, 32)
        marker = b'Runtime: ok (API up)' if mode == 'compat' else b'Service Lasso  operator/v2'
        wait_for(lambda: marker in data, 20)
        if mode == 'v2':
            wait_for(lambda: b'Error:' in data or b'Connecting to authenticated operator API...' not in data, 10)
        ready = True
        canonical = bool(termios.tcgetattr(master)[3] & termios.ICANON)
        events.append({'readyAtSeconds': time.monotonic() - started, 'ICANON': canonical})
        if canonical:
            raise RuntimeError('terminal remains canonical after rendering')
        if mode == 'compat':
            before = len(data)
            key(b'?')
            help_line = 'd dashboard • v services • i inbox • p connections • / search • n narrow • r reconnect • esc back • q quit'.encode()
            wait_for(lambda: help_line in data[before:], 3)
            help_state = True
            before = len(data)
            key(b'\x1b')
            wait_for(lambda: b'Services' in data[before:], 3)
            dashboard_returned = True
            for value in [b'n', b'n', b'j', b'k']:
                key(value)
                collect(time.monotonic() + .15)
            before = len(data)
            key(b'r')
            wait_for(lambda: b'Runtime: ok (API up)' in data[before:], 8)
        resize(80, 24)
        collect(time.monotonic() + .2)
        q_at = time.monotonic()
        key(b'q')
        collect(q_at + 8)
        if not reaped:
            raise RuntimeError('quit deadline expired')
    except BaseException as original:
        failure = type(original).__name__
    finally:
        interrupted = False
        if not reaped:
            interrupted = True
            os.kill(pid, signal.SIGINT)
            collect(time.monotonic() + 8)
        if not reaped:
            # Retain this owner instead of calling an unobserved child closed.
            print(json.dumps({'stage': 'terminal', 'status': 'cleanup-unresolved'}), flush=True)
            while not reaped:
                collect(time.monotonic() + 1)
        os.close(master)
        raw = output / 'terminal.private.ansi'
        raw.write_bytes(data)
        raw.chmod(0o600)
    leaked = connection['token'].encode() in data
    unavailable = b'404 Not Found' in data
    result = {'schemaVersion': 1, 'stage': 'terminal', 'mode': mode,
        'status': 'failed' if failure or leaked or os.waitstatus_to_exitcode(status) != 0 else ('blocked' if mode == 'v2' else 'passed'),
        'reason': failure or ('operator-v2-functional-journeys-not-qualified' if mode == 'v2' else None),
        'apiUnavailable': unavailable, 'readyBeforeKeys': ready,
        'helpStateObserved': help_state, 'dashboardRestored': dashboard_returned,
        'exitCode': os.waitstatus_to_exitcode(status), 'interruptedForCleanup': interrupted,
        'actualWaitpid': True, 'tokenLeaked': leaked, 'events': events,
        'qToReapSeconds': reaped_at - q_at if q_at is not None else None}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', required=True, type=pathlib.Path)
    parser.add_argument('--connection-file', required=True, type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    parser.add_argument('--mode', choices=['compat', 'v2'], required=True)
    args = parser.parse_args()
    connection = json.loads(args.connection_file.read_text())
    result = probe(args.binary.resolve(), connection, args.output.resolve(), args.mode)
    print(json.dumps(result))
    return 0 if result['status'] == 'passed' else 2


if __name__ == '__main__':
    raise SystemExit(main())
