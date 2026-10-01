"""Native Linux/macOS PTY evidence for Issue 20; credentials remain process-memory only."""
import argparse, ctypes, fcntl, hashlib, json, os, pty, select, stat, subprocess, sys, time, urllib.request

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
class BoundProcess:
    """A child launched from the already-verified executable descriptor."""
    def __init__(self, pid): self.pid=pid; self.returncode=None
    def poll(self):
        if self.returncode is not None: return self.returncode
        pid,status=os.waitpid(self.pid,os.WNOHANG)
        if not pid: return None
        self.returncode=os.waitstatus_to_exitcode(status); return self.returncode
    def wait(self,timeout):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            result=self.poll()
            if result is not None: return result
            time.sleep(.02)
        raise subprocess.TimeoutExpired("held executable",timeout)
    def kill(self): os.kill(self.pid,9)
def fexecve_child(fd,argv,env,master,slave):
    pid=os.fork()
    if pid:
        os.close(slave); return BoundProcess(pid)
    try:
        os.close(master); os.dup2(slave,0); os.dup2(slave,1); os.dup2(slave,2)
        if slave>2: os.close(slave)
        values=[value.encode() for value in argv]; argp=(ctypes.c_char_p*(len(values)+1))(*values,None)
        entries=[(key+"="+value).encode() for key,value in env.items()]; envp=(ctypes.c_char_p*(len(entries)+1))(*entries,None)
        libc=ctypes.CDLL(None,use_errno=True); libc.fexecve.argtypes=(ctypes.c_int,ctypes.POINTER(ctypes.c_char_p),ctypes.POINTER(ctypes.c_char_p)); libc.fexecve.restype=ctypes.c_int
        libc.fexecve(fd,argp,envp)
        failure=ctypes.get_errno(); os.write(2,("held fexecve failed: "+os.strerror(failure)+"\n").encode())
    finally: os._exit(127)
class Terminal:
    def __init__(self, held, executable, profile, env, log):
        self.master,self.slave=pty.openpty(); self.log=log
        args=[executable,"--profile",profile]
        if sys_platform()=="linux": self.child=fexecve_child(held,args,env,self.master,self.slave)
        elif sys_platform()=="darwin":
            # Darwin exposes its per-process descriptor namespace through fdescfs.
            # Opening /dev/fd/N duplicates the held descriptor, so exec resolves the
            # same vnode rather than a replacement at the original pathname.
            launch="/dev/fd/"+str(held)
            if not os.path.exists(launch): raise RuntimeError("Darwin held-descriptor execution unavailable")
            self.child=subprocess.Popen([executable,"--profile",profile],executable=launch,cwd=os.path.dirname(executable),env=env,stdin=self.slave,stdout=self.slave,stderr=self.slave,close_fds=True,pass_fds=(held,))
            os.close(self.slave)
        else: raise RuntimeError("native held-executable launch unsupported on this platform")
        self.text=""
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
def sys_platform(): return sys.platform
def sha256_fd(fd):
    digest=hashlib.sha256(); os.lseek(fd,0,os.SEEK_SET)
    while True:
        data=os.read(fd,1024*1024)
        if not data: break
        digest.update(data)
    os.lseek(fd,0,os.SEEK_SET)
    return digest.hexdigest()
def hold_candidate(executable,commit):
    info=os.lstat(executable)
    if os.name=="nt" or not stat.S_ISREG(info.st_mode) or stat.S_ISLNK(info.st_mode) or len(commit)!=40: raise RuntimeError("candidate executable invalid")
    fd=os.open(executable,os.O_RDONLY|getattr(os,"O_NOFOLLOW",0))
    try:
        opened=os.fstat(fd)
        if not stat.S_ISREG(opened.st_mode) or (opened.st_dev,opened.st_ino)!=(info.st_dev,info.st_ino): raise RuntimeError("candidate executable changed during acquisition")
        return fd,{"sourceCommit":commit,"binarySHA256":sha256_fd(fd)},(info.st_dev,info.st_ino)
    except Exception:
        os.close(fd); raise
