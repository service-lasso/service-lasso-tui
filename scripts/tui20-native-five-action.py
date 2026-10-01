"""Windows ConPTY lifecycle evidence with memory-only bearer credentials."""
import argparse, ctypes, hashlib, json, os, select, stat, time, urllib.request
from ctypes import wintypes
from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess

ALLOWED_ENV=("APPDATA","COMSPEC","LOCALAPPDATA","PATHEXT","PATH","SYSTEMROOT","TEMP","TMP","USERPROFILE","WINDIR")
def write_json(path,value):
    with open(path,"w",encoding="utf-8",newline="") as f: json.dump(value,f,separators=(",",":"))
def append_terminal(path,label,chunk):
    with open(path,"a",encoding="utf-8",newline="") as f: f.write("\n--- "+label+" ---\n"+chunk)
def hold_candidate(executable,source):
    if len(source)!=40: raise RuntimeError("candidate source identity invalid")
    executable=os.path.abspath(executable); meta=os.lstat(executable)
    if not stat.S_ISREG(meta.st_mode) or os.name!="nt": raise RuntimeError("candidate executable invalid")
    k=ctypes.WinDLL("kernel32",use_last_error=True); k.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,wintypes.LPVOID,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]; k.CreateFileW.restype=wintypes.HANDLE
    handle=k.CreateFileW(executable,0x80000000,0x00000001,None,3,0x80,None)
    if handle==wintypes.HANDLE(-1).value: raise OSError(ctypes.get_last_error(),"candidate handle acquisition failed")
    import msvcrt
    f=os.fdopen(msvcrt.open_osfhandle(handle,os.O_RDONLY),"rb"); digest=hashlib.sha256()
    try:
        while True:
            chunk=f.read(1024*1024)
            if not chunk: break
            digest.update(chunk)
        return executable,f,{"sourceCommit":source,"binarySHA256":digest.hexdigest()}
    except Exception: f.close(); raise
def terminal_exit(process):
    try:
        if process.isalive(): return None
        signal_status,exit_status=getattr(process,"signalstatus",None),getattr(process,"exitstatus",None)
    except Exception: return "terminal_unknown"
    if isinstance(signal_status,int) and not isinstance(signal_status,bool) and signal_status!=0: return "terminal_signaled"
    if signal_status not in (None,0): return "terminal_unknown"
    if isinstance(exit_status,int) and not isinstance(exit_status,bool):
        return "terminal_exited_zero" if exit_status==0 else "terminal_exit_code_1" if exit_status==1 else "terminal_exit_code_2" if exit_status==2 else "terminal_exited_nonzero"
    return "terminal_unknown"
def drain(process,transcript,path,label):
    try: chunk=process.read()
    except EOFError: return None
    append_terminal(path,label,chunk); transcript[0]+=chunk; return chunk
def wait_for(process,expected,timeout,transcript,path,label,start=0):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        if all(x in transcript[0][start:] for x in expected): return
        readable,_,_=select.select([process],[],[],.1)
        if readable and drain(process,transcript,path,label) is None:
            reason=terminal_exit(process)
            if reason is not None: raise RuntimeError("terminal exited before expected state: "+reason)
            time.sleep(.05)
    raise RuntimeError("missing terminal state")
def close_terminal(process,transcript,path,label,expected="terminal_exited_zero"):
    if process.isalive(): process.write("q")
    end=time.monotonic()+10
    while time.monotonic()<end:
        readable,_,_=select.select([process],[],[],.1)
        if readable: drain(process,transcript,path,label)
        reason=terminal_exit(process)
        if reason is not None:
            if reason!=expected: raise RuntimeError("unexpected terminal exit: "+reason)
            return reason
    raise RuntimeError("terminal exit was not observed")
def request_json(url,token,path):
    req=urllib.request.Request(url+path,headers={"Authorization":"Bearer "+token})
    with urllib.request.urlopen(req,timeout=15) as response: return json.load(response)
def records(url,token): return request_json(url,token,"/api/operator/lifecycle/operations")["operations"]
def stable_hash(value): return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(",",":")).encode()).hexdigest()
def unrelated_snapshot(url,token):
    result=request_json(url,token,"/api/services"); entries=result.get("services",result) if isinstance(result,dict) else result
    service=next((x for x in entries if isinstance(x,dict) and x.get("id")=="tui20-unrelated"),None)
    availability=request_json(url,token,"/api/operator/lifecycle/services/tui20-unrelated/availability")
    if service is None: raise RuntimeError("unrelated service missing")
    return {"servicePresent":True,"runtimeSHA256":stable_hash(service),"lifecycleSHA256":stable_hash(availability)}
def detail(p,t,path,label):
    p.write("/"); p.write("tui20-fixture"); p.write("\r"); wait_for(p,("tui20-fixture",),20,t,path,label); p.write("\r"); wait_for(p,("Lifecycle:","tui20-fixture"),20,t,path,label)
def action(p,t,path,key,name):
    start=len(t[0]); p.write(key); wait_for(p,("Core preview: "+name,"Confirm "+name),45,t,path,"allowed",start); frozen=t[0]; p.write("rj?q"); time.sleep(.3)
    if "Confirm "+name not in t[0] or "Core preview: "+name not in t[0]: raise RuntimeError("confirmation changed by blocked input")
    p.write("y"); p.write("r"); wait_for(p,("Core operation","succeeded."),90,t,path,"allowed",start)
    if t[0].count("Core operation")<frozen.count("Core operation")+1: raise RuntimeError("operation result was not rendered")
