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
    raise RuntimeError("POSIX process birth unavailable")

def sha256(path):
    digest=hashlib.sha256()
    with open(path,"rb") as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b""): digest.update(chunk)
    return digest.hexdigest()

def child_alive(pid, birth):
    try: return process_birth(pid)==birth
    except (FileNotFoundError, subprocess.CalledProcessError): return False

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
    runtime.wait(); close_streams(runtime)
    return {"runtimeChildExit":"terminal_exited_zero" if runtime.returncode==0 else "terminal_signaled" if runtime.returncode and runtime.returncode<0 else "terminal_exited_nonzero","shutdownRequested":requested,"actualExitObserved":True,"runtimeStdoutClosed":True,"runtimeStderrClosed":True}

def start_runtime(args):
    command=[args.node,args.runtime_script,"--root",args.root,"--core-commit",args.core_commit]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.runtime_ready_mode: command += ["--ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    runtime=subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    ready_stream,_,_=select.select([runtime.stdout],[],[],10)
    if not ready_stream:
        birth=process_birth(runtime.pid); result=close_runtime(runtime)
        write_private(args.root,"closed",{"reason":"runtime_ready_timeout","runtimePID":runtime.pid,"runtimeBirth":birth,**result})
        raise RuntimeError("actual Core/JWKS runtime ready handoff timed out")
    try: ready=json.loads(runtime.stdout.readline())
    except Exception as error:
        birth=process_birth(runtime.pid); result=close_runtime(runtime)
        write_private(args.root,"closed",{"reason":"runtime_ready_invalid","runtimePID":runtime.pid,"runtimeBirth":birth,**result})
        raise RuntimeError("actual Core/JWKS runtime handoff invalid") from error
    if ready.get("event")!="ready" or not all(isinstance(ready.get(key),str) and ready[key] for key in ("url","token","deniedToken")) or not isinstance(ready.get("jwksPort"),int):
        birth=process_birth(runtime.pid); result=close_runtime(runtime)
        write_private(args.root,"closed",{"reason":"runtime_ready_invalid","runtimePID":runtime.pid,"runtimeBirth":birth,**result})
        raise RuntimeError("actual Core/JWKS runtime handoff invalid")
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
    runtime,ready=start_runtime(args); runtime_birth=process_birth(runtime.pid)
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
    live_path=os.path.join(args.root,"external-owner-live.json"); live=None
    if os.path.exists(live_path):
        with open(live_path,encoding="utf-8") as stream: live=json.load(stream)
    private_tuple={"observerPID":os.getpid(),"observerBirth":process_birth(os.getpid()),"recoveryOwnerPID":owner.pid,"ownerBirth":owner_birth,"runtimePID":runtime.pid,"runtimeBirth":runtime_birth,"sourceCommit":args.source_commit,"binarySHA256":binary_sha256}
    if live is not None:
        if live.get("externalOwnerPID")!=owner.pid or live.get("coreRuntimePID")!=runtime.pid or live.get("sourceCommit")!=args.source_commit or live.get("binarySHA256")!=binary_sha256 or live.get("ownerBirth")!=owner_birth or live.get("runtimeBirth")!=runtime_birth: raise RuntimeError("owner/runtime/TUI custody tuple mismatch")
        private_tuple.update({"tuiChildPID":live.get("tuiChildPID"),"tuiChildBirth":live.get("tuiChildBirth"),"ownerPhase":live.get("phase")})
    close={**private_tuple,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True,"ownerStdoutClosed":True,"ownerStderrClosed":True}
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
        write_public(args.root,args.source_commit,binary_sha256,"unresolved",False,result["actualExitObserved"])
        raise SystemExit(1)
    result=close_runtime(runtime)
    if status != 0:
        write_private(args.root,"closed",{**private_tuple,"reason":"owner_reported_failure",**result})
        write_public(args.root,args.source_commit,binary_sha256,"failed",False,result["actualExitObserved"])
        raise SystemExit(status)
    if live is not None and live.get("phase")!="normal_q_exit": raise RuntimeError("normal owner exit lacks a natural TUI terminal phase")
    write_private(args.root,"closed",{**private_tuple,"reason":"owner_normal_terminal",**result})
    write_public(args.root,args.source_commit,binary_sha256,"succeeded",True,result["actualExitObserved"])

if __name__=="__main__": main()
