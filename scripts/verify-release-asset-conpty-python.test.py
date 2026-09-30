import importlib.util
import contextlib
import io
import json
import os
import tempfile
import unittest


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


if __name__ == "__main__":
    unittest.main()
