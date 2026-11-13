"""Durable POSIX custody parent for the real Core/JWKS and native PTY owner."""
import argparse, hashlib, json, os, select, subprocess, sys, time, urllib.request

def write_json(path, value):
    with open(path, "w", encoding="utf-8", newline="") as out:
        json.dump(value, out, separators=(",", ":")); out.flush(); os.fsync(out.fileno())

def write_private(root, phase, value):
    write_json(os.path.join(root, "owner-private-%s.json" % phase), {"classification":"owner-private", "phase":phase, **value})

def write_public(root, source_commit, binary_sha256, result, actions_passed, runtime_closed):
    write_json(os.path.join(root, "native-public-projection.json"), {"schemaVersion":1,"kind":"tui20-native-public-result","sourceCommit":source_commit,"binarySHA256":binary_sha256,"result":result,"actionsPassed":actions_passed,"ownedRuntimeClosed":runtime_closed})

def process_birth(pid):
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

def close_streams(process):
    """Drain only in memory, then close every observer-owned pipe."""
    for stream in (process.stdin, process.stdout, process.stderr):
        if stream is None: continue
        try:
            if stream in (process.stdout, process.stderr): stream.read()
        except (OSError, ValueError): pass
        try: stream.close()
        except (OSError, ValueError): pass

def close_runtime(runtime):
    """Only the directly-owned Core/JWKS child is closed; no process sweep occurs."""
    requested=True
    try: runtime.stdin.write("close\n"); runtime.stdin.flush()
    except Exception: requested=False
    # EOF is the runtime's existing graceful close path if the write failed.
    try: runtime.stdin.close()
    except (OSError, ValueError): pass
    runtime.wait(); close_streams(runtime)
    birth=getattr(runtime,"tui20_birth",None)
    absent=birth_absent(runtime.pid,birth)
    return {"runtimeChildExit":"terminal_exited_zero" if runtime.returncode==0 else "terminal_signaled" if runtime.returncode and runtime.returncode<0 else "terminal_exited_nonzero","shutdownRequested":requested,"actualExitObserved":True,"runtimeBirthAbsentObserved":absent,"runtimeChildStillLiveAfterClose":False if absent else None,"runtimeStdoutClosed":True,"runtimeStderrClosed":True}

