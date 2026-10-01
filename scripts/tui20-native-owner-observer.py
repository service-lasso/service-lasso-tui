"""Durable POSIX custody parent for the real Core/JWKS and native PTY owner."""
import argparse, hashlib, json, os, select, subprocess, sys, time, urllib.request

def write_json(path, value):
    with open(path, "w", encoding="utf-8", newline="") as out:
        json.dump(value, out, separators=(",", ":")); out.flush(); os.fsync(out.fileno())

def process_birth(pid):
    if sys.platform == "linux":
        with open("/proc/%d/stat" % pid, encoding="utf-8") as stream:
            return {"platform":"linux", "startTimeTicks":stream.read().rsplit(") ", 1)[1].split()[19]}
    if sys.platform == "darwin":
        value=subprocess.check_output(["/bin/ps","-o","lstart=","-p",str(pid)], text=True).strip()
        if value: return {"platform":"darwin", "startTime":value}
    raise RuntimeError("POSIX process birth unavailable")

def sha256(path):
    digest=hashlib.sha256()
    with open(path,"rb") as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b""): digest.update(chunk)
    return digest.hexdigest()

def child_alive(pid, birth):
    try: return process_birth(pid)==birth
    except (FileNotFoundError, subprocess.CalledProcessError): return False

def start_runtime(args):
    command=[args.node,args.runtime_script,"--root",args.root,"--core-commit",args.core_commit]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.runtime_ready_mode: command += ["--ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    runtime=subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    ready_stream,_,_=select.select([runtime.stdout],[],[],10)
    if not ready_stream:
        try:
            birth=process_birth(runtime.pid); result=close_runtime(runtime)
            write_json(os.path.join(args.root,"owner-runtime-recovery.json"),{"outcome":"observed_closed","runtimePID":runtime.pid,"runtimeBirth":birth,**result})
        except Exception: pass
        raise RuntimeError("actual Core/JWKS runtime ready handoff timed out")
    try: ready=json.loads(runtime.stdout.readline())
    except Exception as error: raise RuntimeError("actual Core/JWKS runtime handoff invalid") from error
    if ready.get("event")!="ready" or not all(isinstance(ready.get(key),str) and ready[key] for key in ("url","token","deniedToken")) or not isinstance(ready.get("jwksPort"),int): raise RuntimeError("actual Core/JWKS runtime handoff invalid")
    return runtime,ready

def close_runtime(runtime):
    requested=True
    try: runtime.stdin.write("close\n"); runtime.stdin.flush()
    except Exception: requested=False
    runtime.wait()
    return {"runtimeChildExit":"terminal_exited_zero" if runtime.returncode==0 else "terminal_signaled" if runtime.returncode and runtime.returncode<0 else "terminal_exited_nonzero", "shutdownRequested":requested, "actualExitObserved":True}

def runtime_live(ready):
    with urllib.request.urlopen(ready["url"]+"/api/health",timeout=2) as response: json.load(response)
    with urllib.request.urlopen("http://127.0.0.1:%d/jwks" % ready["jwksPort"],timeout=2) as response: response.read()

def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--owner",required=True); parser.add_argument("--root",required=True)
    parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True)
    parser.add_argument("--runtime-script",required=True); parser.add_argument("--node",required=True); parser.add_argument("--adverse-controller-crash",action="store_true")
    parser.add_argument("--controller-pid",type=int); parser.add_argument("--invalid-ready-receipt",action="store_true"); parser.add_argument("--inject-finalization-cleanup-failure",action="store_true")
    parser.add_argument("--runtime-ready-mode",choices=("normal","timeout","eof")); parser.add_argument("--shutdown-pipe-failure",action="store_true"); parser.add_argument("--adverse-owner-death",action="store_true")
    args=parser.parse_args()
    runtime,ready=start_runtime(args)
    command=[sys.executable,args.owner,"--root",args.root,"--executable",args.executable,"--source-commit",args.source_commit,"--core-commit",args.core_commit,"--external-runtime"]
    if args.adverse_controller_crash: command += ["--adverse-controller-crash","--controller-pid",str(args.controller_pid)]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.inject_finalization_cleanup_failure: command.append("--inject-finalization-cleanup-failure")
    if args.runtime_ready_mode: command += ["--runtime-ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    if args.adverse_owner_death: command.append("--adverse-owner-death")
    ready["_runtimePID"]=runtime.pid; ready["_runtimeBirth"]=process_birth(runtime.pid)
    owner=subprocess.Popen(command,stdin=subprocess.PIPE,text=True)
    owner_birth=process_birth(owner.pid); runtime_birth=process_birth(runtime.pid)
    write_json(os.path.join(args.root,"owner-birth.json"),{"recoveryOwnerPID":owner.pid,"recoveryOwnerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"nativeSHA256":sha256(args.executable),"ownerStarted":True,"observerOwnsActualRuntime":True})
    owner.stdin.write(json.dumps(ready,separators=(",",":"))+"\n"); owner.stdin.close()
    status=owner.wait()
    live_path=os.path.join(args.root,"external-owner-live.json"); live=None
    if os.path.exists(live_path):
        with open(live_path,encoding="utf-8") as stream: live=json.load(stream)
    tuple_value={"observerPID":os.getpid(),"observerBirth":process_birth(os.getpid()),"recoveryOwnerPID":owner.pid,"ownerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":sha256(args.executable),"phase":"owner_terminal"}
    if live is not None:
        if live.get("externalOwnerPID")!=owner.pid or live.get("coreRuntimePID")!=runtime.pid or live.get("sourceCommit")!=args.source_commit or live.get("binarySHA256")!=tuple_value["binarySHA256"] or live.get("ownerBirth")!=owner_birth or live.get("runtimeBirth")!=runtime_birth: raise RuntimeError("owner/runtime/TUI custody tuple mismatch")
        tuple_value.update({"tuiChildPID":live.get("tuiChildPID"),"tuiChildBirth":live.get("tuiChildBirth"),"phase":live.get("phase")})
    close={**tuple_value,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True}
    write_json(os.path.join(args.root,"owner-close.json"),close)
    if status != 0 and (status < 0 or (live is not None and live.get("phase")=="owner_death_live")):
        externally_reaped=False
        if live is not None and isinstance(live.get("tuiChildPID"),int):
            deadline=time.monotonic()+15
            while child_alive(live["tuiChildPID"],live.get("tuiChildBirth")) and time.monotonic()<deadline: time.sleep(.05)
            externally_reaped=not child_alive(live["tuiChildPID"],live.get("tuiChildBirth"))
        core_live=True
        try: runtime_live(ready)
        except Exception: core_live=False
        write_json(os.path.join(args.root,"owner-death-recovery.json"),{**tuple_value,"outcome":"unresolved","ownerUnexpectedDeath":True,"terminal":"terminal_unknown","actualTuiExternallyReaped":externally_reaped,"coreAndJwksLiveAfterOwnerDeath":core_live,"runtimeRetained":runtime.poll() is None,"actualRuntimeExitObserved":False})
        raise SystemExit(1)
    if status != 0:
        # A reported harness/preflight failure is not an owner-death claim. The
        # observer still owns the actual runtime and can close it only after
        # directly observing the failed owner and its closed primary receipt.
        result=close_runtime(runtime)
        if args.invalid_ready_receipt:
            write_json(os.path.join(args.root,"owner-preflight-cleanup.json"),{"outcome":"failed","preflight":"runtime_receipt_invalid","ownerDrivenRuntimeShutdown":result["shutdownRequested"],**result,"retainedOwnedRuntime":False})
        else:
            write_json(os.path.join(args.root,"owner-runtime-recovery.json"),{"outcome":"observed_closed","runtimePID":runtime.pid,"runtimeBirth":runtime_birth,**result})
        write_json(os.path.join(args.root,"core-parent-exit.json"),{**tuple_value,**result,"coreStopAfterVerifiedOwnerTerminalReceipt":False})
        raise SystemExit(status)
    if live is not None and live.get("phase")!="normal_q_exit": raise RuntimeError("normal owner exit lacks a natural TUI terminal phase")
    result=close_runtime(runtime)
    write_json(os.path.join(args.root,"core-parent-exit.json"),{**tuple_value,**result,"coreStopAfterVerifiedOwnerTerminalReceipt":True})

if __name__=="__main__": main()
