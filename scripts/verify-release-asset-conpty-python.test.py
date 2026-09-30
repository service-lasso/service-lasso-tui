import importlib.util
import contextlib
import io
import json
import os
import hashlib
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


class MarkerThenExitPty(ExitedPty):
    def __init__(self, marker, exitstatus=1):
        super().__init__(exitstatus=exitstatus)
        self.marker = marker
        self.reads = 0

    @staticmethod
    def spawn(*_args, **_kwargs):
        raise AssertionError("inject an instance through wait_for")

    def read(self):
        self.reads += 1
        if self.reads == 1:
            return self.marker
        raise EOFError()


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
    source_commit = "0123456789abcdef0123456789abcdef01234567"
    binary_sha256 = "abcdef0123456789" * 4

    def identity_acquirer(self, executable, source_commit, expected_binary_sha256):
        self.assertEqual(source_commit, self.source_commit)
        self.assertEqual(expected_binary_sha256, self.binary_sha256)

        class Held:
            receipt = {"sourceCommit": source_commit, "binarySHA256": expected_binary_sha256}

            def close(self):
                pass

        return os.path.abspath(executable), Held()

    def run_probe(self, *args, **kwargs):
        kwargs.setdefault("source_commit", self.source_commit)
        kwargs.setdefault("expected_binary_sha256", self.binary_sha256)
        kwargs.setdefault("identity_acquirer", self.identity_acquirer)
        return probe_module.probe(*args, **kwargs)

    def receipt(self, stage, outcome, reason, startup_boundary=None):
        result = {"stage": stage, "outcome": outcome, "closedReason": reason, "candidateIdentity": {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}}
        if startup_boundary is not None:
            result["startupBoundary"] = startup_boundary
        return result

    def test_reconnect_timeout_reports_a_closed_receipt_over_the_controlled_stdout_channel(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe(
                "candidate.exe", "reconnect", "http://127.0.0.1:1", None, "reconnect", None, None, None,
                FakePty, wait=lambda *_args: None,
                wait_file=lambda *_args: self.fail("reconnect wait must not run after startup timeout"), backend=None,
            )
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "startup", "receipt": self.receipt("startup", "timeout", "timed_out")})

    def test_failures_have_only_the_closed_metadata_schema(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe("candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None, None)
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
            (ExitedPty(exitstatus=1), "terminal_exit_code_1"),
            (ExitedPty(exitstatus=2), "terminal_exit_code_2"),
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

    def test_exact_attempt_marker_classifies_the_program_run_boundary_without_retaining_terminal_text(self):
        nonce = "0123456789abcdef" * 4
        process = MarkerThenExitPty(probe_module.STARTUP_MARKER_PREFIX + nonce + ":program_run_error:" + self.source_commit + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX)
        with self.assertRaises(probe_module.TerminalClosed) as closed:
            probe_module.wait_for(process, ("ready",), 1, startup_probe_nonce=nonce, candidate_identity={"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}, select_fn=lambda *_args: ([process], [], []))
        self.assertEqual(closed.exception.reason, "terminal_exit_code_1")
        self.assertEqual(closed.exception.startup_boundary, "program_run_error")
        self.assertNotIn(nonce, str(closed.exception))

    def test_exact_attempt_marker_accepts_only_a_closed_typed_api_boundary(self):
        nonce = "0123456789abcdef" * 4
        process = MarkerThenExitPty(probe_module.STARTUP_MARKER_PREFIX + nonce + ":api_url_invalid:" + self.source_commit + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX, exitstatus=2)
        with self.assertRaises(probe_module.TerminalClosed) as closed:
            probe_module.wait_for(process, ("ready",), 1, startup_probe_nonce=nonce, candidate_identity={"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}, select_fn=lambda *_args: ([process], [], []))
        self.assertEqual(closed.exception.reason, "terminal_exit_code_2")
        self.assertEqual(closed.exception.startup_boundary, "api_url_invalid")

    def test_untrusted_or_wrong_attempt_markers_are_unclassified_and_never_reach_the_receipt(self):
        nonce = "0123456789abcdef" * 4
        process = MarkerThenExitPty("SYNTHETIC_SECRET " + probe_module.STARTUP_MARKER_PREFIX + ("f" * 64) + ":program_run_error:" + self.source_commit + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX)
        with self.assertRaises(probe_module.TerminalClosed) as closed:
            probe_module.wait_for(process, ("ready",), 1, startup_probe_nonce=nonce, candidate_identity={"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}, select_fn=lambda *_args: ([process], [], []))
        self.assertEqual(closed.exception.startup_boundary, "unclassified")
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=EarlyClosingPty,
                wait=lambda *_args: (_ for _ in ()).throw(closed.exception), backend=None,
            )
        self.assertEqual(result, 1)
        receipt = json.loads(output.getvalue())["receipt"]
        self.assertEqual(receipt, self.receipt("startup", "error", "terminal_exit_code_1", "unclassified"))
        self.assertNotIn("SYNTHETIC_SECRET", output.getvalue())

    def test_marker_parser_rejects_partial_duplicate_and_foreign_frames(self):
        nonce = "0123456789abcdef" * 4
        identity = {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}
        valid = probe_module.STARTUP_MARKER_PREFIX + nonce + ":program_run_error:" + self.source_commit + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX
        cases = (
            valid + valid,
            valid + probe_module.STARTUP_MARKER_PREFIX + "truncated",
            probe_module.STARTUP_MARKER_PREFIX + nonce + ":program_run_error:" + self.source_commit + ":" + self.binary_sha256,
            probe_module.STARTUP_MARKER_PREFIX + nonce + ":program_run_error:" + ("f" * 40) + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX,
        )
        for text in cases:
            with self.subTest(text=text):
                self.assertEqual(probe_module.startup_boundary_from_terminal_text(text, nonce, identity), "unclassified")

        self.assertEqual(probe_module.startup_boundary_from_terminal_text(valid, nonce, identity), "program_run_error")

    def test_windows_candidate_identity_handle_binds_hashed_bytes_and_denies_replacement(self):
        if os.name != "nt":
            self.skipTest("Windows candidate lock")
        with tempfile.TemporaryDirectory() as root:
            executable = os.path.join(root, "candidate.exe")
            replacement = os.path.join(root, "replacement.exe")
            with open(executable, "wb") as file:
                file.write(b"candidate-bytes")
            with open(replacement, "wb") as file:
                file.write(b"replacement-bytes")
            digest = hashlib.sha256(b"candidate-bytes").hexdigest()
            path, held = probe_module.acquire_candidate_identity(executable, self.source_commit, digest)
            self.assertEqual(path, os.path.abspath(executable))
            self.assertEqual(held.receipt["binarySHA256"], digest)
            with self.assertRaises(OSError):
                os.replace(replacement, executable)
            held.close()

    def test_exited_terminal_eof_has_a_closed_lifecycle_receipt(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=EarlyClosingPty,
                wait=lambda *_args: (_ for _ in ()).throw(probe_module.TerminalClosed("terminal_exited_zero")),
                backend=None,
            )
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "startup", "receipt": self.receipt("startup", "error", "terminal_exited_zero", "unclassified")})

    def test_launch_resolves_a_relative_candidate_before_passing_it_to_pywinpty(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=FakePty,
                wait=lambda *_args: "Service Lasso TUI q quit Runtime API unavailable",
                backend=None,
            )
        self.assertEqual(result, 0)
        self.assertEqual(FakePty.spawned[0][0][0], os.path.abspath("candidate.exe"))


if __name__ == "__main__":
    unittest.main()
