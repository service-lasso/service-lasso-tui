"""Controlled actual Windows driver orchestration; UNRUN, no native proof."""
import importlib.util, json, sys, unittest, queue
from contextlib import ExitStack
from pathlib import Path
from types import ModuleType, SimpleNamespace
from unittest.mock import patch, mock_open

class WindowsOriginalContinuity(unittest.TestCase):
    def load(self):
        # Only the foreign PTY import is replaced. No PTY/kernel is acquired.
        package=ModuleType("winpty"); enums=ModuleType("winpty.enums"); process=ModuleType("winpty.ptyprocess")
        enums.Backend=SimpleNamespace(ConPTY="controlled")
        process.PtyProcess=SimpleNamespace(spawn=lambda *_args,**_kwargs:self.fail("unexpected native spawn"))
        spec=importlib.util.spec_from_file_location("windows_original_continuity",Path(__file__).with_name("tui28-native-windows-five-action.py"))
        module=importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules,{"winpty":package,"winpty.enums":enums,"winpty.ptyprocess":process}): spec.loader.exec_module(module)
        return module

    def test_actual_main_keeps_primary_and_all_suppressed_finalization_originals(self):
        for primary in (OSError("PRIVATE mutation original"),KeyboardInterrupt()):
            module=self.load(); sink=OSError("PRIVATE receipt original"); close=TypeError("PRIVATE held close"); final=RuntimeError("PRIVATE final birth")
            class Held:
                def close(self): raise close
            held=Held(); directory=object(); observation={}; writes=[]
            paths={"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True}
            args=SimpleNamespace(root="controlled-root",executable="controlled-executable",source_commit="a"*40,core_commit="c"*40,external_runtime=True)
            ready={"token":"controlled-token","deniedToken":"controlled-denied","url":"http://controlled","_binarySHA256":"b"*64,"_runtimePID":123,"_runtimeBirth":{"controlled":True}}
            input_stream=SimpleNamespace(readline=lambda _size:json.dumps(ready))
            opened=mock_open(); count=[]
            def open_source(*values,**options):
                count.append(values[0])
                if len(count)==3: raise primary
                return opened(*values,**options)
            def write(_path,value):
                writes.append(value)
                raise sink
            with ExitStack() as stack:
                for target,name,value in ((module.argparse.ArgumentParser,"parse_args",lambda _self:args),(module.sys,"stdin",input_stream),(module,"strict_load",lambda _stream:{"coreCommit":args.core_commit,"runtimePathReceipt":paths} if len(count)==1 else {}),(module,"hold_candidate",lambda *_args:(args.executable,held,{"sourceCommit":args.source_commit,"binarySHA256":"b"*64},directory)),(module,"write_json",write),(module,"process_birth",lambda _pid:(_ for _ in ()).throw(final))):
                    stack.enter_context(patch.object(target,name,value))
                stack.enter_context(patch("builtins.open",side_effect=open_source))
                with self.assertRaises(type(primary)) as caught: module.main(observation)
            self.assertIs(caught.exception,primary)
            self.assertIs(observation["primary"],primary)
            self.assertIs(observation["candidate"]["file"],held)
            self.assertIs(observation["candidate"]["directory"],directory)
            originals=[item["original"] for item in observation["originals"]]
            self.assertEqual(sum(item is sink for item in originals),3)
            self.assertTrue(any(item is close for item in originals))
            self.assertTrue(any(item is final for item in originals))
            self.assertNotIn("PRIVATE",json.dumps(writes))
            self.assertEqual(observation["processes"],[])

    def test_reader_drain_failure_still_joins_and_retains_original(self):
        module=self.load(); original=OSError("PRIVATE reader drain"); events=[]
        alive=iter((True,False))
        reader=SimpleNamespace(is_alive=lambda:next(alive),join=lambda:events.append("joined"))
        process=SimpleNamespace(tui28_reader=reader,tui28_queue=queue.Queue()); observation={"originals":[]}
        with patch.object(module,"drain",side_effect=original):
            with self.assertRaises(OSError) as caught: module.complete_reader(process,[""],observation)
        self.assertIs(caught.exception,original)
        self.assertIs(observation["originals"][0]["original"],original)
        self.assertTrue(process.tui28_reader_joined)
        module.complete_reader(process,[""],observation)
        self.assertEqual(events,["joined"])

    def test_unknown_join_is_retained_without_retry_or_closed_claim(self):
        module=self.load(); original=RuntimeError("PRIVATE unknown join"); calls=[]
        def join():
            calls.append("join")
            raise original
        process=SimpleNamespace(tui28_reader=SimpleNamespace(is_alive=lambda:False,join=join),tui28_queue=queue.Queue())
        observation={"originals":[]}
        for _ in range(2):
            with self.assertRaises(RuntimeError) as caught: module.complete_reader(process,[""],observation)
            self.assertIs(caught.exception,original)
        self.assertEqual(calls,["join"])
        self.assertFalse(getattr(process,"tui28_reader_joined",False))
        self.assertIs(observation["originals"][0]["original"],original)

    def test_completed_reader_keeps_queued_tail_before_failure_sentinel(self):
        module=self.load(); chunks=queue.Queue(); chunks.put("last\r\n"); chunks.put(None); joined=[]
        process=SimpleNamespace(tui28_reader=SimpleNamespace(is_alive=lambda:False,join=lambda:joined.append(True)),tui28_queue=chunks,tui28_reader_failed=True)
        transcript=[""]; observation={"originals":[]}
        with self.assertRaises(RuntimeError): module.complete_reader(process,transcript,observation)
        self.assertEqual(transcript,["last\r\n"])
        self.assertTrue(chunks.empty())
        self.assertEqual(joined,[True])
        self.assertTrue(process.tui28_reader_joined)

    def test_actual_finish_receipt_failure_keeps_process_for_independent_recovery(self):
        module=self.load(); events=[]; sink=OSError("PRIVATE exit receipt")
        class Reader:
            def __init__(self,target): pass
            def start(self): pass
            def is_alive(self): return False
            def join(self): events.append("join")
        process=SimpleNamespace(pid=456,exitstatus=0,isalive=lambda:False,wait=lambda:events.append("wait") or 0)
        held=SimpleNamespace(close=lambda:events.append("candidate-close")); directory=object()
        args=SimpleNamespace(root="controlled-root",executable="controlled-executable",source_commit="a"*40,core_commit="c"*40,external_runtime=True)
        paths={"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True}
        ready={"token":"controlled-token","deniedToken":"controlled-denied","url":"http://controlled","_binarySHA256":"b"*64,"_runtimePID":123,"_runtimeBirth":{}}
        opened=mock_open(); opens=[]; observation={}
        def open_source(*values,**options):
            opens.append(values[0])
            if len(opens)==3: raise PermissionError()
            return opened(*values,**options)
        def write(_path,value):
            if value.get("phase")=="normal_q_exit": raise sink
        class CloseHandle:
            def __call__(self,_handle): events.append("directory-close"); return True
        kernel=SimpleNamespace(CloseHandle=CloseHandle())
        with ExitStack() as stack:
            for target,name,value in ((module.argparse.ArgumentParser,"parse_args",lambda _self:args),(module.sys,"stdin",SimpleNamespace(readline=lambda _size:json.dumps(ready))),(module,"strict_load",lambda _stream:{"coreCommit":args.core_commit,"runtimePathReceipt":paths} if len(opens)==1 else {}),(module,"hold_candidate",lambda *_args:(args.executable,held,{"binarySHA256":"b"*64},directory)),(module,"write_json",write),(module,"process_birth",lambda _pid:{"parentPID":module.os.getpid(),"image":module.os.path.abspath(args.executable)}),(module.os,"replace",lambda *_args:(_ for _ in ()).throw(PermissionError())),(module,"unrelated_snapshot",lambda *_args:{}),(module,"records",lambda *_args:[]),(module,"denial_audit_count",lambda *_args:0),(module.PtyProcess,"spawn",lambda *_args,**_kwargs:process),(module.threading,"Thread",Reader),(module,"wait_for",lambda *_args:None),(module,"close_terminal",lambda *_args:"terminal_exit_code_2")):
                stack.enter_context(patch.object(target,name,value))
            stack.enter_context(patch.object(module.ctypes,"WinDLL",lambda *_args,**_kwargs:kernel,create=True))
            stack.enter_context(patch.object(module.os,"fsync",lambda _fd:None))
            stack.enter_context(patch("builtins.open",side_effect=open_source))
            with self.assertRaises(OSError) as caught: module.main(observation)
        self.assertIs(caught.exception,sink)
        self.assertEqual(observation["processes"],[process])
        self.assertEqual(events,["join","wait","candidate-close","directory-close"])

if __name__=="__main__": unittest.main()
