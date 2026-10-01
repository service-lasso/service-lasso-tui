"""External parent for the POSIX resource owner; it is the only close attestor."""
import argparse, json, os, subprocess, sys

def write_json(path, value):
    with open(path, "w", encoding="utf-8", newline="") as out:
        json.dump(value, out, separators=(",", ":")); out.flush(); os.fsync(out.fileno())

def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--owner",required=True); parser.add_argument("--root",required=True)
    parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True)
    parser.add_argument("--runtime-script",required=True); parser.add_argument("--node",required=True); parser.add_argument("--adverse-controller-crash",action="store_true")
    parser.add_argument("--controller-pid",type=int); parser.add_argument("--invalid-ready-receipt",action="store_true"); parser.add_argument("--inject-finalization-cleanup-failure",action="store_true")
    parser.add_argument("--runtime-ready-mode",choices=("normal","timeout","eof")); parser.add_argument("--shutdown-pipe-failure",action="store_true")
    args=parser.parse_args()
    command=[sys.executable,args.owner,"--root",args.root,"--executable",args.executable,"--source-commit",args.source_commit,"--core-commit",args.core_commit,"--runtime-script",args.runtime_script,"--node",args.node]
    if args.adverse_controller_crash: command += ["--adverse-controller-crash","--controller-pid",str(args.controller_pid)]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.inject_finalization_cleanup_failure: command.append("--inject-finalization-cleanup-failure")
    if args.runtime_ready_mode: command += ["--runtime-ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    owner=subprocess.Popen(command)
    status=owner.wait()
    # This is an observed wait status from the owner's parent, never a planned
    # exit value emitted by the owner itself.
    close={"observerPID":os.getpid(),"recoveryOwnerPID":owner.pid,"recoveryOwnerActualExit":status if status>=0 else None,"recoveryOwnerActualSignal":-status if status<0 else None,"actualWaitObserved":True,"coreStopAfterVerifiedOwnerTerminalReceipt":True}
    write_json(os.path.join(args.root,"owner-close.json"),close)
    write_json(os.path.join(args.root,"core-parent-exit.json"),close)
    raise SystemExit(status if status>=0 else 1)

if __name__=="__main__": main()
