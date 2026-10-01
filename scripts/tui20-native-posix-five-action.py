"""Native Linux/macOS PTY evidence for Issue 20; credentials remain process-memory only."""
import argparse, hashlib, json, os, pty, select, stat, subprocess, time, urllib.request

ALLOWED_ENV=("HOME","LANG","LC_ALL","PATH","SHELL","TERM","TMPDIR","USER")
def write_json(path,value):
    with open(path,"w",encoding="utf-8",newline="") as out: json.dump(value,out,separators=(",",":"))
def request(url,token,path):
    with urllib.request.urlopen(urllib.request.Request(url+path,headers={"Authorization":"Bearer "+token}),timeout=15) as response: return json.load(response)
def operations(url,token): return request(url,token,"/api/operator/lifecycle/operations")["operations"]
def audit_count(url,token):
    value=request(url,token,"/api/audit"); events=value.get("events",value.get("audit",[]))
    return sum(1 for item in events if isinstance(item,dict) and item.get("action")=="mcp.action.denied" and item.get("actor")=="tui20-native-operator")
def unrelated(url,token):
    value=request(url,token,"/api/services"); entries=value.get("services",value) if isinstance(value,dict) else value
    item=next((x for x in entries if isinstance(x,dict) and x.get("id")=="tui20-unrelated"),None)
    availability=request(url,token,"/api/operator/lifecycle/services/tui20-unrelated/availability")
    if item is None: raise RuntimeError("unrelated service missing")
    runtime=availability.get("runtime",{})
    return {"servicePresent":True,"id":item.get("id"),"enabled":item.get("enabled"),"runtimeState":runtime.get("state"),"running":runtime.get("running"),"availability":availability.get("available")}
class Terminal:
    def __init__(self, executable, profile, env, log):
        self.master,self.slave=pty.openpty(); self.log=log
        self.child=subprocess.Popen([executable,"--profile",profile],cwd=os.path.dirname(executable),env=env,stdin=self.slave,stdout=self.slave,stderr=self.slave,close_fds=True)
        os.close(self.slave); self.text=""
    def write(self,value): os.write(self.master,value.encode())
    def read(self):
        ready,_,_=select.select([self.master],[],[],.1)
        if ready:
            try: chunk=os.read(self.master,65536).decode("utf-8","replace")
            except OSError: chunk=""
            self.text+=chunk
            with open(self.log,"a",encoding="utf-8",newline="") as out: out.write(chunk)
    def wait(self,need,seconds,start=0):
        until=time.monotonic()+seconds
        while time.monotonic()<until:
            if all(value in self.text[start:] for value in need): return
            self.read()
            if self.child.poll() is not None: raise RuntimeError("terminal exited before expected state")
        raise RuntimeError("missing terminal state")
    def close(self,expected=0):
        if self.child.poll() is None: self.write("q")
        try: result=self.child.wait(timeout=10)
        except subprocess.TimeoutExpired: self.child.kill(); self.child.wait(); raise RuntimeError("terminal exit was not observed")
        os.close(self.master)
        if result!=expected: raise RuntimeError("unexpected terminal exit")
        return "terminal_exited_zero" if result==0 else "terminal_exit_code_2" if result==2 else "terminal_exited_nonzero"
def hold_candidate(executable,commit):
    info=os.lstat(executable)
    if os.name=="nt" or not stat.S_ISREG(info.st_mode) or stat.S_ISLNK(info.st_mode) or len(commit)!=40: raise RuntimeError("candidate executable invalid")
    fd=os.open(executable,os.O_RDONLY); digest=hashlib.sha256()
    while True:
        data=os.read(fd,1024*1024)
        if not data: break
        digest.update(data)
    os.lseek(fd,0,os.SEEK_SET)
    return fd,{"sourceCommit":commit,"binarySHA256":digest.hexdigest()}
def detail(term): term.write("/tui20-fixture\r"); term.wait(("tui20-fixture",),20); term.write("\r"); term.wait(("Lifecycle:","tui20-fixture"),20)
def action(term,key,name):
    start=len(term.text); term.write(key); term.wait(("Core preview: "+name,"Confirm "+name),45,start); frozen=term.text; term.write("rj?q"); time.sleep(.3)
    if "Confirm "+name not in term.text or "Core preview: "+name not in term.text: raise RuntimeError("confirmation changed")
    term.write("yr"); deadline=time.monotonic()+90
    while time.monotonic()<deadline:
        if "Core operation" in term.text[start:] and "succeeded." in term.text[start:]: return
        if "Runtime API unavailable:" in term.text[start:]: term.write("r")
        term.read()
    raise RuntimeError("retained operation readback unavailable")
