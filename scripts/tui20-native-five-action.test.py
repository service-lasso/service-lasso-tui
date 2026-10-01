import ast
import os
import pathlib
import runpy
import sys
import tempfile
import unittest


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
        source = pathlib.Path(__file__).with_name("tui20-native-core.mjs").read_text(encoding="utf-8")
        self.assertIn('"tui20-fixture"', source)
        self.assertIn('"tui20-unrelated"', source)
        self.assertIn('["invalid", "SERVICE_LASSO_INVALID_TOKEN"]', source)
        self.assertIn('["missing", "SERVICE_LASSO_MISSING_TOKEN"]', source)
        self.assertNotIn('private-token.json', source)
        self.assertIn('SERVICE_LASSO_TUI20_TOKEN', source)
        self.assertIn('runtimePathReceipt', source)
        self.assertIn('corePathReadback', source)

    def test_posix_harness_uses_a_real_pty_and_retains_the_required_boundaries(self):
        source = pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py").read_text(encoding="utf-8")
        ast.parse(source)
        self.assertIn("pty.openpty()", source)
        self.assertIn('"terminal_exited_zero"', source)
        self.assertIn('os.open(executable,os.O_RDONLY)', source)
        self.assertIn('libc.fexecve', source)
        self.assertIn('"/dev/fd/"+str(held)', source)
        self.assertIn('"reparse"', source)
        self.assertIn('"inplace-content-mutation"', source)
        self.assertIn('linux_sealed_execution', source)
        self.assertIn('darwin_system_immutable_execution', source)
        self.assertIn('F_SEAL_WRITE', source)
        self.assertIn('schg', source)
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
        harness = runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py")))
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


if __name__ == "__main__":
    unittest.main()
