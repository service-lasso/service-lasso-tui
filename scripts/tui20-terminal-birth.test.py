"""Actual POSIX fork/constructor ownership regressions; no Core acceptance."""
import os
import pathlib
import runpy
import sys
import unittest
from unittest.mock import patch

class TerminalBirthTests(unittest.TestCase):
    def test_first_and_later_actual_postfork_birth_failures_keep_latest_owned_child(self):
        self.assertIn(sys.platform, ("linux", "darwin"))
        harness = runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py")))
        constructor = harness["Terminal"]
        registry = harness["OWNED_TERMINALS"]
        executable = os.path.realpath(sys.executable)
        fd = os.open(executable, os.O_RDONLY)
        directory = os.open(os.path.dirname(executable), os.O_RDONLY)
        info = os.lstat(executable)
        launch = fd if sys.platform == "linux" else {"directoryFD": directory, "leaf": os.path.basename(executable), "leafIdentity": (info.st_dev, info.st_ino), "systemImmutable": 0x20000}
        # Darwin's unprotected fixture deliberately cannot pass exec admission;
        # the actual production fork boundary still creates an owned child.
        try:
            for attempt in range(2):
                previous = registry[-1] if registry else None
                with patch.dict(constructor.__init__.__globals__, {"process_birth": lambda _pid: (_ for _ in ()).throw(OSError("controlled actual post-fork birth failure"))}):
                    with self.assertRaises(OSError):
                        constructor(launch, executable, "birth-failure-%d" % attempt, {"PATH": os.environ.get("PATH", ""), "TERM": "xterm-256color"})
                self.assertEqual(len(registry), attempt + 1)
                owned = registry[-1]
                self.assertIsNot(owned, previous)
                self.assertEqual(owned.label, "birth-failure-%d" % attempt)
                self.assertEqual(owned.text, "")
                self.assertIsNone(owned.child.tui20_birth)
                self.assertIsNone(owned.child.returncode)
                self.assertGreater(owned.child.pid, 0)
                # Unknown birth does not authorize release. Direct wait through
                # this same owner positively reaps the actual forked child.
                recovered = owned.recover_until_observed()
                self.assertTrue(recovered["observed"])
                self.assertIsNotNone(owned.child.returncode)
                with self.assertRaises(ChildProcessError): os.waitpid(owned.child.pid, os.WNOHANG)
        finally:
            # No PID signal/kill: only actual owner waits, even after assertion.
            for owned in registry:
                if owned.child.returncode is None:
                    owned.recover_until_observed()
            os.close(directory); os.close(fd)

if __name__ == "__main__": unittest.main()