def linux_sealed_execution(held,identity):
    """Copy verified bytes into a kernel-sealed anonymous executable object."""
    required=("MFD_ALLOW_SEALING","F_ADD_SEALS","F_GET_SEALS","F_SEAL_WRITE","F_SEAL_GROW","F_SEAL_SHRINK","F_SEAL_SEAL")
    if not hasattr(os,"memfd_create") or any(not hasattr(fcntl if name.startswith("F_") else os,name) for name in required): raise RuntimeError("Linux sealed execution unavailable")
    sealed=os.memfd_create("tui20-native-exec",os.MFD_ALLOW_SEALING)
    try:
        os.lseek(held,0,os.SEEK_SET)
        while True:
            chunk=os.read(held,1024*1024)
            if not chunk: break
            os.write(sealed,chunk)
        if sha256_fd(sealed)!=identity["binarySHA256"]: raise RuntimeError("sealed executable digest mismatch")
        seals=fcntl.F_SEAL_WRITE|fcntl.F_SEAL_GROW|fcntl.F_SEAL_SHRINK|fcntl.F_SEAL_SEAL
        fcntl.fcntl(sealed,fcntl.F_ADD_SEALS,seals)
        if fcntl.fcntl(sealed,fcntl.F_GET_SEALS)&seals!=seals: raise RuntimeError("sealed executable immutability unavailable")
        try: os.pwrite(sealed,b"X",0)
        except OSError: pass
        else: raise RuntimeError("sealed executable accepted an in-place write")
        return sealed,{"platform":"linux","mechanism":"memfd-fexecve-seals","seals":["write","grow","shrink","seal"],"inPlaceWriteDenied":True}
    except Exception:
        os.close(sealed); raise
