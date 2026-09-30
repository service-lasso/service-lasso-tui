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

    def test_early_terminal_eof_has_a_distinct_closed_receipt(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = probe_module.probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=EarlyClosingPty,
                wait=lambda *_args: (_ for _ in ()).throw(probe_module.TerminalClosed()),
                backend=None,
            )
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "startup", "receipt": {"stage": "startup", "outcome": "error", "closedReason": "terminal_closed"}})

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
