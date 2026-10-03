"""Source boundary regressions; actual Windows ConPTY acceptance is separate."""
import importlib.util, os, sys, tempfile, unittest
from unittest.mock import patch
from pathlib import Path
# Dependency import is source tested only on the matching pinned Windows job.
class NativeOwnerBoundaries(unittest.TestCase):
    @unittest.skipUnless(sys.platform=="win32","requires actual hash-pinned Windows ConPTY dependency")
    def test_actual_close_boundary_requires_wait_status_after_nonlive_observation(self):
        source=Path(__file__).with_name("tui28-native-windows-five-action.py")
        spec=importlib.util.spec_from_file_location("tui28_windows_owner",source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        class Child:
            exitstatus=0;signalstatus=None
            def isalive(self): return False
            def wait(self): return 1
        with patch.object(module,"drain",return_value=""):
            with self.assertRaisesRegex(RuntimeError,"actual terminal wait mismatch"):module.close_terminal(Child(),[""])
    def test_actual_observer_failed_birth_never_proves_absence(self):
        source=Path(__file__).with_name("tui28-native-owner-observer.py")
        spec=importlib.util.spec_from_file_location("tui28_observer",source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        self.assertFalse(module.birth_absent(os.getpid(),None))
        with patch.object(module,"process_birth",side_effect=OSError("fixture unavailable")):
            self.assertFalse(module.birth_absent(os.getpid(),{"fixture":"birth"}))
    def test_actual_ready_timeout_retains_runtime_before_reader_acquisition(self):
        source=Path(__file__).with_name("tui28-native-owner-observer.py")
        spec=importlib.util.spec_from_file_location("tui28_observer_timeout",source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        from types import SimpleNamespace
        class Stream:
            def readline(self,size): return ""
        child=SimpleNamespace(pid=os.getpid(),stdout=Stream())
        args=SimpleNamespace(node="fixture-node",runtime_script="fixture-script",root="fixture-root",core_commit="a"*40,invalid_ready_receipt=False,runtime_ready_mode=None,shutdown_pipe_failure=False)
        retained=[]
        with patch.object(module.subprocess,"Popen",return_value=child):
            with self.assertRaises(module.RuntimeAcquisitionFailure): module.start_runtime(args,retained.append)
        self.assertEqual(retained,[child]);child.tui28_reader.join()
if __name__=="__main__":unittest.main()
