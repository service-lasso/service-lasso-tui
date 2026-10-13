"""Durable POSIX custody parent for the real Core/JWKS and native PTY owner."""
import argparse, hashlib, json, os, select, subprocess, sys, time, urllib.request, queue, threading
from tui28_json import strict_load, strict_loads

def write_json(path, value):
    with open(path, "w", encoding="utf-8", newline="") as out:
        json.dump(value, out, separators=(",", ":")); out.flush(); os.fsync(out.fileno())

def write_private(root, phase, value):
    write_json(os.path.join(root, "owner-private-%s.json" % phase), {"classification":"owner-private", "phase":phase, **value})

def write_public(root, source_commit, binary_sha256, result, actions_passed, runtime_closed):
    write_json(os.path.join(root, "native-public-projection.json"), {"schemaVersion":1,"kind":"tui20-native-public-result","sourceCommit":source_commit,"binarySHA256":binary_sha256,"result":result,"actionsPassed":actions_passed,"ownedRuntimeClosed":runtime_closed})

def process_birth(pid):
    if sys.platform == "win32":
        from tui28_windows_identity import process_birth as windows_birth
        return windows_birth(pid)
    if sys.platform == "linux":
        with open("/proc/%d/stat" % pid, encoding="utf-8") as stream:
            return {"platform":"linux", "startTimeTicks":stream.read().rsplit(") ", 1)[1].split()[19]}
    if sys.platform == "darwin":
        value=subprocess.check_output(["/bin/ps","-o","lstart=","-p",str(pid)], text=True).strip()
        if value: return {"platform":"darwin", "startTime":value}
        raise ProcessLookupError("Darwin process absent")
    raise RuntimeError("POSIX process birth unavailable")

def sha256(path):
    digest=hashlib.sha256()
    with open(path,"rb") as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b""): digest.update(chunk)
    return digest.hexdigest()

def child_alive(pid, birth):
    try: return process_birth(pid)==birth
    except (FileNotFoundError, ProcessLookupError, subprocess.CalledProcessError): return False

def birth_absent(pid,birth):
    if birth is None: return False
    try: return not child_alive(pid,birth)
    except (OSError,RuntimeError,ValueError): return False

class RuntimeAcquisitionFailure(RuntimeError):
    def __init__(self, reason): super().__init__(reason); self.reason=reason

def owned_streams(process, observation):
    for previous in observation:
        if previous.get("process") is process: return previous
    owned={"process":process,"closed":{"stdin":False,"stdout":False,"stderr":False},"streams":[],"operations":[]}
    observation.append(owned)
    return owned

def owned_stream(owned, name, stream):
    for previous in owned["streams"]:
        if previous["name"]==name: return previous
    item={"name":name,"stream":stream,"readAttempted":False,"closeAttempted":False,"raw":None,"readError":None,"closeError":None}
    owned["streams"].append(item)
    return item

def close_owned_stdin(process, observation):
    owned=owned_streams(process,observation)
    if process.stdin is None: return
    item=owned_stream(owned,"stdin",process.stdin)
    if item["closeAttempted"]: return
    item["closeAttempted"]=True
    try:
        process.stdin.close()
        owned["closed"]["stdin"]=True
    except Exception as original: item["closeError"]=original

def close_streams(process, observation=None):
    """Drain only in memory, then close every observer-owned pipe once."""
    if observation is None: observation=[]
    owned=owned_streams(process,observation); closed=owned["closed"]
    for name,stream in (("stdin",process.stdin),("stdout",process.stdout),("stderr",process.stderr)):
        if stream is None: continue
        item=owned_stream(owned,name,stream)
        if item["closeAttempted"]: continue
        try:
            if name in ("stdout", "stderr"):
                item["readAttempted"]=True
                item["raw"]=stream.read()
        except Exception as original: item["readError"]=original
        try:
            item["closeAttempted"]=True
            stream.close()
            closed[name]=True
        except Exception as original: item["closeError"]=original
    return closed

def stream_failure(process, observation):
    return any(item["readError"] is not None or item["closeError"] is not None for owned in observation if owned.get("process") is process for item in owned["streams"])

def observe_owned_operation(process, observation, operation, subject, invoke):
    owned=owned_streams(process,observation)
    for item in owned["operations"]:
        if item["operation"]==operation and item.get("subject") is subject:
            if not item["completed"]: raise item["original"]
            return item["value"]
    item={"operation":operation,"subject":subject,"attempted":True,"completed":False,"value":None,"original":None}
    owned["operations"].append(item)
    try:
        item["value"]=invoke()
        item["completed"]=True
        return item["value"]
    except BaseException as original:
        item["original"]=original
        raise