def start_runtime(args, retain):
    command=[args.node,args.runtime_script,"--root",args.root,"--core-commit",args.core_commit]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.runtime_ready_mode: command += ["--ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    runtime=subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    # Retain the actual child before birth/read/persistence can raise.
    retain(runtime)
    runtime.tui20_birth=process_birth(runtime.pid)
    ready_stream,_,_=select.select([runtime.stdout],[],[],10)
    if not ready_stream:
        raise RuntimeAcquisitionFailure("runtime_ready_timeout")
    try: ready=json.loads(runtime.stdout.readline())
    except Exception as error:
        raise RuntimeAcquisitionFailure("runtime_ready_invalid") from error
    if ready.get("event")!="ready" or not all(isinstance(ready.get(key),str) and ready[key] for key in ("url","token","deniedToken")) or not isinstance(ready.get("jwksPort"),int):
        raise RuntimeAcquisitionFailure("runtime_ready_invalid")
    return runtime,ready

def runtime_live(ready):
    with urllib.request.urlopen(ready["url"]+"/api/health",timeout=2) as response: json.load(response)
    with urllib.request.urlopen("http://127.0.0.1:%d/jwks" % ready["jwksPort"],timeout=2) as response: response.read()

def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--owner",required=True); parser.add_argument("--root",required=True)
    parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True)
    parser.add_argument("--runtime-script",required=True); parser.add_argument("--node",required=True); parser.add_argument("--adverse-controller-crash",action="store_true")
    parser.add_argument("--controller-pid",type=int); parser.add_argument("--invalid-ready-receipt",action="store_true"); parser.add_argument("--inject-finalization-cleanup-failure",action="store_true")
    parser.add_argument("--runtime-ready-mode",choices=("normal","timeout","eof")); parser.add_argument("--shutdown-pipe-failure",action="store_true"); parser.add_argument("--adverse-owner-death",action="store_true")
    args=parser.parse_args(); binary_sha256=sha256(args.executable)
    runtime=None; owner=None; owner_birth=None; result=None; lifecycle_failed=False; failure_reason="observer_acquisition_or_handoff_failed"
    def retain(child):
        nonlocal runtime
        runtime=child
    try:
        runtime,ready=start_runtime(args,retain); runtime_birth=runtime.tui20_birth
        write_private(args.root,"initial",{"observerPID":os.getpid(),"observerBirth":process_birth(os.getpid()),"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256,"ownedCoreAndJwks":True})
        command=[sys.executable,args.owner,"--root",args.root,"--executable",args.executable,"--source-commit",args.source_commit,"--core-commit",args.core_commit,"--external-runtime"]
        if args.adverse_controller_crash: command += ["--adverse-controller-crash","--controller-pid",str(args.controller_pid)]
        if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
        if args.inject_finalization_cleanup_failure: command.append("--inject-finalization-cleanup-failure")
        if args.runtime_ready_mode: command += ["--runtime-ready-mode",args.runtime_ready_mode]
        if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
        if args.adverse_owner_death: command.append("--adverse-owner-death")
        ready["_runtimePID"]=runtime.pid; ready["_runtimeBirth"]=runtime_birth
        owner=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        owner_birth=process_birth(owner.pid)
        write_private(args.root,"witness",{"observerPID":os.getpid(),"recoveryOwnerPID":owner.pid,"recoveryOwnerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256,"ownerStarted":True})
        owner.stdin.write(json.dumps(ready,separators=(",",":"))+"\n"); owner.stdin.close()
        status=owner.wait(); close_streams(owner)
        live_path=os.path.join(args.root,"external-owner-live.json"); live=None; custody_failure=None; finalization=None
        owner_birth_absent=birth_absent(owner.pid,owner_birth)
        try:
            if os.path.exists(live_path):
                with open(live_path,encoding="utf-8") as stream: live=json.load(stream)
                if not isinstance(live,dict): live=None; raise RuntimeError("owner live custody invalid")
            with open(os.path.join(args.root,"external-owner-finalization.json"),encoding="utf-8") as stream: finalization=json.load(stream)
            expected={"externalOwnerPID":owner.pid,"ownerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
            receipt_failed=finalization.get("finalizationFailure")=="cleanup_receipt_persistence_failed"
            receipt_fields={"cleanupOutcome"} if receipt_failed else set()
            if set(finalization)!=set(expected)|{"finalizationFailure","liveTuiChildRetained","primaryOutcome"}|receipt_fields or any(finalization.get(key)!=value for key,value in expected.items()) or finalization["liveTuiChildRetained"] is not False or finalization["finalizationFailure"] not in (None,"injected_cleanup_failure","finalization_failure","cleanup_failed","cleanup_receipt_persistence_failed") or finalization["primaryOutcome"] not in ("succeeded","failed","unresolved"): raise RuntimeError("owner finalization custody invalid")
            if receipt_failed:
                cleanup=finalization["cleanupOutcome"]
                reasons={"succeeded":("released_and_removed","sealed_descriptor_closed","no_execution_object"),"failed":("live_child_unresolved","leaf_release_failed","leaf_release_readback_failed","parent_release_failed","parent_release_readback_failed","protected_artifact_removal_failed","descriptor_close_failed")}
                if not isinstance(cleanup,dict) or set(cleanup)!={"outcome","closedReason","recoveryRetained"} or cleanup.get("outcome") not in reasons or cleanup.get("closedReason") not in reasons[cleanup["outcome"]] or type(cleanup.get("recoveryRetained")) is not bool or (cleanup["outcome"]=="succeeded" and cleanup["recoveryRetained"]): raise RuntimeError("owner cleanup persistence failure state invalid")
        except Exception:
            custody_failure="owner_finalization_receipt_invalid_or_missing"
        if not owner_birth_absent: custody_failure="owner_birth_absence_unproven"
        private_tuple={"observerPID":os.getpid(),"observerBirth":process_birth(os.getpid()),"recoveryOwnerPID":owner.pid,"ownerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
        if live is not None:
            if live.get("externalOwnerPID")!=owner.pid or live.get("coreRuntimePID")!=runtime.pid or live.get("sourceCommit")!=args.source_commit or live.get("binarySHA256")!=binary_sha256 or live.get("ownerBirth")!=owner_birth or live.get("runtimeBirth")!=runtime_birth: custody_failure="owner_runtime_tui_custody_mismatch"
            private_tuple.update({"tuiChildPID":live.get("tuiChildPID"),"tuiChildBirth":live.get("tuiChildBirth"),"ownerPhase":live.get("phase")})
        close={**private_tuple,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True,"ownerBirthAbsentObserved":owner_birth_absent,"ownerStdoutClosed":True,"ownerStderrClosed":True}
        write_private(args.root,"owner-closed",close)
        if status != 0 and (status < 0 or (live is not None and live.get("phase")=="owner_death_live")):
            externally_reaped=False
            if live is not None and isinstance(live.get("tuiChildPID"),int):
                deadline=time.monotonic()+15
                while child_alive(live["tuiChildPID"],live.get("tuiChildBirth")) and time.monotonic()<deadline: time.sleep(.05)
                externally_reaped=not child_alive(live["tuiChildPID"],live.get("tuiChildBirth"))
            core_live=True
            try: runtime_live(ready)
            except Exception: core_live=False
            adverse_kill_observed=(status == -9 and live is not None and live.get("phase")=="owner_death_live")
            write_private(args.root,"unresolved",{**private_tuple,"outcome":"unresolved","ownerUnexpectedDeath":True,"adverseKillObserved":adverse_kill_observed,"actualTuiExternallyReaped":externally_reaped,"coreAndJwksLiveAfterOwnerDeath":core_live,"actualRuntimeExitObserved":False})
            result=close_runtime(runtime)
            write_private(args.root,"closed",{**private_tuple,"reason":"owner_death_after_unresolved","adverseKillObserved":adverse_kill_observed,**result})
            write_public(args.root,args.source_commit,binary_sha256,"unresolved",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
            raise SystemExit(1)
        result=close_runtime(runtime)
        if custody_failure is not None or result["runtimeChildExit"]!="terminal_exited_zero" or not result["runtimeBirthAbsentObserved"]:
            write_private(args.root,"closed",{**private_tuple,"reason":custody_failure or "runtime_close_failed",**result})
            write_public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
            raise SystemExit(1)
        if finalization["finalizationFailure"] is not None:
            proof={"outcome":"failed","finalizationFailure":finalization["finalizationFailure"],"ownerDrivenRuntimeShutdown":result["shutdownRequested"],"runtimeChildExit":result["runtimeChildExit"],"retainedOwnedRuntime":False,"runtimeChildStillLiveAfterClose":result["runtimeChildStillLiveAfterClose"],"actualExitObserved":result["actualExitObserved"],"observerPID":os.getpid(),"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"ownerPID":owner.pid,"ownerBirth":owner_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
            if finalization["finalizationFailure"]=="cleanup_receipt_persistence_failed": proof["cleanupOutcome"]=finalization["cleanupOutcome"]
            write_json(os.path.join(args.root,"owner-finalization-failure-proof.json"),proof)
            write_private(args.root,"closed",{**private_tuple,"reason":"owner_finalization_failed",**result})
            write_public(args.root,args.source_commit,binary_sha256,"failed",False,True)
            if status != 0 or not args.inject_finalization_cleanup_failure or finalization["finalizationFailure"]!="injected_cleanup_failure" or finalization["primaryOutcome"]!="succeeded" or not result["shutdownRequested"]: raise SystemExit(1)
            return
        if status != 0:
            write_private(args.root,"closed",{**private_tuple,"reason":"owner_reported_failure",**result})
            write_public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"])
            raise SystemExit(status)
        if live is None or live.get("phase")!="normal_q_exit" or finalization["primaryOutcome"]!="succeeded":
            write_private(args.root,"closed",{**private_tuple,"reason":"owner_natural_terminal_receipt_missing",**result})
            write_public(args.root,args.source_commit,binary_sha256,"failed",False,True)
            raise SystemExit(1)
        write_private(args.root,"closed",{**private_tuple,"reason":"owner_normal_terminal",**result})
        write_public(args.root,args.source_commit,binary_sha256,"succeeded",True,result["actualExitObserved"])
    except Exception as error:
        lifecycle_failed=True
        if isinstance(error,RuntimeAcquisitionFailure): failure_reason=error.reason
        raise SystemExit(1) from error
    finally:
        # This boundary owns the ENTIRE lifecycle, including failures before
        # the owner consumes stdin. Persistence never precedes actual cleanup.
        owner_close=None
        if owner is not None:
            try: owner.stdin.close()
            except (OSError,ValueError): pass
            status=owner.wait(); close_streams(owner)
            owner_absent=birth_absent(owner.pid,owner_birth)
            owner_close={"observerPID":os.getpid(),"recoveryOwnerPID":owner.pid,"ownerBirth":owner_birth,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True,"ownerBirthAbsentObserved":owner_absent,"ownerStdoutClosed":True,"ownerStderrClosed":True}
        if result is None or lifecycle_failed:
            # Acquisition/birth/launch/write errors still close and wait the
            # actual held child; missing birth is explicitly unproven absence.
            if runtime is not None:
                if result is None: result=close_runtime(runtime)
                closure={"observerPID":os.getpid(),"runtimePID":runtime.pid,"runtimeBirth":getattr(runtime,"tui20_birth",None),"reason":failure_reason,**result}
            else:
                closure={"observerPID":os.getpid(),"reason":failure_reason,"runtimeStarted":False,"actualExitObserved":False,"runtimeBirthAbsentObserved":False}
            try:
                if owner_close is not None: write_private(args.root,"owner-closed",owner_close)
                write_private(args.root,"closed",closure)
            finally:
                write_public(args.root,args.source_commit,binary_sha256,"failed",False,result is not None and result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
        elif owner_close is not None:
            # Preserve richer phase fields produced by the normal path.
            path=os.path.join(args.root,"owner-private-owner-closed.json")
            existing={}
            try:
                with open(path,encoding="utf-8") as stream: existing=json.load(stream)
            except (OSError,ValueError): pass
            try: write_private(args.root,"owner-closed",{**existing,**owner_close})
            except Exception:
                write_public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
                raise
            if not owner_close["ownerBirthAbsentObserved"]:
                write_public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"] and result["runtimeBirthAbsentObserved"])
                raise SystemExit(1)

if __name__=="__main__": main()
