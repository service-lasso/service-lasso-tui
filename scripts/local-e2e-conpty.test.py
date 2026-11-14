"""Controlled transport tests only, never matching-host ConPTY qualification."""
import importlib.util
import json
import pathlib
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('ordinary_conpty', pathlib.Path(__file__).with_name('local-e2e-conpty.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Clock:
    value = 0.0
    def now(self):
        return self.value
    def sleep(self, seconds):
        self.value += seconds


class Socket:
    def __init__(self, terminal):
        self.terminal = terminal
        self.closed = False
    def settimeout(self, seconds):
        self.timeout = seconds
    def recv(self, size):
        if self.terminal.chunks:
            return self.terminal.chunks.pop(0)
        return b''
    def close(self):
        self.closed = True


class Reader:
    def __init__(self, stuck=False):
        self.stuck = stuck
        self.joined = False
    def join(self, timeout):
        self.joined = True
    def is_alive(self):
        return self.stuck


class Terminal:
    def __init__(self, clock, observations, fail_quit=False, leak=False, stuck_reader=False):
        self.clock, self.observations = clock, observations
        self.fail_quit = fail_quit
        self.fileobj = Socket(self)
        self._server = Socket(self)
        self._thread = Reader(stuck_reader)
        self.chunks = [b'Runtime: ok (API up)\nServices\n' + (b'private-token-only' if leak else b'')]
        self.exit_at = None
        self.exitstatus = 0
        self.signalstatus = None
        self.wait_count = 0
        self.keys = []
        self.sizes = []
        self.forced = False
    def isalive(self):
        return self.exit_at is None or self.clock.now() < self.exit_at
    def wait(self):
        if self.isalive():
            raise AssertionError('wait was called before actual non-live observation')
        self.wait_count += 1
        return self.exitstatus
    def write(self, value):
        self.keys.append((value, self.clock.now()))
        if value == '?':
            self.chunks.append('d dashboard • v services • i inbox • p connections • / search • n narrow • r reconnect • esc back • q quit'.encode())
        elif value == '\x1b':
            self.chunks.append(b'Services\nRuntime: ok (API up)')
        elif value == 'r':
            rows = json.loads(self.observations.read_text())
            rows += [dict(row) for row in rows]
            self.observations.write_text(json.dumps(rows))
        elif value == 'q' and not self.fail_quit:
            self.exit_at = self.clock.now() + .25
    def setwinsize(self, rows, columns):
        self.sizes.append((rows, columns))
    def close(self, force=False):
        if force:
            self.forced = True
            self.exitstatus = 9
            self.exit_at = self.clock.now()


class Tests(unittest.TestCase):
    def run_probe(self, **options):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            observations = root / 'observations.json'
            observations.write_text(json.dumps([{'method': 'GET', 'pathname': route, 'statusCode': 200} for route in ['/api/health', '/api/services']]))
            clock = Clock()
            terminal = Terminal(clock, observations, **options)
            environment = {}
            def factory(argv, env):
                environment.update(env)
                return terminal
            def selected(readers, writers, errors, seconds):
                clock.sleep(seconds)
                return readers, [], []
            result = module.probe(root / 'tui.exe', {'url': 'http://127.0.0.1:12345', 'token': 'private-token-only'}, root / 'proof', 'compat', observations,
                                  factory=factory, select_fn=selected, clock=clock.now, sleep=clock.sleep)
            self.assertTrue(terminal.fileobj.closed)
            self.assertTrue(terminal._server.closed)
            self.assertNotIn('private-token-only', json.dumps(result))
            self.assertTrue(environment['APPDATA'].startswith(str(root / 'proof')))
            return result, terminal

    def test_actual_wait_follows_eof_and_readiness_precedes_first_key(self):
        result, terminal = self.run_probe()
        self.assertEqual(result['status'], 'passed')
        self.assertEqual(result['exitCode'], 0)
        self.assertEqual(terminal.wait_count, 1)
        self.assertTrue(result['actualOwnedWait'])
        self.assertTrue(result['helpStateObserved'])
        self.assertTrue(result['dashboardRestored'])
        self.assertGreaterEqual(result['qToReapSeconds'], .25)
        self.assertFalse(result['forcedCleanup'])
        ready = next(row['readyAtSeconds'] for row in result['events'] if 'readyAtSeconds' in row)
        self.assertGreaterEqual(terminal.keys[0][1], ready)
        self.assertEqual(terminal.sizes, [(24, 80)])
        self.assertTrue(result['readerJoined'])

    def test_nonresponsive_quit_is_failed_even_after_forced_exit(self):
        result, terminal = self.run_probe(fail_quit=True)
        self.assertEqual(result['status'], 'failed')
        self.assertTrue(result['forcedCleanup'])
        self.assertTrue(terminal.forced)
        self.assertTrue(result['actualOwnedWait'])

    def test_private_output_leak_is_boolean_failure_not_public_text(self):
        result, _ = self.run_probe(leak=True)
        self.assertEqual(result['status'], 'failed')
        self.assertTrue(result['tokenLeaked'])

    def test_unjoined_reader_fails_transport_cleanup(self):
        result, _ = self.run_probe(stuck_reader=True)
        self.assertEqual(result['status'], 'failed')
        self.assertFalse(result['readerJoined'])

    def test_exit_status_requires_actual_consistent_integer_wait(self):
        class Exited:
            exitstatus = 0
            signalstatus = None
            def isalive(self): return False
            def wait(self): return True
        with self.assertRaises(RuntimeError):
            module.observed_exit(Exited())
        Exited.wait = lambda self: 0
        self.assertEqual(module.observed_exit(Exited()), 0)
        Exited.signalstatus = 9
        with self.assertRaises(RuntimeError):
            module.observed_exit(Exited())


if __name__ == '__main__':
    unittest.main()