def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--root",required=True); parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True); args=parser.parse_args()
    with open(os.path.join(args.root,"ready.json"),encoding="utf-8") as f: ready=json.load(f)
    paths={"configuredBeforeFirstInvocation":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"]}
    if ready.get("coreCommit")!=args.core_commit or ready.get("runtimePathReceipt")!=paths: raise RuntimeError("Core fixture receipt invalid")
    token=os.environ.get("SERVICE_LASSO_TUI20_TOKEN"); denied=os.environ.get("SERVICE_LASSO_TUI20_DENIED_TOKEN"); url=os.environ.get("SERVICE_LASSO_TUI20_API_URL")
    if not token or not denied or not url: raise RuntimeError("owned in-memory credential handoff unavailable")
    with open(os.path.join(args.root,"connections.json"),encoding="utf-8") as f: connections=json.load(f)
    env={key:os.environ[key] for key in ALLOWED_ENV if os.environ.get(key)}; env.update({"TERM":"xterm-256color","SERVICE_LASSO_API_TOKEN":token,"SERVICE_LASSO_DENIED_TOKEN":denied,"SERVICE_LASSO_INVALID_TOKEN":"tui20-invalid-token","SERVICE_LASSO_CONNECTIONS_CONFIG":os.path.join(args.root,"connections.json")})
    executable,held,identity=hold_candidate(args.executable,args.source_commit); transcript_path=os.path.join(args.root,"native-terminal.txt"); outcome={"outcome":"failed","candidateIdentity":identity,"coreCommit":args.core_commit,"runtimePathReceipt":paths,"terminals":[]}; open_processes=[]
    def launch(profile,label):
        p=PtyProcess.spawn([executable,"--profile",profile],cwd=os.path.dirname(executable),env=env,dimensions=(44,150),backend=Backend.ConPTY); open_processes.append(p); return p,[""]
    def finish(p,t,label,expected="terminal_exited_zero"):
        outcome["terminals"].append({"terminal":label,"exit":close_terminal(p,t,transcript_path,label,expected)}); open_processes.remove(p)
    try:
        before_unrelated=unrelated_snapshot(url,token); adverse=[]
        p,t=launch("missing","missing-credential"); before=len(records(url,token)); wait_for(p,('connection profile "missing" credential is unavailable',),30,t,transcript_path,"missing-credential"); finish(p,t,"missing-credential","terminal_exit_code_2"); after=len(records(url,token)); adverse.append({"case":"missing-credential","beforeOperationCount":before,"afterOperationCount":after,"noOperation":after==before})
        for profile,label in (("invalid","invalid-credential"),("denied","scope-denied")):
            p,t=launch(profile,label); before=len(records(url,token)); wait_for(p,("Runtime identity:",),30,t,transcript_path,label); detail(p,t,transcript_path,label); p.write("i"); wait_for(p,("Runtime API unavailable:",),30,t,transcript_path,label); finish(p,t,label); after=len(records(url,token)); adverse.append({"case":label,"beforeOperationCount":before,"afterOperationCount":after,"noOperation":after==before})
        if not all(x["noOperation"] for x in adverse): raise RuntimeError("adverse lifecycle case created an operation")
        p,t=launch("native","allowed"); wait_for(p,("Runtime identity:",),30,t,transcript_path,"allowed"); detail(p,t,transcript_path,"allowed")
        for key,name in (("i","install"),("c","config"),("s","start"),("x","stop"),("R","restart")): action(p,t,transcript_path,key,name)
        before_reload=records(url,token); p.write("l"); wait_for(p,("reload is unavailable",),15,t,transcript_path,"allowed")
        if len(records(url,token))!=len(before_reload) or "z cancel" in t[0]: raise RuntimeError("reload or cancellation contract changed")
        p.write("r"); wait_for(p,("Lifecycle:",),20,t,transcript_path,"allowed"); finish(p,t,"allowed")
        p,t=launch("native","reconnect"); wait_for(p,("Runtime identity:",),30,t,transcript_path,"reconnect"); detail(p,t,transcript_path,"reconnect"); finish(p,t,"reconnect")
        all_records=records(url,token); audit=[{key:r.get(key) for key in ("operationId","action","targetIds","status","outcome","cancellationSupported")} for r in all_records if r.get("targetIds")==["tui20-fixture"]]
        expected=["service_restart","service_stop","service_start","service_configure","service_install"]
        if len(audit)!=5 or sorted(x["action"] for x in audit)!=sorted(expected) or any(not x["operationId"] or x["cancellationSupported"] for x in audit) or len(all_records)!=len(before_reload): raise RuntimeError("durable action audit invalid")
        after_unrelated=unrelated_snapshot(url,token)
        if after_unrelated!=before_unrelated: raise RuntimeError("unrelated runtime or lifecycle state changed")
        write_json(os.path.join(args.root,"operation-audit.json"),{"operations":audit}); outcome.update({"outcome":"succeeded","actions":["install","config","start","stop","restart"],"adverseAudit":adverse,"reloadDenied":True,"cancellation":{"advertised":False,"supportedActionTested":False},"reconnect":{"completedOperationNoReplay":True,"pendingReconciliation":"blocked_core_1553_no_adapter"},"unrelatedService":{**before_unrelated,"unchangedAfterFiveActions":True},"fixtureOperationCount":len(audit)})
    finally:
        for p in open_processes:
            try:
                if p.isalive(): p.close(force=True)
                outcome["terminals"].append({"terminal":"aborted","exit":terminal_exit(p) or "terminal_unknown"})
            except Exception: outcome["terminals"].append({"terminal":"aborted","exit":"terminal_unknown"})
        held.close(); write_json(os.path.join(args.root,"native-exit-receipt.json"),outcome)
if __name__=="__main__": main()
