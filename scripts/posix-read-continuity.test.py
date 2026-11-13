"""Actual compatibility Terminal.read observations; all cases UNRUN."""
import importlib.util
from pathlib import Path
import sys
import types
import unittest
from contextlib import ExitStack
from unittest.mock import patch, mock_open

class PosixReadContinuity(unittest.TestCase):
    def load(self,name):
        spec=importlib.util.spec_from_file_location("posix_read_continuity",Path(__file__).with_name(name))
        module=importlib.util.module_from_spec(spec)
        stubs={key:types.ModuleType(key) for key in ("fcntl","pty","termios") if key not in sys.modules}
        with patch.dict(sys.modules,stubs): spec.loader.exec_module(module)
        return module

    def test_actual_read_keeps_original_bytes_and_separates_error_from_empty_raw(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            module=self.load(name); terminal=module.Terminal.__new__(module.Terminal)
            terminal.master=17; terminal.text=""; terminal.read_observations=[]
            raw=bytes(bytearray(b"useful\xff PRIVATE raw"))
            with patch.object(module.select,"select",return_value=([17],[],[])), patch.object(module.os,"read",return_value=raw): terminal.read()
            first=terminal.read_observations[0]
            self.assertIs(first["raw"],raw); self.assertEqual(first["descriptor"],17)
            self.assertFalse(first["emptyRaw"]); self.assertIsNone(first["original"])
            self.assertEqual(terminal.text,raw.decode("utf-8","replace"))
            for original in (OSError("PRIVATE read"),RuntimeError("PRIVATE opaque read")):
                before=terminal.text
                with patch.object(module.select,"select",return_value=([17],[],[])), patch.object(module.os,"read",side_effect=original):
                    if isinstance(original,OSError): terminal.read()
                    else:
                        with self.assertRaises(RuntimeError) as caught: terminal.read()
                        self.assertIs(caught.exception,original)
                item=terminal.read_observations[-1]
                self.assertIs(item["original"],original); self.assertIsNone(item["raw"])
                self.assertFalse(item["emptyRaw"]); self.assertEqual(terminal.text,before)
            with patch.object(module.select,"select",return_value=([17],[],[])), patch.object(module.os,"read",return_value=b""): terminal.read()
            last=terminal.read_observations[-1]
            self.assertEqual(last["raw"],b""); self.assertTrue(last["emptyRaw"]); self.assertIsNone(last["original"])

    def test_actual_shutdown_retains_originals_and_preserves_useful_eof_fallback(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            module=self.load(name)
            for failed_stage in (None,"write","flush","close","wait"):
                events=[]; original=OSError("PRIVATE shutdown original")
                class Stream:
                    __slots__=()
                    def write(self,value):
                        events.append("write"); self_case.assertEqual(value,"close\n")
                        if failed_stage=="write": raise original
                        return len(value)
                    def flush(self):
                        events.append("flush")
                        if failed_stage=="flush": raise original
                    def close(self):
                        events.append("close")
                        if failed_stage=="close": raise original
                class Runtime:
                    __slots__=("stdin","returncode")
                    def __init__(self): self.stdin=Stream(); self.returncode=None
                    def poll(self): events.append("poll"); return None
                    def wait(self):
                        events.append("wait")
                        if failed_stage=="wait": raise original
                        self.returncode=0; return 0
                self_case=self; runtime=Runtime(); observations=[]
                if failed_stage=="wait":
                    with self.assertRaises(OSError) as caught: module.stop_owned_runtime(runtime,observations)
                    self.assertIs(caught.exception,original)
                else:
                    result=module.stop_owned_runtime(runtime,observations)
                    self.assertEqual(result,("terminal_exited_zero",failed_stage not in ("write","flush")))
                    self.assertNotIn("PRIVATE",repr(result))
                self.assertEqual(events,["poll","write"]+([] if failed_stage=="write" else ["flush"])+["close","wait"])
                self.assertEqual(len(observations),1); item=observations[0]
                self.assertIs(item["runtime"],runtime); self.assertIs(item["stdin"],runtime.stdin)
                failures=[operation for operation in item["operations"] if operation["original"] is not None]
                self.assertEqual(len(failures),0 if failed_stage is None else 1)
                if failures:
                    self.assertIs(failures[0]["original"],original)
                    self.assertEqual(failures[0]["stage"],failed_stage)
                    self.assertTrue(failures[0]["attempted"]); self.assertFalse(failures[0]["completed"])
                self.assertEqual([operation["stage"] for operation in item["operations"]],events[:1]+["stdin"]+events[1:]+([] if failed_stage=="wait" else ["returncode"]))

    def test_actual_main_preserves_primary_and_refuses_failed_finalization_after_peer_attempt(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            for body_failed,cleanup_failed,persist_failed in ((True,True,True),(False,True,False),(False,False,True),(False,False,False),(False,"descriptor",False),(False,"descriptor-dual",False),(False,"classification",False),(False,"primary-persist",False),(False,"raw-conversion",False)):
                module=self.load(name); events=[]; receipts=[]; observation={}; descriptor_closes=[]
                primary=RuntimeError("PRIVATE body original")
                cleanup_original=OSError("PRIVATE cleanup original")
                persistence_original=ValueError("PRIVATE recovery receipt original")
                class Stream:
                    def write(self,value): events.append("write"); return len(value)
                    def flush(self): events.append("flush")
                    def close(self): events.append("close")
                runtime=types.SimpleNamespace(pid=41,tui20_birth={"birth":"actual controlled"},stdin=Stream(),returncode=None)
                runtime.poll=lambda:runtime.returncode
                def waited(): events.append("wait"); runtime.returncode=0; return 0
                runtime.wait=waited
                args=types.SimpleNamespace(root="owned-source-fixture",executable="controlled-candidate",source_commit="a"*40,core_commit="b"*40,runtime_script="controlled-runtime",node="controlled-node",external_runtime=False,adverse_controller_crash=True,adverse_owner_death=False,controller_pid=1,recovery_self_test=False,recovery_owner_self_test=False,core_url=None,jwks_url=None,invalid_ready_receipt=False,inject_finalization_cleanup_failure=False,runtime_ready_mode=None,shutdown_pipe_failure=False)
                ready={"coreCommit":args.core_commit,"runtimePathReceipt":{"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True},"url":"http://controlled","token":"PRIVATE token","deniedToken":"PRIVATE denied","jwksPort":1234}
                def start(_args,retain):
                    retain(runtime)
                    if body_failed: raise primary
                    return runtime,ready
                child=types.SimpleNamespace(pid=42,tui20_birth={},returncode=None,reaped_unowned=False)
                child.poll=lambda:child.returncode
                ActualTerminal=module.Terminal
                class Terminal:
                    recover_raw_pid_until_observed=ActualTerminal.recover_raw_pid_until_observed
                    def __init__(self,*_args):
                        self.child=child; self.exit_reason=None; self.label="native"; module.OWNED_TERMINALS.append(self)
                        if cleanup_failed=="raw-conversion":
                            self.child=None; self.acquisition={"pid":123,"originals":[primary],"descriptors":{key:{"descriptor":fd,"attempted":False,"completed":False,"original":None} for key,fd in (("slave",21),("master",22))}}
                            raise primary
                    def wait(self,*_args): pass
                    def read(self): pass
                    def close(self): child.returncode=0; self.exit_reason="terminal_exited_zero"; return self.exit_reason
                cleanup_original_writer=OSError("PRIVATE cleanup receipt original")
                primary_writer_original=OSError("PRIVATE primary receipt original")
                def close_descriptor(descriptor):
                    descriptor_closes.append(descriptor)
                    if descriptor==18 and cleanup_failed!="primary-persist": raise cleanup_original
                actual_finalize=module.finalize_native_outcome
                actual_persist_primary=module.persist_primary_outcome
                def finalizer(*_args,**_kwargs):
                    events.append("finalizer")
                    if cleanup_failed is True: raise cleanup_original
                    if cleanup_failed in ("descriptor","descriptor-dual","primary-persist"): return actual_finalize(*_args,writer=writer,**_kwargs)
                    if cleanup_failed=="classification": return {}
                    return {"outcome":"succeeded","closedReason":"controlled","recoveryRetained":False}
                def writer(path,value):
                    receipts.append((path,value))
                    if path.endswith("native-exit-receipt.json") and cleanup_failed=="primary-persist": raise primary_writer_original
                    if path.endswith("owner-runtime-recovery.json") and persist_failed: raise persistence_original
                    if path.endswith("native-cleanup-receipt.json") and cleanup_failed=="descriptor-dual": raise cleanup_original_writer
                with ExitStack() as stack:
                    for target,attribute,value in ((module.argparse.ArgumentParser,"parse_args",lambda _self:args),(module,"start_owned_runtime",start),(module,"finalize_native_outcome",finalizer),(module,"write_json",writer),(module,"owned_runtime_birth_absent",lambda _runtime:False),(module,"process_birth",lambda _pid:{}),(module,"hold_candidate",lambda *_args:(17,{"sourceCommit":args.source_commit,"binarySHA256":"c"*64},(1,2))),(module,"bound_execution",lambda *_args:(18,None,{})),(module,"write_owner_birth",lambda *_args:None),(module,"Terminal",Terminal),(module,"detail",lambda *_args:None),(module,"controller_failed",lambda *_args:True),(module,"request",lambda *_args:{}),(module.urllib.request,"urlopen",lambda *_args,**_kwargs:types.SimpleNamespace(read=lambda:b"")),(module.hashlib,"file_digest",lambda *_args:types.SimpleNamespace(hexdigest=lambda:"c"*64))):
                        stack.enter_context(patch.object(target,attribute,value))
                    stack.enter_context(patch("builtins.open",mock_open(read_data=b"controlled")))
                    if cleanup_failed in ("descriptor","descriptor-dual","primary-persist"): stack.enter_context(patch.object(module.os,"close",side_effect=close_descriptor))
                    if cleanup_failed=="raw-conversion":
                        stack.enter_context(patch.object(module,"persist_primary_outcome",lambda root,value:actual_persist_primary(root,value,writer=writer)))
                        stack.enter_context(patch.object(module.os,"WNOHANG",1,create=True))
                        raw_wait=stack.enter_context(patch.object(module.os,"waitpid",return_value=(123,512),create=True))
                        stack.enter_context(patch.object(module.os,"waitstatus_to_exitcode",side_effect=cleanup_original,create=True))
                        stack.enter_context(patch.object(module.os,"close",side_effect=descriptor_closes.append))
                    stack.enter_context(patch.object(module.sys,"platform","linux"))
                    if hasattr(module,"strict_load"): stack.enter_context(patch.object(module,"strict_load",return_value=ready))
                    else: stack.enter_context(patch.object(module.json,"load",return_value=ready))
                    expected=primary if body_failed or cleanup_failed=="raw-conversion" else primary_writer_original if cleanup_failed=="primary-persist" else cleanup_original if cleanup_failed else persistence_original if persist_failed else None
                    if cleanup_failed=="classification":
                        with self.assertRaises(KeyError) as caught: module.main(observation)
                        self.assertIs(caught.exception,observation["finalizationOriginals"][0]["original"])
                    elif expected is None: self.assertIsNone(module.main(observation))
                    else:
                        with self.assertRaises(type(expected)) as caught: module.main(observation)
                        self.assertIs(caught.exception,expected)
                    if cleanup_failed=="raw-conversion": raw_wait.assert_called_once_with(123,1)
                self.assertEqual(events,["finalizer","write","flush","close","wait"])
                self.assertEqual(observation["bodyPrimaryPresent"],body_failed or cleanup_failed=="raw-conversion")
                if body_failed: self.assertIs(observation["bodyPrimary"],primary)
                originals=[item["original"] for item in observation["finalizationOriginals"]]
                if cleanup_failed=="raw-conversion":
                    self.assertEqual(originals,[cleanup_original]); self.assertEqual(descriptor_closes,[21,22])
                    self.assertIs(observation["bodyPrimary"],primary)
                elif cleanup_failed=="classification":
                    self.assertEqual(len(originals),1); self.assertIsInstance(originals[0],KeyError)
                elif cleanup_failed=="primary-persist":
                    self.assertEqual(originals,[primary_writer_original])
                elif cleanup_failed=="descriptor-dual":
                    self.assertEqual(len(originals),2); self.assertIs(originals[0],cleanup_original)
                    self.assertIsInstance(originals[1],module.CleanupReceiptPersistenceFailure)
                    self.assertIs(originals[1].__cause__,cleanup_original_writer)
                else: self.assertEqual(originals,([cleanup_original] if cleanup_failed else [])+([persistence_original] if persist_failed else []))
                if cleanup_failed in ("descriptor","descriptor-dual","primary-persist"):
                    operations=observation["executionCleanup"][0]["operations"]
                    self.assertEqual(descriptor_closes,[18,17]); self.assertEqual(len(operations),2)
                    self.assertIs(operations[0]["original"],None if cleanup_failed=="primary-persist" else cleanup_original)
                    self.assertEqual(operations[0]["stage"],"launch_close"); self.assertEqual(operations[0]["completed"],cleanup_failed=="primary-persist")
                    self.assertEqual(operations[1]["stage"],"held_close"); self.assertTrue(operations[1]["completed"])
                    self.assertTrue(all(item["attempted"] for item in operations))
                    self.assertTrue(any(value.get("closedReason")==("sealed_descriptor_closed" if cleanup_failed=="primary-persist" else "descriptor_close_failed") for _,value in receipts))
                self.assertIs(observation["runtimeShutdown"][0]["runtime"],runtime)
                self.assertNotIn("PRIVATE",repr(receipts))

    def test_actual_descriptor_cleanup_attempts_each_owned_peer_once_and_retains_unresolved(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            for mode in ("useful","both-fail","opaque-first","same-descriptor"):
                module=self.load(name); observations=[]; calls=[]
                first=RuntimeError("PRIVATE opaque close") if mode=="opaque-first" else OSError("PRIVATE launch close")
                second=OSError("PRIVATE held close")
                def close(descriptor):
                    calls.append(descriptor)
                    if mode in ("both-fail","opaque-first"): raise first if descriptor==18 else second
                with patch.object(module.os,"close",side_effect=close):
                    if mode=="opaque-first":
                        with self.assertRaises(RuntimeError) as caught: module.cleanup_execution(18,None,17,observations=observations)
                        self.assertIs(caught.exception,first)
                    else: result=module.cleanup_execution(18,None,18 if mode=="same-descriptor" else 17,observations=observations)
                self.assertEqual(calls,[18] if mode=="same-descriptor" else [18,17])
                operations=observations[0]["operations"]
                self.assertEqual(len(operations),len(calls)); self.assertTrue(all(item["attempted"] for item in operations))
                if mode in ("both-fail","opaque-first"):
                    self.assertIs(operations[0]["original"],first); self.assertIs(operations[1]["original"],second)
                    self.assertTrue(all(not item["completed"] for item in operations))
                    if mode=="both-fail": self.assertEqual(result,{"outcome":"failed","closedReason":"descriptor_close_failed","recoveryRetained":True})
                else:
                    self.assertTrue(all(item["completed"] and item["original"] is None for item in operations))
                    self.assertEqual(result,{"outcome":"succeeded","closedReason":"sealed_descriptor_closed","recoveryRetained":False})
                if mode!="opaque-first": self.assertNotIn("PRIVATE",repr(result))

    def test_actual_acquisition_primary_survives_once_cleanup_and_useful_return(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            module=self.load(name)
            for failed in (False,True):
                original=RuntimeError("PRIVATE acquisition original"); secondary=OSError("PRIVATE acquisition close")
                observations=[]; calls=[]; info=types.SimpleNamespace(st_mode=module.stat.S_IFREG,st_dev=1,st_ino=2)
                def digest(_fd):
                    if failed: raise original
                    return "c"*64
                def close(fd): calls.append(fd); raise secondary
                with patch.object(module.os,"name","posix"), patch.object(module.os,"lstat",return_value=info), patch.object(module.os,"open",return_value=17), patch.object(module.os,"fstat",return_value=info), patch.object(module,"sha256_fd",side_effect=digest), patch.object(module.os,"close",side_effect=close):
                    if failed:
                        with self.assertRaises(RuntimeError) as caught: module.hold_candidate("controlled","a"*40,observations)
                        self.assertIs(caught.exception,original)
                    else: self.assertEqual(module.hold_candidate("controlled","a"*40,observations),(17,{"sourceCommit":"a"*40,"binarySHA256":"c"*64},(1,2)))
                self.assertEqual(calls,[17] if failed else [])
                if failed:
                    self.assertIs(observations[0]["original"],original); self.assertIs(observations[0]["closeOriginal"],secondary)
                    self.assertTrue(observations[0]["closeAttempted"]); self.assertFalse(observations[0]["closeCompleted"])
                else: self.assertEqual(observations,[])
            original=ValueError("PRIVATE sealed read"); secondary=OSError("PRIVATE sealed close"); observations=[]; calls=[]
            def close(fd): calls.append(fd); raise secondary
            with ExitStack() as stack:
                stack.enter_context(patch.object(module.os,"memfd_create",return_value=18,create=True))
                stack.enter_context(patch.object(module.os,"MFD_ALLOW_SEALING",1,create=True))
                for flag in ("F_ADD_SEALS","F_GET_SEALS","F_SEAL_WRITE","F_SEAL_GROW","F_SEAL_SHRINK","F_SEAL_SEAL"):
                    stack.enter_context(patch.object(module.fcntl,flag,1,create=True))
                stack.enter_context(patch.object(module.os,"lseek",return_value=0))
                stack.enter_context(patch.object(module.os,"read",side_effect=original))
                stack.enter_context(patch.object(module.os,"close",side_effect=close))
                with self.assertRaises(ValueError) as caught: module.linux_sealed_execution(17,{"binarySHA256":"c"*64},observations)
                self.assertIs(caught.exception,original)
            self.assertEqual(calls,[18]); self.assertIs(observations[0]["original"],original)
            self.assertIs(observations[0]["closeOriginal"],secondary)

    def test_actual_primary_persistence_failure_still_attempts_descriptor_cleanup(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            for secondary_failed in (False,True):
                module=self.load(name); primary=OSError("PRIVATE primary persistence"); secondary=ValueError("PRIVATE cleanup persistence")
                originals=[]; cleanup=[]; events=[]; handoff={"present":False}
                def writer(path,value):
                    events.append("primary" if path.endswith("native-exit-receipt.json") else "cleanup_receipt")
                    if path.endswith("native-exit-receipt.json"): raise primary
                    if secondary_failed: raise secondary
                def owned_cleanup(*args): return module.cleanup_execution(*args,observations=cleanup)
                def close(fd): events.append(fd)
                with patch.object(module.os,"close",side_effect=close):
                    with self.assertRaises(OSError) as caught:
                        module.finalize_native_outcome("controlled",{"outcome":"succeeded"},18,None,17,cleanup=owned_cleanup,writer=writer,originals=originals,handoff=handoff)
                    self.assertIs(caught.exception,primary)
                self.assertEqual(events,["primary",18,17,"cleanup_receipt"])
                self.assertIs(originals[0]["original"],primary); self.assertTrue(handoff["present"])
                self.assertTrue(all(item["completed"] for item in cleanup[0]["operations"]))
                self.assertEqual(len(originals),2 if secondary_failed else 1)
                if secondary_failed: self.assertIs(originals[1]["original"],secondary)
    def test_actual_sealed_useful_refusal_preserves_expected_original(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            module=self.load(name); original=OSError("PRIVATE expected write refusal"); observations=[]; closes=[]
            with ExitStack() as stack:
                stack.enter_context(patch.object(module.os,"memfd_create",return_value=18,create=True))
                stack.enter_context(patch.object(module.os,"MFD_ALLOW_SEALING",1,create=True))
                for flag in ("F_ADD_SEALS","F_GET_SEALS","F_SEAL_WRITE","F_SEAL_GROW","F_SEAL_SHRINK","F_SEAL_SEAL"):
                    stack.enter_context(patch.object(module.fcntl,flag,1,create=True))
                stack.enter_context(patch.object(module.fcntl,"fcntl",return_value=1,create=True))
                stack.enter_context(patch.object(module.os,"lseek",return_value=0))
                stack.enter_context(patch.object(module.os,"read",return_value=b""))
                stack.enter_context(patch.object(module,"sha256_fd",return_value="c"*64))
                stack.enter_context(patch.object(module.os,"pwrite",side_effect=original,create=True))
                stack.enter_context(patch.object(module.os,"close",side_effect=lambda fd:closes.append(fd)))
                with patch.object(module,"sys_platform",return_value="linux"):
                    descriptor,protected,receipt=module.bound_execution(17,{"binarySHA256":"c"*64},"controlled",observations)
            self.assertEqual(descriptor,18); self.assertIsNone(protected); self.assertEqual(closes,[])
            self.assertTrue(receipt["inPlaceWriteDenied"]); self.assertNotIn("PRIVATE",repr(receipt))
            self.assertEqual(len(observations),1); self.assertIs(observations[0]["original"],original)
            self.assertTrue(observations[0]["expected"]); self.assertEqual(observations[0]["descriptor"],18)
    def test_actual_recovery_sealed_acquisition_keeps_primary_and_once_close(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            for failed,close_failed in ((False,False),(True,False),(True,True)):
                module=self.load(name); primary=ValueError("PRIVATE recovery acquisition"); secondary=OSError("PRIVATE recovery close")
                observations=[]; closes=[]
                def close(fd):
                    closes.append(fd)
                    if close_failed: raise secondary
                with ExitStack() as stack:
                    stack.enter_context(patch.object(module,"sys_platform",return_value="linux"))
                    stack.enter_context(patch.object(module.os,"memfd_create",return_value=18,create=True))
                    stack.enter_context(patch.object(module.os,"MFD_ALLOW_SEALING",1,create=True))
                    for flag in ("F_ADD_SEALS","F_GET_SEALS","F_SEAL_WRITE","F_SEAL_GROW","F_SEAL_SHRINK","F_SEAL_SEAL"):
                        stack.enter_context(patch.object(module.fcntl,flag,1,create=True))
                    stack.enter_context(patch.object(module.fcntl,"fcntl",return_value=1,create=True))
                    stack.enter_context(patch.object(module.os,"write",side_effect=primary if failed else None))
                    stack.enter_context(patch.object(module.os,"close",side_effect=close))
                    if failed:
                        with self.assertRaises(ValueError) as caught: module.recovery_immutable_execution(observations)
                        self.assertIs(caught.exception,primary)
                    else: self.assertEqual(module.recovery_immutable_execution(observations),(18,1))
                self.assertEqual(closes,[18] if failed else [])
                if failed:
                    self.assertIs(observations[0]["original"],primary)
                    self.assertTrue(observations[0]["closeAttempted"])
                    self.assertEqual(observations[0]["closeCompleted"],not close_failed)
                    self.assertIs(observations[0]["closeOriginal"],secondary if close_failed else None)
                else: self.assertEqual(observations,[])
    def test_actual_linux_replacement_probe_keeps_primary_and_independent_cleanup(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            for mode in ("useful","mutation_dual","terminal_dual","replacement_cleanup","file_dual"):
                module=self.load(name); ActualTerminal=module.Terminal; observations=[]; events=[]; primary=ValueError("PRIVATE first"); secondary=OSError("PRIVATE second")
                # This is a virtual namespace on either host. The actual Linux
                # control body uses os.path.join, so its fixture keys must use
                # the same host algebra even when native operations are mocked.
                executable=module.os.path.join("/owned","candidate")
                directory=module.os.path.dirname(executable)
                alias=module.os.path.join(directory,".tui20-held-original")
                replacement=module.os.path.join(directory,".tui20-untrusted-replacement"); staged=replacement+".link"
                regular=types.SimpleNamespace(st_mode=module.stat.S_IFREG,st_dev=1,st_ino=2)
                foreign=types.SimpleNamespace(st_mode=module.stat.S_IFREG,st_dev=1,st_ino=3)
                symlink=types.SimpleNamespace(st_mode=module.stat.S_IFLNK,st_dev=1,st_ino=4)
                paths={executable:regular}; writes=[]; terminal_count=[0]
                def link(source,destination): paths[destination]=paths[source]
                def replace(source,destination):
                    events.append(("replace",source,destination)); paths[destination]=paths.pop(source)
                def unlink(target):
                    events.append(("unlink",target))
                    if target==replacement and mode=="replacement_cleanup": raise secondary
                    paths.pop(target,None)
                def open_file(*args,**kwargs):
                    paths[replacement]=foreign; stream=mock_open()(*args,**kwargs)
                    if mode=="file_dual": stream.write.side_effect=primary; stream.close.side_effect=secondary
                    return stream
                def pwrite(fd,raw,offset):
                    writes.append(raw); events.append(("pwrite",raw))
                    if mode=="mutation_dual": raise primary if len(writes)==1 else secondary
                    return len(raw)
                def close(fd): events.append(("close",fd))
                class ControlledTerminal:
                    __slots__=("text","number")
                    def __init__(self,*args): self.text=module.SAFE_CONFIGURATION_DIAGNOSTIC; terminal_count[0]+=1; self.number=terminal_count[0]
                    def wait(self,needles,timeout):
                        events.append(("wait",self.number,timeout))
                        if (mode=="terminal_dual" and self.number==1) or (mode=="replacement_cleanup" and self.number==2): raise primary
                    def close(self,expected):
                        events.append(("terminal_close",self.number,expected))
                        if mode=="terminal_dual" and self.number==1: raise secondary
                        if expected!=2: raise AssertionError("actual expected exit code changed")
                        return module.terminal_exit_reason(2)
                with ExitStack() as stack:
                    for target,attribute,value in ((module.sys,"platform","linux"),(module,"Terminal",ControlledTerminal),(module.os.path,"lexists",lambda target:target in paths),(module.os,"link",link),(module.os,"replace",replace),(module.os,"unlink",unlink),(module.os,"lstat",lambda target:paths[target]),(module.os,"fstat",lambda fd:regular),(module.os,"chmod",lambda *args:None),(module.os,"symlink",lambda target,linkpath:paths.__setitem__(linkpath,symlink)),(module.os,"open",lambda *args:18),(module.os,"pread",lambda *args:b"a"),(module.os,"pwrite",pwrite),(module.os,"fsync",lambda fd:events.append(("sync",fd))),(module.os,"close",close)):
                        stack.enter_context(patch.object(target,attribute,value,create=target is module.os and attribute in ("pread","pwrite")))
                    stack.enter_context(patch("builtins.open",side_effect=open_file))
                    stack.enter_context(patch.object(module,"sha256_fd",side_effect=["mutated","original"]))
                    if mode=="useful":
                        result=module.bound_replacement_probe(17,16,executable,(1,2),{"binarySHA256":"original"},"missing",{},observations)
                        self.assertEqual([item["attack"] for item in result],["rename","reparse","inplace-content-mutation"])
                        self.assertEqual([item["terminalExit"] for item in result],["terminal_exit_code_2"]*3)
                        actual=ActualTerminal.__new__(ActualTerminal); actual.master=19; actual.exit_reason=None
                        actual.child=types.SimpleNamespace(poll=lambda:2,reaped_unowned=False,wait=lambda timeout:2)
                        self.assertEqual(actual.close(2),"terminal_exit_code_2")
                        self.assertEqual(actual.exit_reason,"terminal_exit_code_2")
                        self.assertEqual(events.count(("close",19)),1)
                    else:
                        with self.assertRaises(ValueError) as caught: module.bound_replacement_probe(17,16,executable,(1,2),{"binarySHA256":"original"},"missing",{},observations)
                        self.assertIs(caught.exception,primary)
                owner=observations[0]
                self.assertIs(paths[executable],regular); self.assertNotIn(alias,paths)
                if mode=="useful": self.assertEqual(owner["originals"],[])
                else:
                    self.assertIs(owner["originals"][0]["original"],primary)
                    if mode in ("mutation_dual","terminal_dual","replacement_cleanup","file_dual"): self.assertIs(owner["originals"][1]["original"],secondary)
                if mode=="file_dual":
                    self.assertEqual(sum(item["stage"]=="replacement_close" for item in owner["operations"]),1)
                    self.assertFalse(next(item for item in owner["operations"] if item["stage"]=="replacement_close")["completed"])
                if mode in ("useful","mutation_dual"):
                    self.assertEqual(events.count(("close",18)),1)
                    self.assertEqual(len(writes),2)
                    self.assertTrue(next(item for item in owner["operations"] if item["stage"]=="modifier_close")["completed"])
                self.assertEqual(sum(item[0]=="terminal_close" for item in events),3 if mode=="useful" else (0 if mode=="file_dual" else (2 if mode in ("mutation_dual","replacement_cleanup") else 1)))
    def test_actual_linux_terminal_acquisition_keeps_fork_primary_and_child_custody(self):
        for name in ("tui20-native-posix-five-action.py","tui28-native-linux-five-action.py"):
            for mode in ("useful","fork_dual","slave","birth","wrapper"):
                module=self.load(name); original=RuntimeError("PRIVATE acquisition"); secondary=OSError("PRIVATE close")
                closed=[]
                ActualBoundProcess=module.BoundProcess
                def wrap(pid):
                    if mode=="wrapper": raise original
                    return ActualBoundProcess(pid)
                def fork():
                    self.assertEqual(len(module.OWNED_TERMINALS),1)
                    self.assertIsNone(module.OWNED_TERMINALS[0].child)
                    if mode=="fork_dual": raise original
                    return 123
                def close(fd):
                    closed.append(fd)
                    if mode=="fork_dual" and fd==18: raise secondary
                    if mode=="slave": raise original
                with patch.object(module,"BoundProcess",side_effect=wrap), patch.object(module,"sys_platform",return_value="linux"), patch.object(module.pty,"openpty",return_value=(17,18),create=True), patch.object(module.os,"fork",side_effect=fork,create=True), patch.object(module.os,"close",side_effect=close), patch.object(module,"process_birth",side_effect=original if mode=="birth" else lambda pid:"birth"):
                    if mode=="useful": terminal=module.Terminal(19,"candidate","profile",{})
                    else:
                        with self.assertRaises(RuntimeError) as caught: module.Terminal(19,"candidate","profile",{})
                        self.assertIs(caught.exception,original)
                        terminal=module.OWNED_TERMINALS[0]
                self.assertEqual(len(module.OWNED_TERMINALS),1)
                if mode=="fork_dual":
                    self.assertIsNone(terminal.child); self.assertEqual(closed,[18,17])
                    self.assertEqual(terminal.acquisition["originals"],[original,secondary])
                    self.assertFalse(terminal.acquisition["descriptors"]["slave"]["completed"])
                    self.assertTrue(terminal.acquisition["descriptors"]["master"]["completed"])
                elif mode=="wrapper":
                    self.assertIsNone(terminal.child); self.assertEqual(terminal.acquisition["pid"],123)
                    self.assertEqual(closed,[]); self.assertEqual(terminal.acquisition["originals"],[original])
                    wait_original=OSError("PRIVATE raw wait")
                    close_original=OSError("PRIVATE raw slave close")
                    recovery_closes=[]
                    def recovered_close(fd):
                        recovery_closes.append(fd)
                        if fd==18: raise close_original
                    with patch.object(module.select,"select",return_value=([],[],[])), patch.object(module.os,"WNOHANG",1,create=True), patch.object(module.os,"waitpid",side_effect=[wait_original,(0,0),(123,512)],create=True) as raw_wait, patch.object(module.os,"waitstatus_to_exitcode",return_value=2,create=True), patch.object(module.time,"sleep"), patch.object(module.os,"close",side_effect=recovered_close):
                        result=terminal.recover_raw_pid_until_observed()
                    self.assertEqual([item.args for item in raw_wait.call_args_list],[(123,1),(123,1),(123,1)])
                    self.assertEqual(result["outcome"],"terminal_exit_code_2")
                    self.assertTrue(result["observed"]); self.assertEqual(recovery_closes,[18,17])
                    self.assertEqual(terminal.acquisition["originals"],[original,wait_original,close_original])
                    self.assertIs(terminal.acquisition["waitObservations"][0]["original"],wait_original)
                    self.assertEqual(terminal.acquisition["exitStatus"],512)
                else:
                    self.assertEqual(terminal.child.pid,123); self.assertEqual(closed,[18])
                    self.assertFalse(terminal.acquisition["descriptors"]["master"]["attempted"])
                    if mode=="useful": self.assertEqual(terminal.child.tui20_birth,"birth")
                    else: self.assertEqual(terminal.acquisition["originals"],[original])
    def test_actual_linux_waits_drain_original_pty_output_without_renewing_phase_deadline(self):
        for recover in (False,True):
            module=self.load("tui28-native-linux-five-action.py")
            terminal=module.Terminal.__new__(module.Terminal); terminal.master=17; terminal.text=""; terminal.read_observations=[]
            child=module.retain_terminal_child(terminal,123); events=[]
            raw=[b"first\xff",b"last\r\n"]; pending=iter(raw); statuses=iter(((0,0),(123,0)))
            def read(_fd,_size): events.append("read"); return next(pending)
            def waitpid(_pid,_flags): events.append("waitpid"); return next(statuses)
            with patch.object(module.select,"select",return_value=([17],[],[])),patch.object(module.os,"read",side_effect=read),patch.object(module.os,"waitpid",side_effect=waitpid,create=True),patch.object(module.os,"WNOHANG",1,create=True),patch.object(module.os,"waitstatus_to_exitcode",return_value=0,create=True),patch.object(module.time,"sleep"),patch.object(module.time,"monotonic",side_effect=[0,0,1,1]) as clock:
                result=child.wait_until_observed() if recover else child.wait(10)
            self.assertEqual(result,0); self.assertEqual(events,["read","waitpid","read","waitpid"])
            self.assertEqual([item["raw"] for item in terminal.read_observations],raw)
            self.assertEqual(terminal.text,"first\ufffdlast\r\n")
            self.assertIs(child.tui28_terminal,terminal)
            self.assertEqual(clock.call_count,0 if recover else 4)

    def test_actual_linux_late_exit_remains_observed_without_phase_success(self):
        module=self.load("tui28-native-linux-five-action.py"); child=module.BoundProcess(123); events=[]
        child.tui28_terminal=types.SimpleNamespace(read=lambda:events.append("read"))
        with patch.object(module.os,"waitpid",return_value=(123,0),create=True),patch.object(module.os,"WNOHANG",1,create=True),patch.object(module.os,"waitstatus_to_exitcode",return_value=0,create=True),patch.object(module.time,"monotonic",side_effect=[0,0,11]):
            with self.assertRaises(module.subprocess.TimeoutExpired): child.wait(10)
        self.assertEqual(child.returncode,0); self.assertEqual(events,["read"])

if __name__=="__main__": unittest.main()