def wait_owned(process, observation):
    return observe_owned_operation(process,observation,"wait",process,process.wait)

def request_owned_shutdown(runtime, observation):
    owned=owned_streams(runtime,observation)
    for item in owned["operations"]:
        if item["operation"]=="shutdown_write_flush": return item["completed"]
    item={"operation":"shutdown_write_flush","subject":runtime.stdin,"attempted":True,"completed":False,"original":None}
    owned["operations"].append(item)
    try:
        runtime.stdin.write(b"close\n"); runtime.stdin.flush()
        item["completed"]=True
    except Exception as original: item["original"]=original
    return item["completed"]

def close_runtime(runtime, observation=None):
    """Only the directly-owned Core/JWKS child is closed; no process sweep occurs."""
    if observation is None: observation=[]
    requested=request_owned_shutdown(runtime,observation)
    # EOF is the runtime's existing graceful close path if the write failed.
    close_owned_stdin(runtime,observation)
    wait_owned(runtime,observation)
    reader=getattr(runtime,"tui28_reader",None)
    if reader is not None: observe_owned_operation(runtime,observation,"ready_reader_join",reader,reader.join)
    closed=close_streams(runtime,observation)
    birth=getattr(runtime,"tui20_birth",None)
    absent=birth_absent(runtime.pid,birth)
    return {"runtimeChildExit":"terminal_exited_zero" if runtime.returncode==0 else "terminal_signaled" if runtime.returncode and runtime.returncode<0 else "terminal_exited_nonzero","shutdownRequested":requested,"actualExitObserved":True,"runtimeBirthAbsentObserved":absent,"runtimeChildStillLiveAfterClose":False if absent else None,"runtimeStdoutClosed":closed["stdout"],"runtimeStderrClosed":closed["stderr"]}

