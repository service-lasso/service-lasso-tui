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
    @staticmethod
    def spawn(*_args, **_kwargs):
        return FakePty()

    def close(self, force=True):
        self.closed = force


class ReceiptTests(unittest.TestCase):
    def test_reconnect_timeout_uses_the_helper_path_and_writes_no_terminal_text(self):
        with tempfile.TemporaryDirectory() as root:
            receipt = os.path.join(root, "helper-outcome.json")
            with contextlib.redirect_stdout(io.StringIO()):
                result = probe_module.probe(
                    "candidate.exe", "reconnect", "http://127.0.0.1:1", None, "reconnect", None, None, None,
                    root, receipt, FakePty,
                    wait=lambda *_args: None,
                    wait_file=lambda *_args: self.fail("reconnect wait must not run after startup timeout"),
                    backend=None,
                )
            self.assertEqual(result, 1)
            with open(receipt, encoding="utf-8") as saved:
                self.assertEqual(json.load(saved), {"stage": "startup", "outcome": "timeout", "closedReason": "timed_out"})

    def test_existing_receipt_is_not_replaced(self):
        with tempfile.TemporaryDirectory() as root:
            receipt = os.path.join(root, "helper-outcome.json")
            with open(receipt, "x", encoding="utf-8") as saved:
                saved.write("foreign")
            self.assertFalse(probe_module.write_receipt(root, receipt, "startup", "timeout", "timed_out"))
            with open(receipt, encoding="utf-8") as saved:
                self.assertEqual(saved.read(), "foreign")

    def test_receipt_sink_failure_does_not_remove_a_foreign_legacy_temporary_name(self):
        with tempfile.TemporaryDirectory() as root:
            receipt = os.path.join(root, "helper-outcome.json")
            foreign_temporary = os.path.join(root, ".helper-outcome.json.foreign.tmp")
            with open(foreign_temporary, "x", encoding="utf-8") as saved:
                saved.write("foreign")
            with patch("builtins.open", side_effect=OSError("injected receipt sink failure")):
                self.assertFalse(probe_module.write_receipt(root, receipt, "startup", "timeout", "timed_out"))
            self.assertFalse(os.path.lexists(receipt))
            with open(foreign_temporary, encoding="utf-8") as saved:
                self.assertEqual(saved.read(), "foreign")

    def test_receipt_does_not_follow_a_symlink_or_windows_reparse_target(self):
        with tempfile.TemporaryDirectory() as root:
            receipt = os.path.join(root, "helper-outcome.json")
            foreign = os.path.join(root, "foreign")
            with open(foreign, "x", encoding="utf-8") as saved:
                saved.write("foreign")
            try:
                os.symlink(foreign, receipt)
            except OSError as error:
                self.skipTest("symbolic links unavailable: " + str(error))
            self.assertFalse(probe_module.write_receipt(root, receipt, "startup", "timeout", "timed_out"))
            with open(foreign, encoding="utf-8") as saved:
                self.assertEqual(saved.read(), "foreign")


if __name__ == "__main__":
    unittest.main()
