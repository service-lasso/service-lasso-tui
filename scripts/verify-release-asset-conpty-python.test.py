import importlib.util
import contextlib
import io
import json
import os
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = os.path.join(os.path.dirname(__file__), "verify-release-asset-conpty.py")
SPEC = importlib.util.spec_from_file_location("release_asset_probe", SCRIPT)
probe_module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(probe_module)


class FakePty:
    spawned = None

    @staticmethod
    def spawn(*args, **kwargs):
        FakePty.spawned = (args, kwargs)
        return FakePty()

    def close(self, force=True):
        self.closed = force

    def isalive(self):
        return False

    def write(self, _value):
        pass


class EarlyClosingPty(FakePty):
    @staticmethod
    def spawn(*_args, **_kwargs):
        return EarlyClosingPty()

    def read(self):
        raise EOFError()


class ExitedPty(EarlyClosingPty):
    def __init__(self, exitstatus=None, signalstatus=None):
        self.exitstatus = exitstatus
        self.signalstatus = signalstatus


class LiveEofThenOutputPty(FakePty):
    def __init__(self):
        self.reads = 0

    def isalive(self):
        return True

    def read(self):
        self.reads += 1
        if self.reads == 1:
            raise EOFError()
        return "ready"


class ReceiptTests(unittest.TestCase):
    def test_reconnect_timeout_reports_a_closed_receipt_over_the_controlled_stdout_channel(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = probe_module.probe(
                "candidate.exe", "reconnect", "http://127.0.0.1:1", None, "reconnect", None, None, None,
                FakePty, wait=lambda *_args: None,
                wait_file=lambda *_args: self.fail("reconnect wait must not run after startup timeout"), backend=None,
            )
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "startup", "receipt": {"stage": "startup", "outcome": "timeout", "closedReason": "timed_out"}})

    def test_failures_have_only_the_closed_metadata_schema(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = probe_module.probe("candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None, None)
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "launch", "receipt": {"stage": "launch", "outcome": "error", "closedReason": "stage_failed"}})

    def test_helper_has_no_receipt_path_writer(self):
        self.assertFalse(hasattr(probe_module, "write_receipt"))

    def test_live_eof_retains_the_existing_bounded_wait_without_spinning(self):
        process = LiveEofThenOutputPty()
        delays = []
        observed = probe_module.wait_for(
            process, ("ready",), 1,
            select_fn=lambda *_args: ([process], [], []),
            sleeper=delays.append,
        )
        self.assertEqual(observed, "ready")
        self.assertEqual(process.reads, 2)
        self.assertEqual(len(delays), 1)
        self.assertGreater(delays[0], 0)
        self.assertLessEqual(delays[0], 0.05)

    def test_eof_classifies_only_owned_process_lifecycle_metadata(self):
        cases = (
            (ExitedPty(exitstatus=0), "terminal_exited_zero"),
            (ExitedPty(exitstatus=4), "terminal_exited_nonzero"),
            (ExitedPty(exitstatus=0, signalstatus=9), "terminal_signaled"),
            (ExitedPty(exitstatus="SENTINEL_SECRET"), "terminal_unknown"),
        )
        for process, expected_reason in cases:
            with self.subTest(expected_reason=expected_reason):
                with self.assertRaises(probe_module.TerminalClosed) as closed:
                    probe_module.wait_for(process, ("ready",), 1, select_fn=lambda *_args: ([process], [], []))
                self.assertEqual(closed.exception.reason, expected_reason)

    def test_liveness_probe_failure_is_closed_as_unknown(self):
        class BrokenLivenessPty(EarlyClosingPty):
            def isalive(self):
                raise RuntimeError("SENTINEL_SECRET")

        with self.assertRaises(probe_module.TerminalClosed) as closed:
            probe_module.wait_for(BrokenLivenessPty(), ("ready",), 1, select_fn=lambda *_args: ([object()], [], []))
        self.assertEqual(closed.exception.reason, "terminal_unknown")
        self.assertNotIn("SENTINEL_SECRET", str(closed.exception))

    def test_exited_terminal_eof_has_a_closed_lifecycle_receipt(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = probe_module.probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=EarlyClosingPty,
                wait=lambda *_args: (_ for _ in ()).throw(probe_module.TerminalClosed("terminal_exited_zero")),
                backend=None,
            )
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "startup", "receipt": {"stage": "startup", "outcome": "error", "closedReason": "terminal_exited_zero"}})

    def test_launch_resolves_a_relative_candidate_before_passing_it_to_pywinpty(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = probe_module.probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=FakePty,
                wait=lambda *_args: "Service Lasso TUI q quit Runtime API unavailable",
                backend=None,
            )
        self.assertEqual(result, 0)
        self.assertEqual(FakePty.spawned[0][0][0], os.path.abspath("candidate.exe"))


if __name__ == "__main__":
    unittest.main()