def darwin_system_immutable_execution(held,identity,root):
    """Use the Darwin system immutable flag; owner flags are intentionally rejected."""
    if not hasattr(os.stat_result,"st_flags"): raise RuntimeError("Darwin system immutable flag readback unavailable")
    protected=os.path.join(root,".tui20-system-immutable-exec")
    if os.path.lexists(protected): raise RuntimeError("Darwin protected executable path already exists")
    output=None
    try:
        with open(protected,"xb") as out:
            os.lseek(held,0,os.SEEK_SET)
            while True:
                chunk=os.read(held,1024*1024)
                if not chunk: break
                out.write(chunk)
        os.chmod(protected,0o700)
        protected_fd=os.open(protected,os.O_RDONLY)
        if sha256_fd(protected_fd)!=identity["binarySHA256"]: raise RuntimeError("Darwin protected executable digest mismatch")
        result=subprocess.run(["sudo","-n","/usr/bin/chflags","schg",protected],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
        if result.returncode!=0: raise RuntimeError("Darwin system immutable activation unavailable")
        system_immutable=getattr(stat,"SF_IMMUTABLE",0x00020000)
        if not os.stat(protected).st_flags&system_immutable: raise RuntimeError("Darwin system immutable readback failed")
        try: os.open(protected,os.O_RDWR)
        except OSError: pass
        else: raise RuntimeError("Darwin system immutable executable accepted an in-place write")
        return protected_fd,protected,{"platform":"darwin","mechanism":"system-immutable-fdescfs","systemImmutable":True,"inPlaceWriteDenied":True}
    except Exception:
        if 'protected_fd' in locals(): os.close(protected_fd)
        if os.path.lexists(protected):
            subprocess.run(["sudo","-n","/usr/bin/chflags","noschg",protected],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
            try: os.unlink(protected)
            except OSError: pass
        raise
def release_darwin_system_immutable_execution(protected_fd,protected):
    os.close(protected_fd)
    result=subprocess.run(["sudo","-n","/usr/bin/chflags","noschg",protected],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
    if result.returncode!=0: raise RuntimeError("Darwin protected executable release unavailable")
    os.unlink(protected)
def bound_execution(held,identity,root):
    if sys_platform()=="linux":
        fd,binding=linux_sealed_execution(held,identity); return fd,None,binding
    if sys_platform()=="darwin":
        fd,path,binding=darwin_system_immutable_execution(held,identity,root); return fd,path,binding
    raise RuntimeError("native immutable execution unavailable on this platform")
def bound_replacement_probe(launch_fd,held,executable,identity,profile,env,log):
    """Prove pathname replacement and mutable source bytes cannot alter the sealed launch."""
    directory=os.path.dirname(executable); original=os.path.join(directory,".tui20-held-original"); replacement=os.path.join(directory,".tui20-untrusted-replacement")
    if os.path.exists(original) or os.path.lexists(replacement): raise RuntimeError("bound-launch probe paths already exist")
    os.link(executable,original)
    results=[]
    try:
        for name,reparse in (("rename",False),("reparse",True)):
            with open(replacement,"wb") as stream: stream.write(b"untrusted replacement must never execute\n")
            os.chmod(replacement,0o700)
            if reparse:
                staged=replacement+".link"; os.symlink(replacement,staged); os.replace(staged,executable)
            else: os.replace(replacement,executable)
            named=os.lstat(executable)
            held_stat=os.fstat(held)
            if (held_stat.st_dev,held_stat.st_ino)!=(identity[0],identity[1]) or (not reparse and (named.st_dev,named.st_ino)==identity): raise RuntimeError("held executable binding changed")
            term=Terminal(launch_fd,executable,profile,env,log); term.wait(('connection profile "missing" credential is unavailable',),30); exit_value=term.close(2)
            results.append({"attack":name,"heldCandidateLaunch":True,"terminalExit":exit_value})
            os.replace(original,executable); os.link(executable,original)
        modifier=os.open(executable,os.O_RDWR)
        try:
            original_byte=os.pread(modifier,1,0)
            if len(original_byte)!=1: raise RuntimeError("source candidate mutation probe unavailable")
            os.pwrite(modifier,bytes([original_byte[0]^0x01]),0); os.fsync(modifier)
            if sha256_fd(held)==identity["binarySHA256"]: raise RuntimeError("source candidate mutation probe did not alter held inode")
            term=Terminal(launch_fd,executable,profile,env,log); term.wait(('connection profile "missing" credential is unavailable',),30); exit_value=term.close(2)
            results.append({"attack":"inplace-content-mutation","heldCandidateLaunch":True,"terminalExit":exit_value,"sourceDigestChanged":True})
        finally:
            if 'original_byte' in locals(): os.pwrite(modifier,original_byte,0); os.fsync(modifier)
            os.close(modifier)
        if sha256_fd(held)!=identity["binarySHA256"]: raise RuntimeError("source candidate mutation restoration failed")
    finally:
        if os.path.lexists(replacement): os.unlink(replacement)
        if os.path.lexists(original): os.replace(original,executable)
    return results
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
    held,identity,held_inode=hold_candidate(args.executable,args.source_commit); launch_fd=None; darwin_protected=None; log=os.path.join(args.root,"native-terminal.txt"); outcome={"outcome":"failed","candidateIdentity":identity,"coreCommit":args.core_commit,"runtimePathReceipt":paths,"terminals":[]}
    try:
        launch_fd,darwin_protected,binding=bound_execution(held,identity,args.root); outcome["heldExecutableBinding"]=binding
        before=unrelated(url,token); adverse=[]
        for profile,label,expected,audit_expected in (("missing","missing-credential",2,0),("invalid","invalid-credential",0,0),("denied","scope-denied",0,5)):
            count=len(operations(url,token)); audit=audit_count(url,token)
            if profile=="missing":
                outcome["heldExecutableBinding"]["replacementProbe"]=bound_replacement_probe(launch_fd,held,args.executable,held_inode,profile,env,log)
                term=Terminal(launch_fd,args.executable,profile,env,log); term.wait(('connection profile "missing" credential is unavailable',),30)
            else:
                term=Terminal(launch_fd,args.executable,profile,env,log); term.wait(("Runtime identity:",),30); detail(term); term.write("i"); term.wait(("Runtime API unavailable:",),30)
            outcome["terminals"].append({"terminal":label,"exit":term.close(expected)})
            after=len(operations(url,token)); audit_after=audit_count(url,token); adverse.append({"case":label,"beforeOperationCount":count,"afterOperationCount":after,"noOperation":after==count,"coreDeniedAuditBefore":audit,"coreDeniedAuditAfter":audit_after,"coreDeniedAuditDelta":audit_after-audit})
            if after!=count or audit_after-audit!=audit_expected: raise RuntimeError("adverse lifecycle receipt invalid")
        term=Terminal(launch_fd,args.executable,"native",env,log); term.wait(("Runtime identity:",),30); detail(term)
        action_readbacks=[]; known_ids={record.get("operationId") for record in operations(url,token)}
        expected_by_ui={"install":"service_install","config":"service_configure","start":"service_start","stop":"service_stop","restart":"service_restart"}
        for key,name in (("i","install"),("c","config"),("s","start"),("x","stop"),("R","restart")):
            action(term,key,name)
            current=operations(url,token); added=[record for record in current if record.get("operationId") not in known_ids]
            if len(added)!=1 or added[0].get("action")!=expected_by_ui[name] or added[0].get("targetIds")!=["tui20-fixture"]: raise RuntimeError("per-action durable readback invalid")
            known_ids.add(added[0]["operationId"]); action_readbacks.append({key:added[0].get(key) for key in ("operationId","action","targetIds","status","outcome","cancellationSupported")})
        before_reload=operations(url,token); term.write("l"); term.wait(("reload is unavailable",),15)
        if operations(url,token)!=before_reload or "z cancel" in term.text: raise RuntimeError("reload or cancellation contract changed")
        outcome["terminals"].append({"terminal":"allowed","exit":term.close()})
        term=Terminal(launch_fd,args.executable,"native",env,log); term.wait(("Runtime identity:",),30); detail(term); outcome["terminals"].append({"terminal":"reconnect","exit":term.close()})
        records=[{key:record.get(key) for key in ("operationId","action","targetIds","status","outcome","cancellationSupported")} for record in operations(url,token) if record.get("targetIds")==["tui20-fixture"]]
        expected=["service_restart","service_stop","service_start","service_configure","service_install"]
        operation_ids=[item["operationId"] for item in records]
        audit_events=request(url,token,"/api/audit").get("events",[])
        audit_matches=[]
        for record in records:
            matches=[event for event in audit_events if isinstance(event,dict) and event.get("action")=="mcp.operation.succeeded" and event.get("actor")=="tui20-native-operator" and event.get("subject")==record["operationId"] and isinstance(event.get("metadata"),dict) and event["metadata"].get("operationId")==record["operationId"] and event["metadata"].get("action")==record["action"] and event["metadata"].get("targetIds")==record["targetIds"]]
            audit_matches.append({"operationId":record["operationId"],"coreAuditCount":len(matches)})
        if len(records)!=5 or sorted(records,key=lambda item:item["operationId"])!=sorted(action_readbacks,key=lambda item:item["operationId"]) or len(set(operation_ids))!=5 or sorted(item["action"] for item in records)!=sorted(expected) or any(not item["operationId"] or item["cancellationSupported"] for item in records) or any(item["coreAuditCount"]!=1 for item in audit_matches): raise RuntimeError("durable action audit invalid")
        if unrelated(url,token)!=before: raise RuntimeError("unrelated state changed")
        write_json(os.path.join(args.root,"operation-audit.json"),{"operations":records,"coreAudit":audit_matches}); outcome.update({"outcome":"succeeded","actions":["install","config","start","stop","restart"],"adverseAudit":adverse,"reloadDenied":True,"cancellation":{"advertised":False,"supportedActionTested":False},"reconnect":{"completedOperationNoReplay":True,"pendingReconciliation":"blocked_core_1553_no_adapter"},"unrelatedService":{**before,"unchangedAfterFiveActions":True},"fixtureOperationCount":len(records)})
    except Exception: outcome["failureReason"]="native_harness_assertion_failed"; raise
    finally:
        if darwin_protected is not None: release_darwin_system_immutable_execution(launch_fd,darwin_protected)
        elif launch_fd is not None: os.close(launch_fd)
        os.close(held); write_json(os.path.join(args.root,"native-exit-receipt.json"),outcome)
if __name__=="__main__": main()
