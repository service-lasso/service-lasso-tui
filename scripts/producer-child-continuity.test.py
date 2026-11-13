"""Actual producer child graph with controlled acquisition; UNRUN, no native proof."""
import importlib.util, unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

class ProducerChildContinuity(unittest.TestCase):
    def load(self):
        spec=importlib.util.spec_from_file_location("producer_child_continuity",Path(__file__).with_name("tui28-native-producer.py"))
        module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module); return module

    def test_nonzero_keeps_actual_child_bytes_and_status_under_safe_error(self):
        module=self.load(); stdout=b"PRIVATE\x00\xff\r\n"; stderr=b"PRIVATE error\xff"; observation=[]
        child=SimpleNamespace(returncode=7,communicate=lambda:(stdout,stderr))
        with patch.object(module.subprocess,"Popen",return_value=child):
            with self.assertRaises(module.NativeChildFailure) as caught: module.run([],"",{},observation)
        item=observation[0]
        self.assertIs(caught.exception.observation,item)
        self.assertIs(item["process"],child)
        self.assertIs(item["stdout"],stdout); self.assertIs(item["stderr"],stderr)
        self.assertEqual(item["returncode"],7); self.assertTrue(item["completed"])
        self.assertNotIn("PRIVATE",str(caught.exception))

    def test_actual_communicate_control_original_and_child_remain_owned(self):
        module=self.load(); original=KeyboardInterrupt(); observation=[]
        def communicate(): raise original
        child=SimpleNamespace(returncode=None,communicate=communicate)
        with patch.object(module.subprocess,"Popen",return_value=child):
            with self.assertRaises(KeyboardInterrupt) as caught: module.run([],"",{},observation)
        self.assertIs(caught.exception,original)
        self.assertIs(observation[0]["process"],child)
        self.assertIs(observation[0]["original"],original)
        self.assertFalse(observation[0]["completed"])

    def test_actual_main_shares_child_graph_without_changing_useful_stdout(self):
        module=self.load(); observation={}; output=b"useful\r\n"
        children=[SimpleNamespace(returncode=0,communicate=lambda:(output,b"")),SimpleNamespace(returncode=3,communicate=lambda:(b"",b"private"))]
        def attempt(run):
            self.assertIs(run([],"",{}),output)
            return run([],"",{})
        with patch.object(module.subprocess,"Popen",side_effect=children),patch.object(module,"_main_attempt",attempt):
            with self.assertRaises(module.NativeChildFailure): module.main(observation)
        self.assertEqual([item["process"] for item in observation["children"]],children)
        self.assertEqual([item["returncode"] for item in observation["children"]],[0,3])

if __name__=="__main__": unittest.main()