def start_runtime(args, retain, observation=None):
    if observation is None: observation=[]
    command=[args.node,args.runtime_script,"--root",args.root,"--core-commit",args.core_commit]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.runtime_ready_mode: command += ["--ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    runtime=subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=False)
    # Retain the actual child before birth/read/persistence can raise.
    retain(runtime)
    runtime.tui20_birth=process_birth(runtime.pid)
    lines=queue.Queue(maxsize=1)
    def read_ready():
        item={"stream":runtime.stdout,"raw":None,"readError":None}
        observation.append(item)
        try:
            item["raw"]=runtime.stdout.readline(1048577)
        except Exception as original: item["readError"]=original
        lines.put(item)
    reader=threading.Thread(target=read_ready); reader.start(); runtime.tui28_reader=reader
    try: read=lines.get(timeout=10)
    except queue.Empty: raise RuntimeAcquisitionFailure("runtime_ready_timeout")
    if read["readError"] is not None:
        raise RuntimeAcquisitionFailure("runtime_ready_invalid") from read["readError"]
    line=read["raw"]
    try:
        if not line or len(line)>1048576: raise ValueError("ready quota")
        ready=strict_loads(line)
    except Exception as error:
        raise RuntimeAcquisitionFailure("runtime_ready_invalid") from error
    if ready.get("event")!="ready" or not all(isinstance(ready.get(key),str) and ready[key] for key in ("url","token","deniedToken")) or not isinstance(ready.get("jwksPort"),int):
        raise RuntimeAcquisitionFailure("runtime_ready_invalid")
    return runtime,ready

def runtime_live(ready):
    with urllib.request.urlopen(ready["url"]+"/api/health",timeout=2) as response: strict_load(response)
    with urllib.request.urlopen("http://127.0.0.1:%d/jwks" % ready["jwksPort"],timeout=2) as response: response.read()

def observed_receipt(observation, phase, writer, *values):
    try: return writer(*values)
    except BaseException as original:
        observation["originals"].append({"phase":phase,"original":original})
        if observation["finalizing"] and not observation["primaryPresent"]:
            observation["primaryPresent"]=True; observation["primary"]=original
        raise

def _main_attempt(observation):
    def private(*values): return observed_receipt(observation,"private_receipt",write_private,*values)
    def public(*values): return observed_receipt(observation,"public_receipt",write_public,*values)
    parser=argparse.ArgumentParser(); parser.add_argument("--owner",required=True); parser.add_argument("--root",required=True)
    parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True)
    parser.add_argument("--runtime-script",required=True); parser.add_argument("--node",required=True); parser.add_argument("--adverse-controller-crash",action="store_true")
    parser.add_argument("--controller-pid",type=int); parser.add_argument("--invalid-ready-receipt",action="store_true"); parser.add_argument("--inject-finalization-cleanup-failure",action="store_true")
    parser.add_argument("--runtime-ready-mode",choices=("normal","timeout","eof")); parser.add_argument("--shutdown-pipe-failure",action="store_true"); parser.add_argument("--adverse-owner-death",action="store_true")
    args=parser.parse_args(); binary_sha256=sha256(args.executable)
    runtime=None; owner=None; owner_birth=None; result=None; lifecycle_failed=False; failure_reason="observer_acquisition_or_handoff_failed"
    stream_observation=observation["streams"]; ready_observation=observation["ready"]
    def retain(child):
        nonlocal runtime
        runtime=child
    try:
        runtime,ready=start_runtime(args,retain,ready_observation); runtime_birth=runtime.tui20_birth
        private(args.root,"initial",{"observerPID":os.getpid(),"observerBirth":process_birth(os.getpid()),"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256,"ownedCoreAndJwks":True})
        command=[sys.executable,args.owner,"--root",args.root,"--executable",args.executable,"--source-commit",args.source_commit,"--core-commit",args.core_commit,"--external-runtime"]
        if args.adverse_controller_crash: command += ["--adverse-controller-crash","--controller-pid",str(args.controller_pid)]
        if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
        if args.inject_finalization_cleanup_failure: command.append("--inject-finalization-cleanup-failure")
        if args.runtime_ready_mode: command += ["--runtime-ready-mode",args.runtime_ready_mode]
        if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
        if args.adverse_owner_death: command.append("--adverse-owner-death")
        ready["_binarySHA256"]=binary_sha256; ready["_runtimePID"]=runtime.pid; ready["_runtimeBirth"]=runtime_birth
        owner=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=False)
        owner_birth=process_birth(owner.pid)
        private(args.root,"witness",{"observerPID":os.getpid(),"recoveryOwnerPID":owner.pid,"recoveryOwnerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256,"ownerStarted":True})
        owner.stdin.write((json.dumps(ready,separators=(",",":"))+"\n").encode("utf-8")); close_owned_stdin(owner,stream_observation)
        status=wait_owned(owner,stream_observation); owner_closed=close_streams(owner,stream_observation)
        live_path=os.path.join(args.root,"external-owner-live.json"); live=None; custody_failure=None; finalization=None
        owner_birth_absent=birth_absent(owner.pid,owner_birth)
        try:
            if os.path.exists(live_path):
                with open(live_path,encoding="utf-8") as stream: live=strict_load(stream)
                if not isinstance(live,dict): live=None; raise RuntimeError("owner live custody invalid")
            with open(os.path.join(args.root,"external-owner-finalization.json"),encoding="utf-8") as stream: finalization=strict_load(stream)
            expected={"externalOwnerPID":owner.pid,"ownerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
            receipt_failed=finalization.get("finalizationFailure")=="cleanup_receipt_persistence_failed"
            receipt_fields={"cleanupOutcome"} if receipt_failed else set()
            if set(finalization)!=set(expected)|{"finalizationFailure","liveTuiChildRetained","primaryOutcome"}|receipt_fields or any(finalization.get(key)!=value for key,value in expected.items()) or finalization["liveTuiChildRetained"] is not False or finalization["finalizationFailure"] not in (None,"injected_cleanup_failure","finalization_failure","cleanup_failed","cleanup_receipt_persistence_failed") or finalization["primaryOutcome"] not in ("succeeded","failed","unresolved"): raise RuntimeError("owner finalization custody invalid")
            if receipt_failed:
                cleanup=finalization["cleanupOutcome"]
                reasons={"succeeded":("released_and_removed","sealed_descriptor_closed","no_execution_object","held_candidate_handle_closed"),"failed":("live_child_unresolved","leaf_release_failed","leaf_release_readback_failed","parent_release_failed","parent_release_readback_failed","protected_artifact_removal_failed","descriptor_close_failed")}
                if not isinstance(cleanup,dict) or set(cleanup)!={"outcome","closedReason","recoveryRetained"} or cleanup.get("outcome") not in reasons or cleanup.get("closedReason") not in reasons[cleanup["outcome"]] or type(cleanup.get("recoveryRetained")) is not bool or (cleanup["outcome"]=="succeeded" and cleanup["recoveryRetained"]): raise RuntimeError("owner cleanup persistence failure state invalid")
        except Exception as original:
            observation["originals"].append({"phase":"owner_finalization_receipt","original":original})
            custody_failure="owner_finalization_receipt_invalid_or_missing"
        if not owner_birth_absent: custody_failure="owner_birth_absence_unproven"
        if stream_failure(owner,stream_observation): custody_failure="owner_stream_close_failed"
        private_tuple={"observerPID":os.getpid(),"observerBirth":process_birth(os.getpid()),"recoveryOwnerPID":owner.pid,"ownerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
        if live is not None:
            if live.get("externalOwnerPID")!=owner.pid or live.get("coreRuntimePID")!=runtime.pid or live.get("sourceCommit")!=args.source_commit or live.get("binarySHA256")!=binary_sha256 or live.get("ownerBirth")!=owner_birth or live.get("runtimeBirth")!=runtime_birth: custody_failure="owner_runtime_tui_custody_mismatch"
            private_tuple.update({"tuiChildPID":live.get("tuiChildPID"),"tuiChildBirth":live.get("tuiChildBirth"),"ownerPhase":live.get("phase")})
        close={**private_tuple,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True,"ownerBirthAbsentObserved":owner_birth_absent,"ownerStdoutClosed":owner_closed["stdout"],"ownerStderrClosed":owner_closed["stderr"]}
        private(args.root,"owner-closed",close)
        if status != 0 and (status < 0 or (live is not None and live.get("phase")=="owner_death_live")):
            externally_reaped=False
            if live is not None and isinstance(live.get("tuiChildPID"),int):
                deadline=time.monotonic()+15
                while child_alive(live["tuiChildPID"],live.get("tuiChildBirth")) and time.monotonic()<deadline: time.sleep(.05)
                externally_reaped=not child_alive(live["tuiChildPID"],live.get("tuiChildBirth"))
            core_live=True
            try: runtime_live(ready)
            except Exception as original:
                observation["originals"].append({"phase":"runtime_live","original":original})
                core_live=False
            adverse_kill_observed=(status == -9 and live is not None and live.get("phase")=="owner_death_live")
            private(args.root,"unresolved",{**private_tuple,"outcome":"unresolved","ownerUnexpectedDeath":True,"adverseKillObserved":adverse_kill_observed,"actualTuiExternallyReaped":externally_reaped,"coreAndJwksLiveAfterOwnerDeath":core_live,"actualRuntimeExitObserved":False})
            result=close_runtime(runtime,stream_observation)
            private(args.root,"closed",{**private_tuple,"reason":"owner_death_after_unresolved","adverseKillObserved":adverse_kill_observed,**result})
            public(args.root,args.source_commit,binary_sha256,"unresolved",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
            raise SystemExit(1)
        result=close_runtime(runtime,stream_observation)
        if custody_failure is not None or stream_failure(runtime,stream_observation) or result["runtimeChildExit"]!="terminal_exited_zero" or not result["runtimeBirthAbsentObserved"]:
            private(args.root,"closed",{**private_tuple,"reason":custody_failure or "runtime_close_failed",**result})
            public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
            raise SystemExit(1)
        if finalization["finalizationFailure"] is not None:
            proof={"outcome":"failed","finalizationFailure":finalization["finalizationFailure"],"ownerDrivenRuntimeShutdown":result["shutdownRequested"],"runtimeChildExit":result["runtimeChildExit"],"retainedOwnedRuntime":False,"runtimeChildStillLiveAfterClose":result["runtimeChildStillLiveAfterClose"],"actualExitObserved":result["actualExitObserved"],"observerPID":os.getpid(),"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"ownerPID":owner.pid,"ownerBirth":owner_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
            if finalization["finalizationFailure"]=="cleanup_receipt_persistence_failed": proof["cleanupOutcome"]=finalization["cleanupOutcome"]
            write_json(os.path.join(args.root,"owner-finalization-failure-proof.json"),proof)
            private(args.root,"closed",{**private_tuple,"reason":"owner_finalization_failed",**result})
            public(args.root,args.source_commit,binary_sha256,"failed",False,True)
            if status != 0 or not args.inject_finalization_cleanup_failure or finalization["finalizationFailure"]!="injected_cleanup_failure" or finalization["primaryOutcome"]!="succeeded" or not result["shutdownRequested"]: raise SystemExit(1)
            return
        if status != 0:
            private(args.root,"closed",{**private_tuple,"reason":"owner_reported_failure",**result})
            public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"])
            raise SystemExit(status)
        if live is None or live.get("phase")!="normal_q_exit" or finalization["primaryOutcome"]!="succeeded":
            private(args.root,"closed",{**private_tuple,"reason":"owner_natural_terminal_receipt_missing",**result})
            public(args.root,args.source_commit,binary_sha256,"failed",False,True)
            raise SystemExit(1)
        private(args.root,"closed",{**private_tuple,"reason":"owner_normal_terminal",**result})
        observation["pendingPublic"]=(args.root,args.source_commit,binary_sha256,"succeeded",True,result["actualExitObserved"])
    except Exception as error:
        lifecycle_failed=True
        if isinstance(error,RuntimeAcquisitionFailure): failure_reason=error.reason
        observation["originals"].append({"phase":"lifecycle","original":error})
        primary=SystemExit(1)
        observation["primaryPresent"]=True; observation["primary"]=primary
        raise primary from error
    except BaseException as primary:
        observation["primaryPresent"]=True; observation["primary"]=primary
        raise
    finally:
        observation["finalizing"]=True
        # This boundary owns the ENTIRE lifecycle, including failures before
        # the owner consumes stdin. Persistence never precedes actual cleanup.
        owner_close=None
        if owner is not None:
            try:
                close_owned_stdin(owner,stream_observation)
                status=wait_owned(owner,stream_observation); owner_closed=close_streams(owner,stream_observation)
                owner_absent=birth_absent(owner.pid,owner_birth)
                owner_close={"observerPID":os.getpid(),"recoveryOwnerPID":owner.pid,"ownerBirth":owner_birth,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True,"ownerBirthAbsentObserved":owner_absent,"ownerStdoutClosed":owner_closed["stdout"],"ownerStderrClosed":owner_closed["stderr"]}
            except BaseException as original:
                observation["originals"].append({"phase":"owner_finalization","original":original})
                if not observation["primaryPresent"]:
                    observation["primaryPresent"]=True; observation["primary"]=original
                if runtime is not None and result is None:
                    try: result=close_runtime(runtime,stream_observation)
                    except BaseException as secondary:
                        observation["originals"].append({"phase":"runtime_peer_finalization","original":secondary})
                raise
        if result is None or lifecycle_failed:
            # Acquisition/birth/launch/write errors still close and wait the
            # actual held child; missing birth is explicitly unproven absence.
            if runtime is not None:
                if result is None: result=close_runtime(runtime,stream_observation)
                closure={"observerPID":os.getpid(),"runtimePID":runtime.pid,"runtimeBirth":getattr(runtime,"tui20_birth",None),"reason":failure_reason,**result}
            else:
                closure={"observerPID":os.getpid(),"reason":failure_reason,"runtimeStarted":False,"actualExitObserved":False,"runtimeBirthAbsentObserved":False}
            try:
                if owner_close is not None: private(args.root,"owner-closed",owner_close)
                private(args.root,"closed",closure)
            finally:
                public(args.root,args.source_commit,binary_sha256,"failed",False,result is not None and result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
        elif owner_close is not None:
            # Preserve richer phase fields produced by the normal path.
            path=os.path.join(args.root,"owner-private-owner-closed.json")
            existing={}
            try:
                with open(path,encoding="utf-8") as stream: existing=strict_load(stream)
                if existing!={"classification":"owner-private","phase":"owner-closed",**close}:
                    raise RuntimeError("owner closed receipt invalid")
            except (OSError,ValueError,RuntimeError) as original:
                observation["originals"].append({"phase":"owner_closed_receipt_read","original":original})
                if not observation["primaryPresent"]:
                    observation["primaryPresent"]=True; observation["primary"]=original
                public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
                raise
            try: private(args.root,"owner-closed",{**existing,**owner_close})
            except Exception:
                public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
                raise
            if not owner_close["ownerBirthAbsentObserved"]:
                public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
                raise SystemExit(1)

def main(observation=None):
    if observation is None: observation={}
    observation.update({"originals":[],"streams":[],"ready":[],"primaryPresent":False,"primary":None,"pendingPublic":None,"finalizing":False})
    try:
        _main_attempt(observation)
    except BaseException as original:
        if observation["primaryPresent"]:
            if original is not observation["primary"]:
                observation["originals"].append({"phase":"finalization","original":original})
            raise observation["primary"]
        observation["originals"].append({"phase":"finalization","original":original})
        raise
    pending=observation["pendingPublic"]
    if pending is not None:
        try: write_public(*pending)
        except BaseException as original:
            observation["originals"].append({"phase":"public_projection","original":original})
            raise

if __name__=="__main__": main()
