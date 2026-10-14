"""Controlled actual observer helpers; source cases remain UNRUN."""
import importlib.util
from pathlib import Path
from types import SimpleNamespace
import unittest
from contextlib import ExitStack
from unittest.mock import patch, mock_open

class ObserverStreamContinuity(unittest.TestCase):
    def load(self):
        spec=importlib.util.spec_from_file_location("observer_stream_continuity",Path(__file__).with_name("tui28-native-owner-observer.py"))
        module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        return module

    def test_actual_ready_reader_keeps_original_exception_instead_of_fabricated_eof(self):
        module=self.load(); original=OSError("PRIVATE ready read")
        class Stream:
            def readline(self,size): raise original
        child=SimpleNamespace(pid=123,stdout=Stream())
        args=SimpleNamespace(node="fixture-node",runtime_script="fixture-script",root="fixture-root",core_commit="a"*40,invalid_ready_receipt=False,runtime_ready_mode=None,shutdown_pipe_failure=False)
        retained=[]; observation=[]
        with patch.object(module.subprocess,"Popen",return_value=child), patch.object(module,"process_birth",return_value={"fixture":True}):
            with self.assertRaises(module.RuntimeAcquisitionFailure) as caught: module.start_runtime(args,retained.append,observation)
        child.tui28_reader.join()
        self.assertEqual(retained,[child]); self.assertIs(caught.exception.__cause__,original)
        self.assertIs(observation[0]["stream"],child.stdout); self.assertIs(observation[0]["readError"],original)
        self.assertIsNone(observation[0]["raw"]); self.assertNotIn("PRIVATE",str(caught.exception))

    def test_actual_drain_and_close_preserve_originals_and_attempt_other_streams_once(self):
        module=self.load(); read_original=OSError("PRIVATE read"); close_original=ValueError("PRIVATE close")
        raw=bytearray(b"PRIVATE bytes"); calls=[]
        class Stream:
            def __init__(self,name): self.name=name
            def read(self):
                calls.append((self.name,"read"))
                if self.name=="stdout": raise read_original
                return raw
            def close(self):
                calls.append((self.name,"close"))
                if self.name=="stdout": raise close_original
        child=SimpleNamespace(stdin=Stream("stdin"),stdout=Stream("stdout"),stderr=Stream("stderr"))
        observation=[]; closed=module.close_streams(child,observation)
        self.assertEqual(closed,{"stdin":True,"stdout":False,"stderr":True})
        self.assertIs(observation[0]["process"],child)
        stdout=observation[0]["streams"][1]; stderr=observation[0]["streams"][2]
        self.assertIs(stdout["stream"],child.stdout)
        self.assertIs(stdout["readError"],read_original); self.assertIs(stdout["closeError"],close_original)
        self.assertIs(stderr["raw"],raw); self.assertTrue(module.stream_failure(child,observation))
        self.assertIs(module.close_streams(child,observation),closed)
        self.assertEqual(calls,[("stdin","close"),("stdout","read"),("stdout","close"),("stderr","read"),("stderr","close")])

    def test_actual_runtime_result_projects_completed_closes_without_private_originals(self):
        module=self.load(); original=OSError("PRIVATE close")
        class Stream:
            def __init__(self,fail=False): self.fail=fail
            def write(self,value): pass
            def flush(self): pass
            def read(self): return "PRIVATE raw"
            def close(self):
                if self.fail: raise original
        child=SimpleNamespace(stdin=Stream(),stdout=Stream(True),stderr=Stream(),pid=123,returncode=0,tui20_birth={"fixture":True},wait=lambda:None)
        observation=[]
        with patch.object(module,"birth_absent",return_value=True): result=module.close_runtime(child,observation)
        self.assertFalse(result["runtimeStdoutClosed"]); self.assertTrue(result["runtimeStderrClosed"])
        self.assertTrue(result["actualExitObserved"]); self.assertTrue(module.stream_failure(child,observation))
        self.assertNotIn("PRIVATE",str(result)); self.assertNotIn("stream",result)
        self.assertIs(observation[0]["streams"][1]["closeError"],original)

    def test_opaque_stream_exceptions_continue_but_control_flow_baseexception_propagates(self):
        module=self.load()
        for read_original,close_original in ((RuntimeError("PRIVATE read"),TypeError("PRIVATE close")),(OSError("PRIVATE read"),ValueError("PRIVATE close"))):
            calls=[]
            class Stream:
                def __init__(self,fail=False): self.fail=fail
                def read(self):
                    calls.append("read")
                    if self.fail: raise read_original
                    return "useful drain"
                def close(self):
                    calls.append("close")
                    if self.fail: raise close_original
            child=SimpleNamespace(stdin=None,stdout=Stream(True),stderr=Stream()); observation=[]
            closed=module.close_streams(child,observation)
            self.assertEqual(calls,["read","close","read","close"])
            self.assertFalse(closed["stdout"]); self.assertTrue(closed["stderr"])
            self.assertIs(observation[0]["streams"][0]["readError"],read_original)
            self.assertIs(observation[0]["streams"][0]["closeError"],close_original)
        control=KeyboardInterrupt()
        class Interrupted:
            def read(self): raise control
            def close(self): raise AssertionError("control-flow interruption must propagate")
        with self.assertRaises(KeyboardInterrupt) as caught: module.close_streams(SimpleNamespace(stdin=None,stdout=Interrupted(),stderr=None),[])
        self.assertIs(caught.exception,control)

    def test_actual_shutdown_and_finally_close_stdin_once_and_keep_write_close_originals(self):
        module=self.load()
        for failing in (False,True):
            write_original=RuntimeError("PRIVATE write"); close_original=TypeError("PRIVATE close"); calls=[]
            class Input:
                def write(self,value):
                    calls.append("write")
                    if failing: raise write_original
                def flush(self): calls.append("flush")
                def close(self):
                    calls.append("close")
                    if failing: raise close_original
            child=SimpleNamespace(stdin=Input(),stdout=None,stderr=None,pid=123,returncode=0,tui20_birth={"fixture":True},wait=lambda:calls.append("wait"))
            observation=[]
            with patch.object(module,"birth_absent",return_value=True):
                result=module.close_runtime(child,observation)
                self.assertEqual(module.close_runtime(child,observation),result)
            module.close_owned_stdin(child,observation); module.close_streams(child,observation)
            self.assertEqual(calls.count("close"),1); self.assertEqual(calls.count("wait"),1)
            self.assertEqual(calls.count("write"),1); self.assertEqual(calls.count("flush"),0 if failing else 1)
            self.assertEqual(result["shutdownRequested"],not failing)
            self.assertNotIn("PRIVATE",str(result))
            if failing:
                self.assertIs(observation[0]["operations"][0]["original"],write_original)
                self.assertIs(observation[0]["streams"][0]["closeError"],close_original)
                self.assertTrue(module.stream_failure(child,observation))
            else:
                self.assertEqual(calls,["write","flush","close","wait"])
                self.assertTrue(observation[0]["closed"]["stdin"])
                self.assertFalse(module.stream_failure(child,observation))

    def test_actual_main_preserves_lifecycle_primary_and_both_finalization_receipt_originals(self):
        module=self.load(); primary=RuntimeError("PRIVATE acquisition"); secondary=OSError("PRIVATE private receipt"); tertiary=TypeError("PRIVATE public receipt")
        args=SimpleNamespace(executable="unused",root="unused",source_commit="a"*40)
        observation={}
        with patch.object(module.argparse.ArgumentParser,"parse_args",return_value=args), patch.object(module,"sha256",return_value="b"*64), patch.object(module,"start_runtime",side_effect=primary), patch.object(module,"write_private",side_effect=secondary), patch.object(module,"write_public",side_effect=tertiary):
            with self.assertRaises(SystemExit) as caught: module.main(observation)
        self.assertIs(caught.exception,observation["primary"]); self.assertIs(caught.exception.__cause__,primary)
        originals=[item["original"] for item in observation["originals"]]
        self.assertTrue(any(item is primary for item in originals)); self.assertTrue(any(item is secondary for item in originals)); self.assertTrue(any(item is tertiary for item in originals))
        self.assertIsNone(observation["pendingPublic"])

    def test_actual_main_owner_wait_failure_still_attempts_owned_runtime_peer(self):
        module=self.load(); primary=RuntimeError("PRIVATE owner wait"); secondary=OSError("PRIVATE runtime peer"); calls=[]
        class Input:
            def write(self,value): calls.append("write")
            def close(self): calls.append("close")
        def wait():
            calls.append("wait")
            raise primary
        owner=SimpleNamespace(pid=456,stdin=Input(),stdout=None,stderr=None,wait=wait)
        runtime=SimpleNamespace(pid=123,tui20_birth={"fixture":True})
        args=SimpleNamespace(executable="unused",root="unused",source_commit="a"*40,core_commit="c"*40,owner="unused",adverse_controller_crash=False,invalid_ready_receipt=False,inject_finalization_cleanup_failure=False,runtime_ready_mode=None,shutdown_pipe_failure=False,adverse_owner_death=False)
        ready_original=OSError("PRIVATE retained ready")
        def start(_args,retain,_observation):
            _observation.append({"stream":runtime,"raw":None,"readError":ready_original})
            retain(runtime)
            return runtime,{}
        def close_peer(child,_observation):
            self.assertIs(child,runtime); calls.append("peer")
            raise secondary
        observation={}
        with ExitStack() as stack:
            for target,name,value in ((module.argparse.ArgumentParser,"parse_args",lambda _self:args),(module,"sha256",lambda _path:"b"*64),(module,"start_runtime",start),(module.subprocess,"Popen",lambda *_args,**_kwargs:owner),(module,"process_birth",lambda _pid:{"fixture":True}),(module,"write_private",lambda *_args:None),(module,"close_runtime",close_peer)):
                stack.enter_context(patch.object(target,name,value))
            with self.assertRaises(SystemExit) as caught: module.main(observation)
        self.assertIs(caught.exception.__cause__,primary)
        self.assertIs(caught.exception,observation["primary"])
        self.assertEqual(calls,["write","close","wait","peer"])
        self.assertTrue(any(item["original"] is secondary for item in observation["originals"]))
        self.assertIs(observation["ready"][0]["readError"],ready_original)
        self.assertIs(observation["streams"][0]["process"],owner)
        self.assertIs(observation["streams"][0]["operations"][0]["original"],primary)

    def test_actual_main_publishes_useful_success_only_after_finalizer_and_refuses_failed_finalizer(self):
        for failure_mode in (None,"write","read","unknown"):
            failing=failure_mode is not None
            module=self.load(); original=OSError("PRIVATE finalizer receipt"); events=[]; writes=[]; owner_receipts=[]
            class Stream:
                def write(self,value): writes.append(value)
                def flush(self): pass
                def close(self): pass
                def read(self): return ""
            birth={"fixture":True}
            runtime=SimpleNamespace(pid=123,stdin=Stream(),stdout=Stream(),stderr=Stream(),returncode=0,tui20_birth=birth,wait=lambda:0)
            owner=SimpleNamespace(pid=456,stdin=Stream(),stdout=Stream(),stderr=Stream(),wait=lambda:0)
            args=SimpleNamespace(executable="unused",root="unused",source_commit="a"*40,core_commit="c"*40,owner="unused",adverse_controller_crash=False,invalid_ready_receipt=False,inject_finalization_cleanup_failure=False,runtime_ready_mode=None,shutdown_pipe_failure=False,adverse_owner_death=False)
            live={"externalOwnerPID":456,"coreRuntimePID":123,"sourceCommit":args.source_commit,"binarySHA256":"b"*64,"ownerBirth":birth,"runtimeBirth":birth,"phase":"normal_q_exit"}
            finalization={"externalOwnerPID":456,"ownerBirth":birth,"runtimePID":123,"runtimeBirth":birth,"sourceCommit":args.source_commit,"binarySHA256":"b"*64,"finalizationFailure":None,"liveTuiChildRetained":False,"primaryOutcome":"succeeded"}
            def start(_args,retain,_observation):
                retain(runtime)
                return runtime,{}
            def private(_root,phase,_value):
                events.append(("private",phase))
                if phase=="owner-closed": owner_receipts.append(_value)
                if failure_mode=="write" and phase=="owner-closed" and events.count(("private","owner-closed"))==2: raise original
            def public(_root,_source,_binary,result,_actions,_closed): events.append(("public",result))
            reads=[]
            def read(_stream):
                reads.append(True)
                if len(reads)==1: return live
                if len(reads)==2: return finalization
                if failure_mode=="read": raise original
                if failure_mode=="unknown": return {"unknown":"PRIVATE field"}
                return {"classification":"owner-private","phase":"owner-closed",**owner_receipts[0]}
            observation={}
            with ExitStack() as stack:
                for target,name,value in ((module.argparse.ArgumentParser,"parse_args",lambda _self:args),(module,"sha256",lambda _path:"b"*64),(module,"start_runtime",start),(module.subprocess,"Popen",lambda *_args,**_kwargs:owner),(module,"process_birth",lambda _pid:birth),(module,"birth_absent",lambda *_args:True),(module.os.path,"exists",lambda _path:True),(module,"write_private",private),(module,"write_public",public)):
                    stack.enter_context(patch.object(target,name,value))
                stack.enter_context(patch("builtins.open",mock_open(read_data="{}")))
                stack.enter_context(patch.object(module,"strict_load",side_effect=read))
                if failing:
                    with self.assertRaises(RuntimeError if failure_mode=="unknown" else OSError) as caught: module.main(observation)
                    if failure_mode!="unknown": self.assertIs(caught.exception,original)
                    else: self.assertNotIn("PRIVATE",str(caught.exception))
                else: module.main(observation)
            self.assertEqual(events.count(("private","owner-closed")),1 if failure_mode in ("read","unknown") else 2)
            self.assertEqual(events[-1],("public","failed" if failing else "succeeded"))
            self.assertEqual(events.count(("public","succeeded")),0 if failing else 1)
            self.assertEqual(len(writes),2)
            self.assertIs(observation["streams"][0]["process"],owner)
            self.assertTrue(any(item["process"] is runtime for item in observation["streams"]))

    def test_actual_wait_and_reader_join_keep_exact_original_and_completed_once_result(self):
        module=self.load()
        for operation in ("wait","join"):
            original=RuntimeError("PRIVATE owned operation"); calls=[]
            class Reader:
                def join(self):
                    calls.append("join")
                    if operation=="join": raise original
            class Input:
                def write(self,value): calls.append("write")
                def flush(self): calls.append("flush")
                def close(self): calls.append("close")
            def wait():
                calls.append("wait")
                if operation=="wait": raise original
                return 0
            child=SimpleNamespace(stdin=Input(),stdout=None,stderr=None,pid=123,returncode=0,tui20_birth={"fixture":True},tui28_reader=Reader(),wait=wait)
            observation=[]
            for attempt in range(2):
                with self.assertRaises(RuntimeError) as caught: module.close_runtime(child,observation)
                self.assertIs(caught.exception,original)
            operations=observation[0]["operations"]
            failed=next(item for item in operations if item["operation"]==("wait" if operation=="wait" else "ready_reader_join"))
            self.assertIs(failed["original"],original); self.assertFalse(failed["completed"])
            self.assertEqual(calls.count("wait"),1); self.assertEqual(calls.count("close"),1)
            self.assertEqual(calls.count("write"),1); self.assertEqual(calls.count("flush"),1)
            self.assertEqual(calls.count("join"),0 if operation=="wait" else 1)
        values=[]
        useful=SimpleNamespace(wait=lambda:values.append("wait") or 17)
        observation=[]
        self.assertEqual(module.wait_owned(useful,observation),17)
        self.assertEqual(module.wait_owned(useful,observation),17)
        self.assertEqual(values,["wait"])
        item=observation[0]["operations"][0]
        self.assertIs(item["subject"],useful); self.assertTrue(item["completed"]); self.assertIsNone(item["original"])

    def test_actual_ready_reader_retains_binary_bytes_before_strict_decode(self):
        for raw in (b'{"event":"ready","url":"http://fixture","token":"private","deniedToken":"denied","jwksPort":1234}\r\n', b'\xff\r\n'):
            module=self.load(); reads=[]; acquired=[]; retained=[]; observation=[]
            class BinaryReady:
                def readline(self,size):
                    reads.append(size)
                    return raw
            child=SimpleNamespace(pid=123,stdout=BinaryReady())
            args=SimpleNamespace(node="fixture-node",runtime_script="fixture-script",root="fixture-root",core_commit="a"*40,invalid_ready_receipt=False,runtime_ready_mode=None,shutdown_pipe_failure=False)
            def popen(*values,**options):
                acquired.append(options)
                return child
            with patch.object(module.subprocess,"Popen",side_effect=popen), patch.object(module,"process_birth",return_value={"fixture":True}):
                if raw.startswith(b'\xff'):
                    with self.assertRaises(module.RuntimeAcquisitionFailure) as caught: module.start_runtime(args,retained.append,observation)
                    self.assertIsInstance(caught.exception.__cause__,UnicodeDecodeError)
                    self.assertNotIn("private",str(caught.exception))
                else:
                    runtime,ready=module.start_runtime(args,retained.append,observation)
                    self.assertIs(runtime,child); self.assertEqual(ready["event"],"ready")
            child.tui28_reader.join()
            self.assertEqual(retained,[child]); self.assertEqual(reads,[1048577])
            self.assertIs(acquired[0]["text"],False)
            self.assertIs(observation[0]["raw"],raw)
            self.assertIs(observation[0]["stream"],child.stdout)
            self.assertIsNone(observation[0]["readError"])

    def test_actual_shutdown_writes_original_binary_protocol_once(self):
        module=self.load(); writes=[]
        class Input:
            def write(self,value): writes.append(value)
            def flush(self): pass
        child=SimpleNamespace(stdin=Input()); observation=[]
        self.assertTrue(module.request_owned_shutdown(child,observation))
        self.assertTrue(module.request_owned_shutdown(child,observation))
        self.assertEqual(writes,[b"close\n"])
        self.assertIs(observation[0]["operations"][0]["subject"],child.stdin)

if __name__=="__main__": unittest.main()
