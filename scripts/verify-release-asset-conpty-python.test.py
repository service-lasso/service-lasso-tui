import importlib.util
import contextlib
import io
import json
import os
import hashlib
import tempfile
import unittest
from unittest.mock import patch, mock_open


SCRIPT = os.path.join(os.path.dirname(__file__), "verify-release-asset-conpty.py")
SPEC = importlib.util.spec_from_file_location("release_asset_probe", SCRIPT)
probe_module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(probe_module)


class FakePty:
    spawned = None
    exitstatus = 0
    signalstatus = None

    def wait(self):
        self.wait_observed = True
        return self.exitstatus

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

    def setwinsize(self, _rows, _columns):
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
            path = os.path.abspath(executable)
            receipt = {"sourceCommit": source_commit, "binarySHA256": expected_binary_sha256}

            def close(self):
                pass

        return os.path.abspath(executable), Held()

    def test_direct_constructor_requires_exact_invalid_url_marker_without_exposing_terminal_text(self):
        nonce = "0123456789abcdef" * 4
        marker = probe_module.STARTUP_MARKER_PREFIX + nonce + ":api_url_invalid:" + self.source_commit + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX
        observed = {}

        class Completed:
            returncode = 2
            stderr = marker.encode("utf-8")

        def runner(args, **kwargs):
            observed.update({"args": args, "environment": kwargs["env"]})
            return Completed()

        with patch.dict(os.environ, {"SERVICE_LASSO_API_TOKEN": "host-credential", "UNRELATED_HOST_VALUE": "host-only"}):
            result = probe_module.direct_constructor_assertion("candidate.exe", {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}, runner=runner, nonce=nonce)
        self.assertEqual(result, {"exit": "exit_code_2", "startupBoundary": "api_url_invalid", "candidateIdentity": {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}})
        self.assertEqual(observed["args"], ["candidate.exe", "--compat-v1"])
        self.assertEqual(observed["environment"]["SERVICE_LASSO_API_URL"], "://invalid")
        self.assertEqual(observed["environment"]["SERVICE_LASSO_API_TOKEN"], probe_module.INERT_PROBE_TOKEN)
        self.assertNotEqual(observed["environment"]["SERVICE_LASSO_API_TOKEN"], "host-credential")
        self.assertNotIn("UNRELATED_HOST_VALUE", observed["environment"])
        self.assertNotIn(marker, json.dumps(result))

    def test_unavailable_conpty_child_uses_the_same_inert_token_without_host_inheritance(self):
        nonce = "0123456789abcdef" * 4
        observed_waits = []
        def wait(*args):
            observed_waits.append(args[1])
            return "rendered"
        with patch.dict(os.environ, {"SERVICE_LASSO_API_TOKEN": "host-credential", "UNRELATED_HOST_VALUE": "host-only"}):
            with contextlib.redirect_stdout(io.StringIO()) as output:
                result = self.run_probe(
                    "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                    pty_process=FakePty,
                    wait=wait,
                    backend=None,
                    startup_probe_nonce=nonce,
                )
        self.assertEqual(result, 0)
        environment = FakePty.spawned[1]["env"]
        self.assertEqual(environment["SERVICE_LASSO_API_TOKEN"], probe_module.INERT_PROBE_TOKEN)
        self.assertNotEqual(environment["SERVICE_LASSO_API_TOKEN"], "host-credential")
        self.assertNotIn("UNRELATED_HOST_VALUE", environment)
        self.assertNotIn(probe_module.INERT_PROBE_TOKEN, output.getvalue())
        self.assertEqual(observed_waits, [("Service Lasso TUI", "Runtime API unavailable", "Press r to retry")])

    def test_unavailable_still_waiting_is_bounded_before_keyboard_input(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=FakePty, wait=lambda *_args: None, backend=None,
            )
        self.assertEqual(result, 1)
        self.assertEqual(json.loads(output.getvalue()), {"ok": False, "stage": "startup", "receipt": self.receipt("startup", "timeout", "timed_out")})

    def test_direct_constructor_rejects_wrong_exit_or_extra_terminal_text(self):
        nonce = "0123456789abcdef" * 4
        marker = probe_module.STARTUP_MARKER_PREFIX + nonce + ":api_url_invalid:" + self.source_commit + ":" + self.binary_sha256 + probe_module.STARTUP_MARKER_SUFFIX

        class Completed:
            def __init__(self, returncode, stderr):
                self.returncode = returncode
                self.stderr = stderr.encode("utf-8")

        identity = {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}
        self.assertIsNone(probe_module.direct_constructor_assertion("candidate.exe", identity, runner=lambda *_args, **_kwargs: Completed(1, marker), nonce=nonce))
        self.assertIsNone(probe_module.direct_constructor_assertion("candidate.exe", identity, runner=lambda *_args, **_kwargs: Completed(2, "SYNTHETIC_SECRET" + marker), nonce=nonce))

    def test_discriminator_keeps_one_held_identity_through_direct_then_conpty(self):
        direct_identity = {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = probe_module.discriminate_constructor_then_probe(
                "candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None,
                pty_process=FakePty,
                wait=lambda *_args: "Service Lasso TUI Runtime API unavailable Press r to retry",
                backend=None,
                source_commit=self.source_commit,
                expected_binary_sha256=self.binary_sha256,
                identity_acquirer=self.identity_acquirer,
                direct_assertion=lambda executable, identity: {"exit": "exit_code_2", "startupBoundary": "api_url_invalid", "candidateIdentity": identity} if identity == direct_identity else None,
            )
        self.assertEqual(result, 0)
        payload = json.loads(output.getvalue())
        self.assertEqual(payload["directConstructor"]["candidateIdentity"], payload["receipt"]["candidateIdentity"])

    def test_conpty_failure_retains_only_the_direct_constructor_identity(self):
        direct = {"exit": "exit_code_2", "startupBoundary": "api_url_invalid", "candidateIdentity": {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}}
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = self.run_probe("candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None, pty_process=EarlyClosingPty, wait=lambda *_args: (_ for _ in ()).throw(probe_module.TerminalClosed("terminal_exit_code_2", "api_client_error")), backend=None, direct_constructor=direct)
        self.assertEqual(result, 1)
        payload = json.loads(output.getvalue())
        self.assertEqual(payload["directConstructor"], direct)
        self.assertEqual(payload["receipt"]["candidateIdentity"], direct["candidateIdentity"])
        self.assertNotIn("SYNTHETIC_SECRET", output.getvalue())

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
                wait=lambda *_args: "Service Lasso TUI Runtime API unavailable Press r to retry",
                backend=None,
            )
        self.assertEqual(result, 0)
        self.assertEqual(FakePty.spawned[0][0], [os.path.abspath("candidate.exe"), "--compat-v1"])

    def test_actual_main_requires_both_identity_arguments_before_constructor_entry(self):
        base = [SCRIPT, "--executable", "candidate.exe", "--mode", "reconnect", "--api-url", "http://127.0.0.1:1"]
        identities = ["--source-commit", self.source_commit, "--expected-executable-sha256", self.binary_sha256]
        for args, admitted in [(base, False), (base + identities[:2], False), (base + identities[2:], False), (base + identities, True)]:
            with self.subTest(args=args), patch("sys.argv", args), patch.object(probe_module, "discriminate_constructor_then_probe", return_value=17) as constructor, contextlib.redirect_stdout(io.StringIO()):
                result = probe_module.main()
            self.assertEqual(constructor.called, admitted)
            self.assertEqual(result, 17 if admitted else 1)
            if admitted:
                self.assertEqual(constructor.call_args.kwargs["source_commit"], self.source_commit)
                self.assertEqual(constructor.call_args.kwargs["expected_binary_sha256"], self.binary_sha256)

    def test_q_outcomes_traverse_both_actual_release_probe_modes_and_actual_real_core_probe(self):
        import runpy
        import types
        import sys
        # Portable source contract fixture; Windows uses the installed backend.
        modules = {}
        if os.name != "nt":
            enums = types.ModuleType("winpty.enums"); enums.Backend = types.SimpleNamespace(ConPTY=None)
            ptyprocess = types.ModuleType("winpty.ptyprocess"); ptyprocess.PtyProcess = FakePty
            modules = {"winpty": types.ModuleType("winpty"), "winpty.enums": enums, "winpty.ptyprocess": ptyprocess}
        with patch.dict(sys.modules, modules):
            real_core = runpy.run_path(os.path.join(os.path.dirname(SCRIPT), "verify-real-core-conpty.py"))
        for exitstatus, signalstatus, waited, reason in [(0, None, 0, "completed"), (1, None, 1, "terminal_exit_code_1"), (2, None, 2, "terminal_exit_code_2"), (9, None, 9, "terminal_exited_nonzero"), (None, 9, None, "terminal_signaled"), (None, None, None, "terminal_unknown"), (0, None, None, "terminal_unknown"), (True, None, True, "terminal_unknown"), ("sentinel", None, None, "terminal_unknown"), (0, "sentinel", 0, "terminal_unknown"), (0, None, OSError("controlled wait failure"), "terminal_unknown")]:
            for helper_mode in ("unavailable", "reconnect", "connected", "core-unavailable"):
                with self.subTest(exitstatus=exitstatus, signalstatus=signalstatus, waited=waited, mode=helper_mode), tempfile.TemporaryDirectory() as root:
                    class QTerminal(FakePty):
                        def wait(self):
                            self.wait_observed = True
                            if isinstance(waited, Exception): raise waited
                            return waited
                    terminal = QTerminal(); terminal.exitstatus = exitstatus; terminal.signalstatus = signalstatus
                    launches = []
                    def spawn(args, **kwargs):
                        launches.append(args)
                        return terminal
                    pty = types.SimpleNamespace(spawn=spawn)
                    output = io.StringIO()
                    with contextlib.redirect_stdout(output):
                        if helper_mode in ("connected", "core-unavailable"):
                            with patch.dict(real_core["probe"].__globals__, {"PtyProcess": pty, "wait_for": lambda *_a: True}):
                                result = real_core["probe"]("candidate.exe", "unavailable" if helper_mode == "core-unavailable" else helper_mode, "http://127.0.0.1:1")
                        else:
                            result = self.run_probe("candidate.exe", helper_mode, "http://127.0.0.1:1", os.path.join(root, "ready"), "reconnect", None, None, None, pty_process=pty, wait=lambda *_a: "rendered", wait_file=lambda *_a: True, backend=None)
                    payload = json.loads(output.getvalue())
                    self.assertEqual(launches, [[os.path.abspath("candidate.exe") if helper_mode in ("unavailable", "reconnect") else "candidate.exe", "--compat-v1"]])
                    self.assertTrue(terminal.wait_observed)
                    self.assertEqual(result, 0 if reason == "completed" else 1)
                    self.assertEqual(payload["receipt"]["closedReason"], reason)
                    self.assertEqual(payload["receipt"]["outcome"], "normal" if reason == "completed" else "error")


    def test_actual_probe_defers_public_success_until_close_and_retains_original_failures(self):
        primary = OSError("PRIVATE primary terminal read")
        cleanup = OSError("PRIVATE terminal close")
        held_cleanup = OSError("PRIVATE held close")
        events = []
        class BrokenClose(FakePty):
            def close(self, force=True):
                events.append("terminal-close")
                raise cleanup
        terminal = BrokenClose()
        import types
        for failed in (False, True):
            observation = {"failures": [], "cleanupFailures": []}
            def wait(*_args):
                if failed: raise primary
                return "rendered"
            def emit(value):
                events.append("emit")
                self.assertFalse(value["ok"])
                self.assertNotIn("PRIVATE", json.dumps(value))
            events.clear()
            with patch.object(probe_module, "emit", emit):
                result = self.run_probe("candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None, pty_process=types.SimpleNamespace(spawn=lambda *_a, **_k: terminal), wait=wait, backend=None, observation=observation)
            self.assertEqual(result, 1)
            self.assertEqual(events, ["terminal-close", "emit"])
            self.assertEqual(observation["cleanupFailures"], [cleanup])
            self.assertEqual(observation["failures"], [primary] if failed else [])
        class Held:
            path = os.path.abspath("candidate.exe")
            receipt = {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}
            def close(self):
                events.append("held-close")
                raise held_cleanup
        held = Held()
        observation = {"failures": [], "cleanupFailures": []}
        events.clear()
        with patch.object(probe_module, "emit", emit):
            result = probe_module.discriminate_constructor_then_probe("candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None, pty_process=types.SimpleNamespace(spawn=lambda *_a, **_k: terminal), wait=lambda *_a: "rendered", backend=None, identity_acquirer=lambda *_a: (held.path, held), direct_assertion=lambda *_a: {"exit": "exit_code_2"}, observation=observation)
        self.assertEqual(result, 1)
        self.assertEqual(events, ["terminal-close", "held-close", "emit"])
        self.assertEqual(observation["cleanupFailures"], [cleanup, held_cleanup])

    def test_actual_windows_acquisition_retains_conversion_primary_and_once_owned_cleanup(self):
        from unittest.mock import Mock
        import types
        import sys
        import stat
        primary = OSError("PRIVATE conversion")
        secondary = OSError("PRIVATE descriptor close")
        for conversion in ("crt", "file"):
            calls = []
            kernel = types.SimpleNamespace(CreateFileW=Mock(return_value=123), CloseHandle=Mock(return_value=0))
            def open_handle(*_args):
                calls.append("crt")
                if conversion == "crt": raise primary
                return 456
            def open_file(*_args):
                calls.append("file")
                raise primary
            def close_descriptor(value):
                calls.append(("close-descriptor", value))
                raise secondary
            observation = {"failures": [], "cleanupFailures": []}
            with patch.object(probe_module.os, "name", "nt"), patch.object(probe_module.os, "lstat", return_value=types.SimpleNamespace(st_mode=stat.S_IFREG, st_file_attributes=0)), patch.object(probe_module.ctypes, "WinDLL", return_value=kernel, create=True), patch.object(probe_module.ctypes, "get_last_error", return_value=5, create=True), patch.dict(sys.modules, {"msvcrt": types.SimpleNamespace(open_osfhandle=open_handle)}), patch.object(probe_module.os, "fdopen", side_effect=open_file), patch.object(probe_module.os, "close", side_effect=close_descriptor):
                with self.assertRaises(OSError) as caught:
                    probe_module.acquire_candidate_identity("candidate.exe", self.source_commit, self.binary_sha256, observation=observation)
            self.assertIs(caught.exception, primary)
            self.assertFalse(hasattr(primary, "_service_lasso_cleanup_failures"))
            if conversion == "crt":
                kernel.CloseHandle.assert_called_once_with(123)
                self.assertEqual(calls, ["crt"])
                self.assertEqual(len(observation["cleanupFailures"]), 1)
            else:
                kernel.CloseHandle.assert_not_called()
                self.assertEqual(calls, ["crt", "file", ("close-descriptor", 456)])
                self.assertEqual(observation["cleanupFailures"], [secondary])

    def test_useful_release_probe_accepts_nonwritable_foreign_process(self):
        import types
        class SlottedTerminal:
            __slots__ = ()
            exitstatus = 0
            signalstatus = None
            def isalive(self): return False
            def write(self, _value): pass
            def wait(self): return 0
            def close(self, force=True): pass
        terminal = SlottedTerminal()
        observation = {"failures": [], "cleanupFailures": []}
        with contextlib.redirect_stdout(io.StringIO()) as output:
            result = self.run_probe("candidate.exe", "unavailable", "http://127.0.0.1:1", None, None, None, None, None, pty_process=types.SimpleNamespace(spawn=lambda *_a, **_k: terminal), wait=lambda *_a: "rendered", backend=None, observation=observation)
        self.assertEqual(result, 0)
        self.assertTrue(json.loads(output.getvalue())["ok"])
        self.assertEqual(observation["failures"], [])
        self.assertEqual(observation["cleanupFailures"], [])
        self.assertIs(observation["terminalCloses"][0]["process"],terminal)
        self.assertTrue(observation["terminalCloses"][0]["completed"])

    def test_actual_shutdown_then_probe_finally_attempts_terminal_close_once(self):
        import types
        original=OSError("PRIVATE first terminal close")
        for failing in (False,True):
            calls=[]; observation={"failures":[],"cleanupFailures":[]}
            class Terminal:
                __slots__=()
                def close(self,force=True):
                    calls.append("close")
                    if failing: raise original
                def isalive(self): return False
            terminal=Terminal()
            def wait(*_args):
                if failing:
                    for attempt in range(2):
                        with self.assertRaises(RuntimeError): probe_module.check_shutdown(terminal,"request","ack","token",observation)
                    raise RuntimeError("cleanup-unconfirmed")
                probe_module.check_shutdown(terminal,"request","ack","token",observation)
            with patch("builtins.open",mock_open(read_data="token\n")), patch.object(probe_module.os.path,"exists",return_value=True), contextlib.redirect_stdout(io.StringIO()) as output:
                result=self.run_probe("candidate.exe","unavailable","http://127.0.0.1:1",None,None,None,None,None,pty_process=types.SimpleNamespace(spawn=lambda *_a,**_k:terminal),wait=wait,backend=None,observation=observation)
            self.assertEqual(result,1); self.assertEqual(calls,["close"])
            self.assertFalse(json.loads(output.getvalue())["ok"])
            self.assertEqual(observation["cleanupFailures"],[])
            item=observation["terminalCloses"][0]
            self.assertIs(item["process"],terminal); self.assertEqual(item["completed"],not failing)
            self.assertIs(item["original"],original if failing else None)
            if failing:
                self.assertIs(observation["failures"][0],original)
                self.assertEqual(sum(item is original for item in observation["failures"]),1)

    def test_actual_real_core_probe_closes_before_emit_and_preserves_primary(self):
        import runpy
        import types
        import sys
        modules = {}
        if os.name != "nt":
            enums = types.ModuleType("winpty.enums"); enums.Backend = types.SimpleNamespace(ConPTY=None)
            ptyprocess = types.ModuleType("winpty.ptyprocess"); ptyprocess.PtyProcess = FakePty
            modules = {"winpty": types.ModuleType("winpty"), "winpty.enums": enums, "winpty.ptyprocess": ptyprocess}
        with patch.dict(sys.modules, modules):
            actual = runpy.run_path(os.path.join(os.path.dirname(SCRIPT), "verify-real-core-conpty.py"))
        primary, cleanup = OSError("PRIVATE read"), OSError("PRIVATE close")
        for failed in (False, True):
            events = []
            class Terminal:
                __slots__ = ()
                exitstatus = 0
                signalstatus = None
                def isalive(self): return False
                def write(self, _value): pass
                def wait(self): return 0
                def close(self, force=True):
                    events.append("close")
                    raise cleanup
            def wait_for(*_args):
                if failed: raise primary
                return True
            def emit(value):
                events.append("emit")
                self.assertFalse(value["ok"])
                self.assertNotIn("PRIVATE", json.dumps(value))
            observation = {"failures": [], "cleanupFailures": []}
            with patch.dict(actual["probe"].__globals__, {"PtyProcess": types.SimpleNamespace(spawn=lambda *_a, **_k: Terminal()), "wait_for": wait_for, "emit": emit}):
                result = actual["probe"]("candidate.exe", "unavailable", "http://127.0.0.1:1", observation=observation)
            self.assertEqual(result, 1)
            self.assertEqual(events, ["close", "emit"])
            self.assertEqual(observation["failures"], [primary] if failed else [])
            self.assertEqual(observation["cleanupFailures"], [cleanup])

    def test_actual_private_shutdown_wait_and_constructor_failures_keep_originals(self):
        original = OSError("PRIVATE lifecycle")
        observation = {"failures": [], "cleanupFailures": []}
        class Broken:
            def close(self, force=True): raise original
            def wait(self): raise original
        self.assertFalse(probe_module.acknowledge_shutdown(Broken(), "unused", "unused", observation=observation))
        self.assertEqual(probe_module.observed_q_exit(Broken(), observation), "terminal_unknown")
        self.assertIsNone(probe_module.direct_constructor_assertion("candidate.exe", {"sourceCommit": self.source_commit, "binarySHA256": self.binary_sha256}, runner=lambda *_a, **_k: (_ for _ in ()).throw(original), observation=observation))
        self.assertEqual(observation["failures"], [original, original, original])
        eof = EOFError("PRIVATE eof")
        class Exited:
            exitstatus = 0
            signalstatus = None
            def isalive(self): return False
            def read(self): raise eof
        with self.assertRaises(probe_module.TerminalClosed):
            probe_module.wait_for(Exited(), ("ready",), 1, select_fn=lambda *_a: ([object()], [], []), observation=observation)
        self.assertIs(observation["failures"][-1], eof)

if __name__ == "__main__":
    unittest.main()
