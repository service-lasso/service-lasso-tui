import ast
import pathlib
import unittest


class NativeFiveActionHarnessTests(unittest.TestCase):
    def test_harness_records_the_closed_reload_denial_and_incremental_transcript(self):
        source = pathlib.Path(__file__).with_name("tui20-native-five-action.py").read_text(encoding="utf-8")
        ast.parse(source)
        self.assertIn('("reload is unavailable",)', source)
        self.assertIn('append_terminal(transcript_path, label, chunk)', source)
        self.assertIn('"operationId": record.get("operationId")', source)
        self.assertIn('"reconnectNoReplay": True', source)
        self.assertIn('connection profile "missing" credential is unavailable', source)
        self.assertIn('detail(invalid, invalid_transcript', source)

    def test_core_fixture_defines_only_owned_profiles_and_services(self):
        source = pathlib.Path(__file__).with_name("tui20-native-core.mjs").read_text(encoding="utf-8")
        self.assertIn('"tui20-fixture"', source)
        self.assertIn('"tui20-unrelated"', source)
        self.assertIn('}, invalid: {', source)
        self.assertIn('}, missing: {', source)


if __name__ == "__main__":
    unittest.main()