def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--root",required=True); parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True); args=parser.parse_args()
    with open(os.path.join(args.root,"ready.json"),encoding="utf-8") as stream: ready=json.load(stream)
    paths={"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True}
    if ready.get("coreCommit")!=args.core_commit or ready.get("runtimePathReceipt")!=paths: raise RuntimeError("Core fixture receipt invalid")
    token=os.environ.get("SERVICE_LASSO_TUI20_TOKEN"); denied=os.environ.get("SERVICE_LASSO_TUI20_DENIED_TOKEN"); url=os.environ.get("SERVICE_LASSO_TUI20_API_URL")
    if not token or not denied or not url: raise RuntimeError("in-memory credential handoff unavailable")
    env={key:os.environ[key] for key in ALLOWED_ENV if os.environ.get(key)}; env.update({"TERM":"xterm-256color","SERVICE_LASSO_API_TOKEN":token,"SERVICE_LASSO_DENIED_TOKEN":denied,"SERVICE_LASSO_INVALID_TOKEN":"tui20-invalid-token","SERVICE_LASSO_CONNECTIONS_CONFIG":os.path.join(args.root,"connections.json")})
    held,identity=hold_candidate(args.executable,args.source_commit); log=os.path.join(args.root,"native-terminal.txt"); outcome={"outcome":"failed","candidateIdentity":identity,"coreCommit":args.core_commit,"runtimePathReceipt":paths,"terminals":[]}
    try:
        before=unrelated(url,token); adverse=[]
        for profile,label,expected,audit_expected in (("missing","missing-credential",2,0),("invalid","invalid-credential",0,0),("denied","scope-denied",0,5)):
            count=len(operations(url,token)); audit=audit_count(url,token); term=Terminal(args.executable,profile,env,log)
            if profile=="missing": term.wait(('connection profile "missing" credential is unavailable',),30)
            else: term.wait(("Runtime identity:",),30); detail(term); term.write("i"); term.wait(("Runtime API unavailable:",),30)
            outcome["terminals"].append({"terminal":label,"exit":term.close(expected)})
            after=len(operations(url,token)); audit_after=audit_count(url,token); adverse.append({"case":label,"beforeOperationCount":count,"afterOperationCount":after,"noOperation":after==count,"coreDeniedAuditBefore":audit,"coreDeniedAuditAfter":audit_after,"coreDeniedAuditDelta":audit_after-audit})
            if after!=count or audit_after-audit!=audit_expected: raise RuntimeError("adverse lifecycle receipt invalid")
        term=Terminal(args.executable,"native",env,log); term.wait(("Runtime identity:",),30); detail(term)
        for key,name in (("i","install"),("c","config"),("s","start"),("x","stop"),("R","restart")): action(term,key,name)
        before_reload=len(operations(url,token)); term.write("l"); term.wait(("reload is unavailable",),15)
        if len(operations(url,token))!=before_reload or "z cancel" in term.text: raise RuntimeError("reload or cancellation contract changed")
        outcome["terminals"].append({"terminal":"allowed","exit":term.close()})
        term=Terminal(args.executable,"native",env,log); term.wait(("Runtime identity:",),30); detail(term); outcome["terminals"].append({"terminal":"reconnect","exit":term.close()})
        records=[{key:record.get(key) for key in ("operationId","action","targetIds","status","outcome","cancellationSupported")} for record in operations(url,token) if record.get("targetIds")==["tui20-fixture"]]
        expected=["service_restart","service_stop","service_start","service_configure","service_install"]
        if len(records)!=5 or sorted(item["action"] for item in records)!=sorted(expected) or any(not item["operationId"] or item["cancellationSupported"] for item in records): raise RuntimeError("durable action audit invalid")
        if unrelated(url,token)!=before: raise RuntimeError("unrelated state changed")
        write_json(os.path.join(args.root,"operation-audit.json"),{"operations":records}); outcome.update({"outcome":"succeeded","actions":["install","config","start","stop","restart"],"adverseAudit":adverse,"reloadDenied":True,"cancellation":{"advertised":False,"supportedActionTested":False},"reconnect":{"completedOperationNoReplay":True,"pendingReconciliation":"blocked_core_1553_no_adapter"},"unrelatedService":{**before,"unchangedAfterFiveActions":True},"fixtureOperationCount":len(records)})
    except Exception: outcome["failureReason"]="native_harness_assertion_failed"; raise
    finally: os.close(held); write_json(os.path.join(args.root,"native-exit-receipt.json"),outcome)
if __name__=="__main__": main()
