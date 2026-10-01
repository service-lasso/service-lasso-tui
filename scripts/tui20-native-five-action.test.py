import ast
import os
import pathlib
import runpy
import sys
import tempfile
import unittest
from unittest.mock import patch
import types


def load_posix_harness():
    # The receipt-order functions are platform-neutral; provide import stubs so
    # this controlled recorder-path test also runs on Windows source CI.
    if sys.platform == "linux":
        return runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py")))
    with patch.dict(sys.modules, {"fcntl": types.ModuleType("fcntl"), "pty": types.ModuleType("pty")}):
        return runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py")))


class NativeFiveActionHarnessTests(unittest.TestCase):
    def test_harness_records_closed_exit_and_adverse_audits_without_persisting_tokens(self):
        source = pathlib.Path(__file__).with_name("tui20-native-five-action.py").read_text(encoding="utf-8")
        ast.parse(source)
        self.assertIn('("reload is unavailable",)', source)
        self.assertIn('append_terminal(path,label,chunk)', source)
        self.assertIn('"targetIds","status","outcome","cancellationSupported"', source)
        self.assertIn('"completedOperationNoReplay":True', source)
        self.assertIn('connection profile "missing" credential is unavailable', source)
        self.assertIn('terminal_exited_zero', source)
        self.assertIn('CreateFileW', source)
        self.assertIn('"adverseAudit"', source)
        self.assertIn('"coreDeniedAuditDelta"', source)
        self.assertIn('"unrelatedService"', source)
        self.assertIn('"runtimeState"', source)
        self.assertIn('expected_denials', source)
        self.assertIn('retained operation readback unavailable', source)
        self.assertNotIn('private-token.json', source)

    def test_core_fixture_defines_only_owned_profiles_and_services(self):
        source = pathlib.Path(__file__).with_name("tui20-native-runtime.mjs").read_text(encoding="utf-8")
        self.assertIn('"tui20-fixture"', source)
        self.assertIn('"tui20-unrelated"', source)
        self.assertIn('["invalid", "SERVICE_LASSO_INVALID_TOKEN"]', source)
        self.assertIn('["missing", "SERVICE_LASSO_MISSING_TOKEN"]', source)
        self.assertNotIn('private-token.json', source)
        self.assertIn('runtimePathReceipt', source)
        self.assertIn('coreReadback', source)
        self.assertLess(source.index('Object.assign(process.env, ownedRuntimeEnvironment)'), source.index('const { exportJWK, generateKeyPair, SignJWT }'))
        self.assertIn('SERVICE_LASSO_WORKSPACE_ROOT: workspaceRoot', source)
        self.assertIn('SERVICE_LASSO_INSTANCE_REGISTRY_PATH: instanceRegistryPath', source)
        self.assertIn('SERVICE_LASSO_HOST_PORT_REGISTRY_PATH: hostPortRegistryPath', source)
        controller = pathlib.Path(__file__).with_name("tui20-native-core.mjs").read_text(encoding="utf-8")
        self.assertIn('tui20-native-runtime.mjs', controller)
        self.assertIn('--adverse-controller-crash', controller)

    def test_posix_harness_uses_a_real_pty_and_retains_the_required_boundaries(self):
        source = pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py").read_text(encoding="utf-8")
        ast.parse(source)
        self.assertIn("pty.openpty()", source)
        self.assertIn('"terminal_exited_zero"', source)
        self.assertIn('terminal_signaled', source)
        self.assertIn('live_child_unresolved', source)
        self.assertIn('external recovery owner must observe a live child before finalization', source)
        self.assertIn('ChildReapedUnowned', source)
        self.assertNotIn('self.child.kill()', source)
        self.assertIn('os.open(executable,os.O_RDONLY|getattr(os,"O_NOFOLLOW",0))', source)
        self.assertIn('libc.fexecve', source)
        self.assertIn('os.fchdir(launch["directoryFD"])', source)
        self.assertIn('os.execve("./"+launch["leaf"],argv,env)', source)
        self.assertIn('"reparse"', source)
        self.assertIn('"inplace-content-mutation"', source)
        self.assertIn('linux_sealed_execution', source)
        self.assertIn('darwin_system_immutable_execution', source)
        self.assertIn('F_SEAL_WRITE', source)
        self.assertIn('schg', source)
        self.assertIn('system-immutable-held-directory-execve', source)
        self.assertIn('existing-writer in-place write', source)
        self.assertIn('Darwin immutable leaf digest mismatch', source)
        self.assertIn('persist_primary_outcome(root,outcome,writer)', source)
        self.assertIn('finalize_native_outcome(args.root,outcome', source)
        self.assertIn('Every post-start preflight stays inside this owner boundary', source)
        self.assertIn('owner-preflight-cleanup.json', source)
        self.assertIn('runtime is not None and not live_child', source)
        self.assertIn('OwnedRuntimeAcquisitionFailure', source)
        self.assertNotIn('runtime.kill()', source)
        self.assertIn('owner-finalization-failure-proof.json', source)
        self.assertIn('injected native finalization cleanup failure', source)
        self.assertIn('native-cleanup-receipt.json', source)
        self.assertIn('"adverseAudit"', source)
        self.assertIn('len(set(operation_ids))!=5', source)
        self.assertIn('"mcp.operation.succeeded"', source)
        self.assertIn('"coreAuditCount"', source)
        self.assertIn('"completedOperationNoReplay":True', source)
        self.assertIn('"unrelatedService"', source)
        self.assertIn('"blocked_core_1553_no_adapter"', source)
        self.assertNotIn("private-token.json", source)

    @unittest.skipUnless(sys.platform == "linux", "requires Linux memfd seals")
    def test_linux_kernel_seal_rejects_an_in_place_write(self):
        harness = load_posix_harness()
        with tempfile.NamedTemporaryFile() as candidate:
            candidate.write(b"trusted native executable bytes")
            candidate.flush()
            held, identity, _ = harness["hold_candidate"](candidate.name, "a" * 40)
            sealed = None
            try:
                sealed, receipt = harness["linux_sealed_execution"](held, identity)
                self.assertEqual(receipt["mechanism"], "memfd-fexecve-seals")
                with self.assertRaises(OSError):
                    os.pwrite(sealed, b"X", 0)
            finally:
                if sealed is not None:
                    os.close(sealed)
                os.close(held)

    def test_cleanup_failure_is_recorded_after_primary_without_replacing_it(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "succeeded", "terminals": [{"exit": "terminal_exited_zero"}]}
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        cleanup = lambda *_: {"outcome":"failed","closedReason":"leaf_release_failed","recoveryRetained":True}
        actual = harness["finalize_native_outcome"]("controlled-root", primary, None, object(), -1, cleanup=cleanup, writer=record)
        self.assertEqual(actual["closedReason"], "leaf_release_failed")
        self.assertEqual(writes, [("native-exit-receipt.json", primary), ("native-cleanup-receipt.json", actual)])

    def test_cleanup_receipt_writer_failure_cannot_replace_primary_outcome(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "failed", "failureReason": "native_harness_assertion_failed"}
        def record(path, value):
            writes.append((pathlib.Path(path).name, value))
            if path.endswith("native-cleanup-receipt.json"): raise OSError("controlled recorder failure")
        cleanup = lambda *_: {"outcome":"failed","closedReason":"protected_artifact_removal_failed","recoveryRetained":True}
        actual = harness["finalize_native_outcome"]("controlled-root", primary, None, object(), -1, cleanup=cleanup, writer=record)
        self.assertEqual(actual["recoveryRetained"], True)
        self.assertEqual(writes[0], ("native-exit-receipt.json", primary))

    def test_injected_cleanup_failure_can_be_observed_without_rewriting_primary(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "succeeded", "terminals": [{"exit": "terminal_exited_zero"}]}
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        def failing_cleanup(*_): raise RuntimeError("injected native finalization cleanup failure")
        with self.assertRaisesRegex(RuntimeError, "injected native finalization cleanup failure"):
            harness["finalize_native_outcome"]("controlled-root", primary, None, None, -1, cleanup=failing_cleanup, writer=record)
        self.assertEqual(writes, [("native-exit-receipt.json", primary)])

    def test_negative_owned_child_status_is_signaled_not_nonzero(self):
        harness = load_posix_harness()
        self.assertEqual(harness["terminal_exit_reason"](-9), "terminal_signaled")

    def test_finalization_rejects_a_live_child_without_external_recovery(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "failed", "terminals": [{"exit": "terminal_unknown"}]}
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        with self.assertRaisesRegex(RuntimeError, "external recovery owner"):
            harness["finalize_native_outcome"]("controlled-root", primary, object(), object(), -1, live_child=True, writer=record)
        self.assertEqual(writes, [])

    def test_partial_darwin_activation_is_carried_to_closed_cleanup(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "failed", "heldExecutableBinding": {"partialActivationRecovery": True}}
        recovery = object()
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        def rollback(launch_fd, darwin_protected, held, live_child):
            self.assertIs(launch_fd, recovery)
            self.assertIs(darwin_protected, recovery)
            self.assertFalse(live_child)
            return {"outcome":"failed","closedReason":"parent_release_failed","recoveryRetained":True}
        actual = harness["finalize_native_outcome"]("controlled-root", primary, recovery, recovery, -1, cleanup=rollback, writer=record)
        self.assertTrue(actual["recoveryRetained"])
        self.assertEqual(writes, [("native-exit-receipt.json", primary), ("native-cleanup-receipt.json", actual)])

    @unittest.skipUnless(os.name == "posix", "requires POSIX directory descriptors")
    def test_controlled_darwin_leaf_activation_parent_failure_and_rollback_failure_retains_recovery(self):
        harness = load_posix_harness()
        module_os, module_subprocess, module_stat = harness["os"], harness["subprocess"], harness["stat"]
        actual_open, actual_fstat = module_os.open, module_os.fstat
        state = {"leafActive": False}
        with tempfile.TemporaryDirectory() as root:
            candidate_path = pathlib.Path(root, "candidate")
            candidate_path.write_bytes(b"trusted native executable bytes")
            held = actual_open(str(candidate_path), module_os.O_RDONLY)
            identity = {"binarySHA256": harness["sha256_fd"](held)}
            def run(command, **_kwargs):
                action, target = command[-2:]
                if action == "schg" and target.endswith("launch"):
                    state["leafActive"] = True
                    return types.SimpleNamespace(returncode=0)
                return types.SimpleNamespace(returncode=1)
            def open_protected(path, flags, *args):
                if state["leafActive"] and path.endswith("launch") and flags & module_os.O_RDWR:
                    raise OSError("controlled immutable denial")
                return actual_open(path, flags, *args)
            def fstat_immutable(fd):
                value = actual_fstat(fd)
                return types.SimpleNamespace(st_dev=value.st_dev, st_ino=value.st_ino, st_flags=getattr(module_stat, "SF_IMMUTABLE", 0x00020000))
            def pwrite_denied(fd, data, offset):
                if state["leafActive"]:
                    raise OSError("controlled immutable denial")
                return len(data)
            with patch.object(module_os, "stat_result", types.SimpleNamespace(st_flags=1)), patch.object(module_os, "open", open_protected), patch.object(module_os, "fstat", fstat_immutable), patch.object(module_os, "pwrite", pwrite_denied, create=True), patch.object(module_subprocess, "run", run):
                with self.assertRaises(harness["DarwinActivationFailure"]) as raised:
                    harness["darwin_system_immutable_execution"](held, identity, root)
                recovery = raised.exception.recovery
                primary = {"outcome":"failed", "heldExecutableBinding":{"partialActivationRecovery":True}}
                writes = []
                actual = harness["finalize_native_outcome"](root, primary, recovery, recovery, held, writer=lambda path, value: writes.append((pathlib.Path(path).name, value)))
            self.assertEqual(actual, {"outcome":"failed","closedReason":"leaf_release_failed","recoveryRetained":True})
            self.assertEqual(writes, [("native-exit-receipt.json", primary), ("native-cleanup-receipt.json", actual)])
            module_os.close(recovery["leafFD"])
            module_os.close(recovery["directoryFD"])


if __name__ == "__main__":
    unittest.main()
