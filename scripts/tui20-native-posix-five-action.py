"""Native Linux/macOS PTY evidence for Issue 20; credentials remain process-memory only."""
import argparse, ctypes, fcntl, hashlib, json, os, pty, select, signal, stat, subprocess, sys, time, urllib.request

ALLOWED_ENV=("HOME","LANG","LC_ALL","PATH","SHELL","TERM","TMPDIR","USER")
OWNED_TERMINALS=[]
def write_json(path,value):
    with open(path,"w",encoding="utf-8",newline="") as out:
        json.dump(value,out,separators=(",",":")); out.flush(); os.fsync(out.fileno())
def persist_primary_outcome(root,outcome,writer=write_json):
    """Durably retain the closed primary result before immutable-object teardown."""
    writer(os.path.join(root,"native-exit-receipt.json"),outcome)
def persist_cleanup_outcome(root,outcome,writer=write_json):
    """Retain a separate closed cleanup result without rewriting the primary result."""
    writer(os.path.join(root,"native-cleanup-receipt.json"),outcome)
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
    def __init__(self, pid): self.pid=pid; self.returncode=None; self.reaped_unowned=False
    def poll(self):
        if self.returncode is not None: return self.returncode
        if self.reaped_unowned: return None
        try: pid,status=os.waitpid(self.pid,os.WNOHANG)
        except ChildProcessError:
            self.reaped_unowned=True; return None
        except OSError as error:
            if error.errno==getattr(os,"ECHILD",10): self.reaped_unowned=True; return None
            raise
        if not pid: return None
        self.returncode=os.waitstatus_to_exitcode(status); return self.returncode
    def wait(self,timeout):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            result=self.poll()
            if result is not None: return result
            if self.reaped_unowned: raise ChildReapedUnowned("owned child was reaped outside BoundProcess")
            time.sleep(.02)
        raise subprocess.TimeoutExpired("held executable",timeout)
    def wait_until_observed(self):
        """No second deadline: the owner stays alive until it observes its child."""
        while True:
            result=self.poll()
            if result is not None: return result
            if self.reaped_unowned: raise ChildReapedUnowned("owned child was reaped outside BoundProcess")
            time.sleep(.02)
def terminal_exit_reason(result):
    if result is None: return "terminal_unknown"
    if result < 0: return "terminal_signaled"
    return "terminal_exited_zero" if result==0 else "terminal_exit_code_1" if result==1 else "terminal_exit_code_2" if result==2 else "terminal_exited_nonzero"
class ChildReapedUnowned(RuntimeError): pass
class TerminalUnresolved(RuntimeError):
    def __init__(self, terminal, reason): super().__init__(reason); self.terminal=terminal
def retain_terminal_child(terminal,pid):
    child=BoundProcess(pid); child.tui20_birth=None
    if terminal is not None:
        terminal.child=child
        OWNED_TERMINALS.append(terminal)
    return child
def fexecve_child(fd,argv,env,master,slave,terminal=None):
    pid=os.fork()
    if pid:
        child=retain_terminal_child(terminal,pid)
        os.close(slave); return child
    try:
        os.close(master); os.dup2(slave,0); os.dup2(slave,1); os.dup2(slave,2)
        if slave>2: os.close(slave)
        values=[value.encode() for value in argv]; argp=(ctypes.c_char_p*(len(values)+1))(*values,None)
        entries=[(key+"="+value).encode() for key,value in env.items()]; envp=(ctypes.c_char_p*(len(entries)+1))(*entries,None)
        libc=ctypes.CDLL(None,use_errno=True); libc.fexecve.argtypes=(ctypes.c_int,ctypes.POINTER(ctypes.c_char_p),ctypes.POINTER(ctypes.c_char_p)); libc.fexecve.restype=ctypes.c_int
        libc.fexecve(fd,argp,envp)
        failure=ctypes.get_errno(); os.write(2,("held fexecve failed: "+os.strerror(failure)+"\n").encode())
    finally: os._exit(127)
def darwin_execve_child(launch,argv,env,master,slave,terminal=None):
    """Execute the protected leaf only after resolving it through the held directory."""
    pid=os.fork()
    if pid:
        child=retain_terminal_child(terminal,pid)
        os.close(slave); return child
    try:
        os.close(master); os.dup2(slave,0); os.dup2(slave,1); os.dup2(slave,2)
        if slave>2: os.close(slave)
        os.fchdir(launch["directoryFD"])
        named=os.lstat(launch["leaf"])
        if (named.st_dev,named.st_ino)!=launch["leafIdentity"] or named.st_flags&launch["systemImmutable"]==0:
            raise RuntimeError("Darwin protected executable binding changed")
        os.execve("./"+launch["leaf"],argv,env)
    except Exception:
        os.write(2,b"held Darwin execve failed\n")
    finally: os._exit(127)
class Terminal:
    def __init__(self, held, executable, profile, env):
        self.master,self.slave=pty.openpty()
        self.text=""; self.label=profile; self.exit_reason=None; self.child=None
        args=[executable,"--profile",profile]
        if sys_platform()=="linux": self.child=fexecve_child(held,args,env,self.master,self.slave,self)
        elif sys_platform()=="darwin":
            self.child=darwin_execve_child(held,args,env,self.master,self.slave,self)
        else: raise RuntimeError("native held-executable launch unsupported on this platform")
        # Registration precedes every fallible post-fork observation. Unknown
        # birth stays owned and cannot be used as proof of process absence.
        self.child.tui20_birth=process_birth(self.child.pid)
    def write(self,value): os.write(self.master,value.encode())
    def read(self):
        ready,_,_=select.select([self.master],[],[],.1)
        if ready:
            try: chunk=os.read(self.master,65536).decode("utf-8","replace")
            except OSError: chunk=""
            # Terminal bytes are required in memory for the bounded interaction
            # assertions, but are never written to a receipt, artifact, or log.
            self.text+=chunk
    def wait(self,need,seconds,start=0):
        until=time.monotonic()+seconds
        while time.monotonic()<until:
            if all(value in self.text[start:] for value in need): return
            self.read()
            if self.child.poll() is not None: raise RuntimeError("terminal exited before expected state")
            if self.child.reaped_unowned:
                self.exit_reason="terminal_unknown"; self.recovery_state="child_reaped_unowned"
                raise TerminalUnresolved(self,"owned terminal was reaped without an observable exit status")
        raise RuntimeError("missing terminal state")
    def close(self,expected=0):
        current=self.child.poll()
        if current is None and not self.child.reaped_unowned: self.write("q")
        try: result=self.child.wait(timeout=10)
        except ChildReapedUnowned:
            self.exit_reason="terminal_unknown"; self.recovery_state="child_reaped_unowned"
            raise TerminalUnresolved(self,"owned terminal was reaped without an observable exit status")
        except subprocess.TimeoutExpired:
            # The helper stays alive as the recovery owner; do not close its
            # PTY or execution object while an owned terminal remains live.
            self.exit_reason="terminal_unknown"
            self.recovery_state="live_child_unresolved"
            raise TerminalUnresolved(self,"owned terminal exit unresolved")
        os.close(self.master)
        reason=terminal_exit_reason(result)
        self.exit_reason=reason
        if result!=expected: raise RuntimeError("unexpected terminal exit")
        return reason
    def recover_until_observed(self):
        """Keep this helper, its PTY, and its held launch object alive until exit."""
        if self.child.reaped_unowned:
            os.close(self.master)
            return {"terminal":self.label,"outcome":"terminal_unknown","recovery":"child_reaped_unowned","observed":False}
        try: result=self.child.wait_until_observed()
        except ChildReapedUnowned:
            self.exit_reason="terminal_unknown"; self.recovery_state="child_reaped_unowned"
            os.close(self.master)
            return {"terminal":self.label,"outcome":"terminal_unknown","recovery":"child_reaped_unowned","observed":False}
        os.close(self.master)
        self.exit_reason=terminal_exit_reason(result)
        return {"terminal":self.label,"outcome":self.exit_reason,"recovery":"owned_exit_observed","observed":True}
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
class DarwinActivationFailure(RuntimeError):
    def __init__(self,recovery): super().__init__("Darwin system immutable activation unavailable"); self.recovery=recovery
def darwin_system_immutable_execution(held,identity,root):
    """Bind execve to a system-immutable leaf reached from its held parent."""
    if not hasattr(os.stat_result,"st_flags"): raise RuntimeError("Darwin system immutable flag readback unavailable")
    directory=os.path.join(root,".tui20-system-immutable-exec-dir")
    protected=os.path.join(directory,"launch")
    if os.path.lexists(directory): raise RuntimeError("Darwin protected executable directory already exists")
    protected_writer=None; protected_fd=None; directory_fd=None; leaf_activation=False
    try:
        os.mkdir(directory,0o700)
        with open(protected,"xb") as out:
            os.lseek(held,0,os.SEEK_SET)
            while True:
                chunk=os.read(held,1024*1024)
                if not chunk: break
                out.write(chunk)
        os.chmod(protected,0o700)
        # Darwin must deny a writer that was already open before SF_IMMUTABLE;
        # a later O_RDWR-open denial alone would not establish that property.
        protected_writer=os.open(protected,os.O_RDWR)
        protected_fd=os.open(protected,os.O_RDONLY)
        directory_fd=os.open(directory,os.O_RDONLY|getattr(os,"O_DIRECTORY",0))
        system_immutable=getattr(stat,"SF_IMMUTABLE",0x00020000)
        result=subprocess.run(["sudo","-n","/usr/bin/chflags","schg",protected],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
        if result.returncode!=0: raise RuntimeError("Darwin leaf system immutable activation unavailable")
        if not os.fstat(protected_fd).st_flags&system_immutable: raise RuntimeError("Darwin leaf system immutable readback failed")
        leaf=os.fstat(protected_fd)
        recovery={"leafFD":protected_fd,"directoryFD":directory_fd,"leaf":"launch","leafIdentity":(leaf.st_dev,leaf.st_ino),"systemImmutable":system_immutable,"path":protected,"directory":directory}
        leaf_activation=True
        try: os.open(protected,os.O_RDWR)
        except OSError: pass
        else: raise RuntimeError("Darwin system immutable executable accepted an in-place write")
        try: os.pwrite(protected_writer,b"X",0)
        except OSError: pass
        else: raise RuntimeError("Darwin system immutable executable accepted an existing-writer in-place write")
        if sha256_fd(protected_fd)!=identity["binarySHA256"]: raise RuntimeError("Darwin immutable leaf digest mismatch")
        result=subprocess.run(["sudo","-n","/usr/bin/chflags","schg",directory],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
        if result.returncode!=0: raise RuntimeError("Darwin parent system immutable activation unavailable")
        if not os.fstat(directory_fd).st_flags&system_immutable: raise RuntimeError("Darwin parent system immutable readback failed")
        os.close(protected_writer); protected_writer=None
        return recovery,{"platform":"darwin","mechanism":"system-immutable-held-directory-execve","systemImmutableLeaf":True,"systemImmutableParent":True,"inPlaceWriteDenied":True,"existingWriterDenied":True,"immutableLeafDigestVerified":True}
    except Exception:
        if locals().get('protected_writer') is not None: os.close(protected_writer)
        if leaf_activation:
            # Do not attempt suppressed pathname rollback here.  The held
            # descriptor is carried to primary-before-cleanup finalization.
            raise DarwinActivationFailure(recovery)
        if protected_fd is not None: os.close(protected_fd)
        if directory_fd is not None: os.close(directory_fd)
        if os.path.lexists(protected): subprocess.run(["sudo","-n","/usr/bin/chflags","noschg",protected],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
        if os.path.lexists(directory): subprocess.run(["sudo","-n","/usr/bin/chflags","noschg",directory],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
        if os.path.lexists(protected):
            try: os.unlink(protected)
            except OSError: pass
        if os.path.lexists(directory):
            try: os.rmdir(directory)
            except OSError: pass
        raise
def release_darwin_system_immutable_execution(launch):
    """Return closed cleanup evidence; leave the protected artifact on failure."""
    result=subprocess.run(["sudo","-n","/usr/bin/chflags","noschg",launch["path"]],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
    if result.returncode!=0: return {"outcome":"failed","closedReason":"leaf_release_failed","recoveryRetained":True}
    if os.stat(launch["path"]).st_flags&launch["systemImmutable"]: return {"outcome":"failed","closedReason":"leaf_release_readback_failed","recoveryRetained":True}
    result=subprocess.run(["sudo","-n","/usr/bin/chflags","noschg",launch["directory"]],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=False)
    if result.returncode!=0: return {"outcome":"failed","closedReason":"parent_release_failed","recoveryRetained":True}
    if os.stat(launch["directory"]).st_flags&launch["systemImmutable"]: return {"outcome":"failed","closedReason":"parent_release_readback_failed","recoveryRetained":True}
    try:
        os.close(launch["leafFD"]); os.close(launch["directoryFD"])
        os.unlink(launch["path"]); os.rmdir(launch["directory"])
    except OSError:
        return {"outcome":"failed","closedReason":"protected_artifact_removal_failed","recoveryRetained":True}
    return {"outcome":"succeeded","closedReason":"released_and_removed","recoveryRetained":False}
def cleanup_execution(launch_fd,darwin_protected,held,live_child=False):
    try:
        if live_child: return {"outcome":"failed","closedReason":"live_child_unresolved","recoveryRetained":True}
        if darwin_protected is not None: result=release_darwin_system_immutable_execution(darwin_protected)
        elif launch_fd is not None:
            os.close(launch_fd); result={"outcome":"succeeded","closedReason":"sealed_descriptor_closed","recoveryRetained":False}
        else: result={"outcome":"succeeded","closedReason":"no_execution_object","recoveryRetained":False}
        if held is not None: os.close(held)
        return result
    except OSError:
        return {"outcome":"failed","closedReason":"descriptor_close_failed","recoveryRetained":darwin_protected is not None}
def finalize_native_outcome(root,outcome,launch_fd,darwin_protected,held,live_child=False,primary_persisted=False,recovery_observation=None,cleanup=cleanup_execution,writer=write_json):
    """Record primary state first; cleanup recording can never mask that state."""
    if live_child: raise RuntimeError("external recovery owner must observe a live child before finalization")
    if not primary_persisted: persist_primary_outcome(root,outcome,writer)
    cleanup_outcome=cleanup(launch_fd,darwin_protected,held,live_child)
    if recovery_observation is not None: cleanup_outcome={**cleanup_outcome,"recoveryObservation":recovery_observation}
    try: persist_cleanup_outcome(root,cleanup_outcome,writer)
    except OSError: pass
    return cleanup_outcome
def bound_execution(held,identity,root):
    if sys_platform()=="linux":
        fd,binding=linux_sealed_execution(held,identity); return fd,None,binding
    if sys_platform()=="darwin":
        launch,binding=darwin_system_immutable_execution(held,identity,root); return launch,launch,binding
    raise RuntimeError("native immutable execution unavailable on this platform")
def bound_replacement_probe(launch_fd,held,executable,inode_identity,digest_identity,profile,env):
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
            if (held_stat.st_dev,held_stat.st_ino)!=inode_identity or (not reparse and (named.st_dev,named.st_ino)==inode_identity): raise RuntimeError("held executable binding changed")
            term=Terminal(launch_fd,executable,profile,env); term.wait(('connection profile "missing" credential is unavailable',),30); exit_value=term.close(2)
            results.append({"attack":name,"heldCandidateLaunch":True,"terminalExit":exit_value})
            os.replace(original,executable); os.link(executable,original)
        modifier=os.open(executable,os.O_RDWR)
        try:
            original_byte=os.pread(modifier,1,0)
            if len(original_byte)!=1: raise RuntimeError("source candidate mutation probe unavailable")
            os.pwrite(modifier,bytes([original_byte[0]^0x01]),0); os.fsync(modifier)
            if sha256_fd(held)==digest_identity["binarySHA256"]: raise RuntimeError("source candidate mutation probe did not alter held inode")
            term=Terminal(launch_fd,executable,profile,env); term.wait(('connection profile "missing" credential is unavailable',),30); exit_value=term.close(2)
            results.append({"attack":"inplace-content-mutation","heldCandidateLaunch":True,"terminalExit":exit_value,"sourceDigestChanged":True})
        finally:
            if 'original_byte' in locals(): os.pwrite(modifier,original_byte,0); os.fsync(modifier)
            os.close(modifier)
        if sha256_fd(held)!=digest_identity["binarySHA256"]: raise RuntimeError("source candidate mutation restoration failed")
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
def recovery_self_test(root):
    """Legacy owner-only process probe retained for source-level reaped-child coverage."""
    os.makedirs(root,exist_ok=True)
    master,slave=pty.openpty(); pid=os.fork()
    if pid==0:
        os.close(master); time.sleep(.2); os.close(slave); os._exit(0)
    os.close(slave); live=BoundProcess(pid)
    try: live.wait(.01); raise RuntimeError("controlled child exited before bounded observation")
    except subprocess.TimeoutExpired: pass
    if live.poll() is not None: raise RuntimeError("controlled child was not live at unresolved receipt")
    write_json(os.path.join(root,"recovery-unresolved-receipt.json"),{"outcome":"failed","closedReason":"live_child_unresolved","recoveryRetained":True})
    observed=live.wait_until_observed(); os.close(master)
    reaped_pid=os.fork()
    if reaped_pid==0: os._exit(0)
    os.waitpid(reaped_pid,0); reaped=BoundProcess(reaped_pid)
    if reaped.poll() is not None or not reaped.reaped_unowned: raise RuntimeError("controlled reaped child was not classified")
    try: reaped.wait(.01)
    except ChildReapedUnowned: pass
    else: raise RuntimeError("controlled reaped child was treated as terminal")
    write_json(os.path.join(root,"recovery-process-proof.json"),{"helperPID":os.getpid(),"childPID":pid,"childLiveAtUnresolvedReceipt":True,"ownedExitObserved":terminal_exit_reason(observed),"reapedChildOutcome":"terminal_unknown","reapedChildRecovery":"child_reaped_unowned"})
def recovery_dependency_live(url,name):
    """Read a dependency-owned loopback endpoint without retaining its address or body."""
    with urllib.request.urlopen(url,timeout=2) as response:
        value=json.load(response)
    if value!={"owner":"tui20-external-recovery-parent","dependency":name}: raise RuntimeError("external recovery dependency ownership unavailable")
def recovery_immutable_execution():
    """The recovery owner, not the failed helper, holds this sealed launch object."""
    if sys_platform()!="linux" or not hasattr(os,"memfd_create"): raise RuntimeError("external recovery process proof requires Linux sealed execution")
    sealed=os.memfd_create("tui20-external-recovery-owner",os.MFD_ALLOW_SEALING)
    try:
        os.write(sealed,b"tui20 external recovery execution object\n")
        seals=fcntl.F_SEAL_WRITE|fcntl.F_SEAL_GROW|fcntl.F_SEAL_SHRINK|fcntl.F_SEAL_SEAL
        fcntl.fcntl(sealed,fcntl.F_ADD_SEALS,seals)
        if fcntl.fcntl(sealed,fcntl.F_GET_SEALS)&seals!=seals: raise RuntimeError("external recovery immutable execution unavailable")
        return sealed,seals
    except Exception:
        os.close(sealed); raise
def recovery_owner_self_test(root,core_url,jwks_url):
    """Prove a crashed helper cannot tear down the distinct PTY/resource owner."""
    os.makedirs(root,exist_ok=True)
    master,slave=pty.openpty(); sealed,seals=recovery_immutable_execution(); pid=os.fork()
    if pid==0:
        os.close(master); time.sleep(.25); os.close(slave); os._exit(0)
    os.close(slave); live=BoundProcess(pid)
    helper=subprocess.Popen([sys.executable,"-c","import os,signal; os.kill(os.getpid(),signal.SIGKILL)"])
    helper_status=helper.wait()
    if helper_status>=0: raise RuntimeError("controlled recovery helper did not crash")
    try: live.wait(.01); raise RuntimeError("controlled child exited before helper-failure observation")
    except subprocess.TimeoutExpired: pass
    if live.poll() is not None or live.reaped_unowned: raise RuntimeError("controlled child was not live after helper crash")
    try: os.fstat(master)
    except OSError as error: raise RuntimeError("recovery owner lost PTY after helper crash") from error
    if fcntl.fcntl(sealed,fcntl.F_GET_SEALS)&seals!=seals: raise RuntimeError("recovery owner lost immutable execution after helper crash")
    recovery_dependency_live(core_url,"core"); recovery_dependency_live(jwks_url,"jwks")
    write_json(os.path.join(root,"recovery-unresolved-receipt.json"),{"outcome":"failed","closedReason":"live_child_unresolved","recoveryRetained":True})
    observed=live.wait_until_observed(); os.close(master); os.close(sealed)
    reaped_pid=os.fork()
    if reaped_pid==0: os._exit(0)
    os.waitpid(reaped_pid,0); reaped=BoundProcess(reaped_pid)
    if reaped.poll() is not None or not reaped.reaped_unowned: raise RuntimeError("controlled reaped child was not classified")
    write_json(os.path.join(root,"recovery-owner-proof.json"),{"recoveryOwnerPID":os.getpid(),"failedHelperPID":helper.pid,"helperCrashObserved":True,"childPID":pid,"childLiveAfterHelperCrash":True,"ptyHeldAfterHelperCrash":True,"immutableExecutionHeld":True,"dependenciesLiveAfterHelperCrash":True,"ownedExitObserved":terminal_exit_reason(observed),"reapedChildOutcome":"terminal_unknown","reapedChildRecovery":"child_reaped_unowned"})
class OwnedRuntimeAcquisitionFailure(RuntimeError):
    """Preserve the actual owned runtime for owner-driven graceful cleanup."""
    def __init__(self,runtime,message): super().__init__(message); self.runtime=runtime
def process_birth(pid):
    """Bind a PID to its OS birth value so a later PID reuse cannot be attested."""
    if sys_platform()=="linux":
        with open("/proc/%d/stat"%pid,encoding="utf-8") as stream: fields=stream.read().rsplit(") ",1)[1].split()
        return {"platform":"linux","startTimeTicks":fields[19]}
    if sys_platform()=="darwin":
        value=subprocess.check_output(["/bin/ps","-o","lstart=","-p",str(pid)],text=True).strip()
        if not value: raise RuntimeError("Darwin process birth unavailable")
        return {"platform":"darwin","startTime":value}
    raise RuntimeError("POSIX process birth unavailable")
def write_owner_birth(args,runtime,identity):
    """The live resource owner records only identities it directly holds."""
    write_json(os.path.join(args.root,"owner-birth.json"),{"recoveryOwnerPID":os.getpid(),"recoveryOwnerBirth":process_birth(os.getpid()),"runtimePID":runtime.pid,"runtimeBirth":process_birth(runtime.pid),"nativeSHA256":identity["binarySHA256"],"ownerStarted":True})
def start_owned_runtime(args):
    """Start the actual Core/JWKS process under this durable resource owner."""
    command=[args.node,args.runtime_script,"--root",args.root,"--core-commit",args.core_commit]
    if args.invalid_ready_receipt: command.append("--invalid-ready-receipt")
    if args.runtime_ready_mode: command += ["--ready-mode",args.runtime_ready_mode]
    if args.shutdown_pipe_failure: command.append("--shutdown-pipe-failure")
    runtime=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    runtime.tui20_birth=process_birth(runtime.pid)
    # A runtime that never hands off its line must not trap the owner at a
    # blocking readline.  It remains an owned child for recovery below.
    ready_stream,_,_=select.select([runtime.stdout],[],[],10)
    if not ready_stream:
        raise OwnedRuntimeAcquisitionFailure(runtime,"owned Core runtime ready handoff timed out")
    line=runtime.stdout.readline()
    try: ready=json.loads(line)
    except Exception as error:
        raise OwnedRuntimeAcquisitionFailure(runtime,"owned Core runtime did not provide a bounded ready handoff") from error
    if ready.get("event")!="ready" or not all(isinstance(ready.get(key),str) and ready[key] for key in ("url","token","deniedToken")) or not isinstance(ready.get("jwksPort"),int):
        raise OwnedRuntimeAcquisitionFailure(runtime,"owned Core runtime handoff invalid")
    return runtime,ready
def stop_owned_runtime(runtime):
    """Retain the owner handle through a natural, directly observed close."""
    if runtime.poll() is not None: return terminal_exit_reason(runtime.returncode),False
    requested=True
    try:
        runtime.stdin.write("close\n"); runtime.stdin.flush()
    except Exception:
        # A failed control pipe is unresolved, not authority to kill or abandon
        # the actual Core/JWKS child.  Keep its parent and wait handle alive.
        requested=False
    runtime.wait()
    return terminal_exit_reason(runtime.returncode),requested
def controller_failed(pid):
    try: os.kill(pid,0)
    except ProcessLookupError: return True
    return False
def retain_primary_and_recover(root,outcome,terminal):
    """Persist an unresolved primary while its helper still owns recovery."""
    terminal.exit_reason=terminal.exit_reason or "terminal_unknown"
    if not any(item.get("terminal")==terminal.label and item.get("exit")==terminal.exit_reason for item in outcome["terminals"]): outcome["terminals"].append({"terminal":terminal.label,"exit":terminal.exit_reason})
    outcome["failureReason"]="native_harness_assertion_failed"
    persist_primary_outcome(root,outcome)
    outcome["recoveryObservation"]=terminal.recover_until_observed()
    return True
def main():
    OWNED_TERMINALS.clear()
    parser=argparse.ArgumentParser(); parser.add_argument("--root",required=True); parser.add_argument("--executable"); parser.add_argument("--source-commit"); parser.add_argument("--core-commit"); parser.add_argument("--runtime-script"); parser.add_argument("--node"); parser.add_argument("--external-runtime",action="store_true"); parser.add_argument("--adverse-controller-crash",action="store_true"); parser.add_argument("--adverse-owner-death",action="store_true"); parser.add_argument("--controller-pid",type=int); parser.add_argument("--recovery-self-test",action="store_true"); parser.add_argument("--recovery-owner-self-test",action="store_true"); parser.add_argument("--core-url"); parser.add_argument("--jwks-url"); parser.add_argument("--invalid-ready-receipt",action="store_true"); parser.add_argument("--inject-finalization-cleanup-failure",action="store_true"); parser.add_argument("--runtime-ready-mode",choices=("normal","timeout","eof")); parser.add_argument("--shutdown-pipe-failure",action="store_true"); args=parser.parse_args()
    if args.recovery_owner_self_test:
        if not args.core_url or not args.jwks_url: raise RuntimeError("external recovery dependency endpoints are required")
        recovery_owner_self_test(args.root,args.core_url,args.jwks_url); return
    if args.recovery_self_test: recovery_self_test(args.root); return
    if not args.executable or not args.source_commit or not args.core_commit or (not args.external_runtime and (not args.runtime_script or not args.node)): raise RuntimeError("native executable, source identity, and owned runtime are required")
    if args.adverse_controller_crash and not args.controller_pid: raise RuntimeError("adverse controller proof requires controller identity")
    paths={"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True}
    runtime=None; held=None; launch_fd=None; darwin_protected=None; term=None; primary_persisted=False; finalization_failure=None
    with open(args.executable,"rb") as source_binary: binary_sha256=hashlib.file_digest(source_binary,"sha256").hexdigest()
    outcome={"outcome":"failed","coreCommit":args.core_commit,"runtimePathReceipt":paths,"terminals":[]}
    try:
        # Every post-start preflight stays inside this owner boundary. A bad
        # receipt or candidate can therefore never strand the actual Core/JWKS
        # child that this process owns.
        if args.external_runtime:
            try: runtime_handoff=json.loads(sys.stdin.readline())
            except Exception as error: raise RuntimeError("external runtime handoff unavailable") from error
            if not all(isinstance(runtime_handoff.get(key),str) and runtime_handoff[key] for key in ("url","token","deniedToken")) or not isinstance(runtime_handoff.get("jwksPort"),int) or not isinstance(runtime_handoff.get("_runtimePID"),int) or not isinstance(runtime_handoff.get("_runtimeBirth"),dict): raise RuntimeError("external runtime handoff invalid")
        else:
            runtime,runtime_handoff=start_owned_runtime(args)
        if args.shutdown_pipe_failure: raise RuntimeError("controlled post-handoff runtime pipe failure")
        with open(os.path.join(args.root,"ready.json"),encoding="utf-8") as stream: ready=json.load(stream)
        if ready.get("coreCommit")!=args.core_commit or ready.get("runtimePathReceipt")!=paths: raise RuntimeError("Core fixture receipt invalid")
        token,denied,url=runtime_handoff["token"],runtime_handoff["deniedToken"],runtime_handoff["url"]
        env={key:os.environ[key] for key in ALLOWED_ENV if os.environ.get(key)}; env.update({"TERM":"xterm-256color","SERVICE_LASSO_API_TOKEN":token,"SERVICE_LASSO_DENIED_TOKEN":denied,"SERVICE_LASSO_INVALID_TOKEN":"tui20-invalid-token","SERVICE_LASSO_CONNECTIONS_CONFIG":os.path.join(args.root,"connections.json")})
        held,identity,held_inode=hold_candidate(args.executable,args.source_commit); outcome["candidateIdentity"]=identity
        if runtime is not None: write_owner_birth(args,runtime,identity)
        launch_fd,darwin_protected,binding=bound_execution(held,identity,args.root); outcome["heldExecutableBinding"]=binding
        if args.adverse_controller_crash:
            term=Terminal(launch_fd,args.executable,"native",env); term.wait(("Runtime identity:",),30); detail(term)
            runtime_pid=runtime.pid if runtime is not None else runtime_handoff["_runtimePID"]; runtime_birth=runtime.tui20_birth if runtime is not None else runtime_handoff["_runtimeBirth"]
            write_json(os.path.join(args.root,"external-owner-live.json"),{"externalOwnerPID":os.getpid(),"ownerBirth":process_birth(os.getpid()),"coreRuntimePID":runtime_pid,"runtimeBirth":runtime_birth,"tuiChildPID":term.child.pid,"tuiChildBirth":term.child.tui20_birth,"sourceCommit":identity["sourceCommit"],"binarySHA256":identity["binarySHA256"],"phase":"controller_failure_live","immutableExecutionHeld":True,"ptyHeld":True})
            deadline=time.monotonic()+15
            while not controller_failed(args.controller_pid):
                if time.monotonic()>=deadline: raise RuntimeError("actual controller did not fail")
                time.sleep(.02)
            if term.child.poll() is not None or term.child.reaped_unowned: raise RuntimeError("TUI child did not survive actual controller failure")
            request(url,token,"/api/health"); urllib.request.urlopen("http://127.0.0.1:%d/jwks"%runtime_handoff["jwksPort"],timeout=2).read()
            exit_reason=term.close()
            outcome["terminals"].append({"terminal":"controller-failure-recovery","exit":exit_reason})
            outcome.update({"outcome":"succeeded","adverseControllerFailure":{"controllerFailed":True,"childLiveAfterFailure":True,"ptyHeldAfterFailure":True,"immutableExecutionHeldAfterFailure":True,"coreAndJwksLiveAfterFailure":True,"naturalChildExit":exit_reason}})
            write_json(os.path.join(args.root,"recovery-owner-proof.json"),outcome["adverseControllerFailure"])
            write_json(os.path.join(args.root,"external-owner-live.json"),{"externalOwnerPID":os.getpid(),"ownerBirth":process_birth(os.getpid()),"coreRuntimePID":runtime_pid,"runtimeBirth":runtime_birth,"tuiChildPID":term.child.pid,"tuiChildBirth":term.child.tui20_birth,"sourceCommit":identity["sourceCommit"],"binarySHA256":identity["binarySHA256"],"phase":"normal_q_exit","immutableExecutionHeld":True,"ptyHeld":False})
            return
        if args.adverse_owner_death:
            term=Terminal(launch_fd,args.executable,"native",env); term.wait(("Runtime identity:",),30); detail(term)
            runtime_pid=runtime.pid if runtime is not None else runtime_handoff["_runtimePID"]; runtime_birth=runtime.tui20_birth if runtime is not None else runtime_handoff["_runtimeBirth"]
            write_json(os.path.join(args.root,"external-owner-live.json"),{"externalOwnerPID":os.getpid(),"ownerBirth":process_birth(os.getpid()),"coreRuntimePID":runtime_pid,"runtimeBirth":runtime_birth,"tuiChildPID":term.child.pid,"tuiChildBirth":term.child.tui20_birth,"sourceCommit":identity["sourceCommit"],"binarySHA256":identity["binarySHA256"],"phase":"owner_death_live","immutableExecutionHeld":True,"ptyHeld":True})
            # This is an adverse owner death while the production TUI is live.
            # The observer owns Core/JWKS and records the later external reaping;
            # no child is force-closed and no terminal status is fabricated.
            os.kill(os.getpid(),signal.SIGKILL)
        before=unrelated(url,token); adverse=[]
        for profile,label,expected,audit_expected in (("missing","missing-credential",2,0),("invalid","invalid-credential",0,0),("denied","scope-denied",0,5)):
            count=len(operations(url,token)); audit=audit_count(url,token)
            if profile=="missing":
                outcome["heldExecutableBinding"]["replacementProbe"]=bound_replacement_probe(launch_fd,held,args.executable,held_inode,identity,profile,env)
                term=Terminal(launch_fd,args.executable,profile,env); term.wait(('connection profile "missing" credential is unavailable',),30)
            else:
                term=Terminal(launch_fd,args.executable,profile,env); term.wait(("Runtime identity:",),30); detail(term); term.write("i"); term.wait(("Runtime API unavailable:",),30)
            outcome["terminals"].append({"terminal":label,"exit":term.close(expected)})
            after=len(operations(url,token)); audit_after=audit_count(url,token); adverse.append({"case":label,"beforeOperationCount":count,"afterOperationCount":after,"noOperation":after==count,"coreDeniedAuditBefore":audit,"coreDeniedAuditAfter":audit_after,"coreDeniedAuditDelta":audit_after-audit})
            if after!=count or audit_after-audit!=audit_expected: raise RuntimeError("adverse lifecycle receipt invalid")
        term=Terminal(launch_fd,args.executable,"native",env); term.wait(("Runtime identity:",),30); detail(term)
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
        runtime_pid=runtime.pid if runtime is not None else runtime_handoff["_runtimePID"]; runtime_birth=runtime.tui20_birth if runtime is not None else runtime_handoff["_runtimeBirth"]
        write_json(os.path.join(args.root,"external-owner-live.json"),{"externalOwnerPID":os.getpid(),"ownerBirth":process_birth(os.getpid()),"coreRuntimePID":runtime_pid,"runtimeBirth":runtime_birth,"tuiChildPID":term.child.pid,"tuiChildBirth":term.child.tui20_birth,"sourceCommit":identity["sourceCommit"],"binarySHA256":identity["binarySHA256"],"phase":"normal_q_exit","immutableExecutionHeld":True,"ptyHeld":False})
        term=Terminal(launch_fd,args.executable,"native",env); term.wait(("Runtime identity:",),30); detail(term); outcome["terminals"].append({"terminal":"reconnect","exit":term.close()})
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
    except OwnedRuntimeAcquisitionFailure as failure:
        runtime=failure.runtime
        outcome["failureReason"]="native_harness_assertion_failed"
        raise
    except TerminalUnresolved as unresolved:
        primary_persisted=retain_primary_and_recover(args.root,outcome,unresolved.terminal)
        raise
    except DarwinActivationFailure as failure:
        launch_fd=darwin_protected=failure.recovery
        outcome["heldExecutableBinding"]={"platform":"darwin","mechanism":"system-immutable-held-directory-execve","systemImmutableLeaf":True,"systemImmutableParent":False,"partialActivationRecovery":True}
        outcome["failureReason"]="native_harness_assertion_failed"; raise
    except Exception:
        observed=OWNED_TERMINALS[-1] if OWNED_TERMINALS else term
        if observed is not None and observed.exit_reason is not None and not any(item.get("terminal")==observed.label and item.get("exit")==observed.exit_reason for item in outcome["terminals"]): outcome["terminals"].append({"terminal":observed.label,"exit":observed.exit_reason})
        if observed is not None and observed.child.returncode is None: primary_persisted=retain_primary_and_recover(args.root,outcome,observed)
        else: outcome["failureReason"]="native_harness_assertion_failed"
        raise
    finally:
        # Cleanup is reported separately and cannot relabel a true child exit.
        live_child=any(owned.child.poll() is None for owned in OWNED_TERMINALS)
        try:
            cleanup=lambda *values: (_ for _ in ()).throw(RuntimeError("injected native finalization cleanup failure")) if args.inject_finalization_cleanup_failure else cleanup_execution(*values)
            finalize_native_outcome(args.root,outcome,launch_fd,darwin_protected,held,live_child,primary_persisted,outcome.get("recoveryObservation"),cleanup=cleanup)
        except Exception as error:
            finalization_failure="injected_cleanup_failure" if args.inject_finalization_cleanup_failure else "finalization_failure"
            outcome["finalizationFailure"]=finalization_failure
        finally:
            if args.external_runtime:
                # This is the owner's outcome only. Core belongs to the durable
                # observer; no owner declaration can attest its shutdown.
                handoff=locals().get("runtime_handoff",{})
                write_json(os.path.join(args.root,"external-owner-finalization.json"),{"externalOwnerPID":os.getpid(),"ownerBirth":process_birth(os.getpid()),"runtimePID":handoff.get("_runtimePID"),"runtimeBirth":handoff.get("_runtimeBirth"),"sourceCommit":args.source_commit,"binarySHA256":binary_sha256,"finalizationFailure":finalization_failure,"liveTuiChildRetained":live_child,"primaryOutcome":outcome["outcome"]})
            # This is deliberately nested: a persistence or cleanup failure must
            # never strand the real owner child once no TUI child remains live.
            if runtime is not None and not live_child:
                runtime_exit,shutdown_requested=stop_owned_runtime(runtime)
                write_json(os.path.join(args.root,"owner-runtime-recovery.json"),{"outcome":"observed_closed","runtimePID":runtime.pid,"runtimeBirth":runtime.tui20_birth,"runtimeChildExit":runtime_exit,"shutdownRequested":shutdown_requested,"actualExitObserved":True})
                if args.invalid_ready_receipt:
                    write_json(os.path.join(args.root,"owner-preflight-cleanup.json"),{"outcome":"failed","preflight":"runtime_receipt_invalid","ownerDrivenRuntimeShutdown":shutdown_requested,"runtimeChildExit":runtime_exit,"retainedOwnedRuntime":False,"actualExitObserved":True})
                if finalization_failure is not None:
                    write_json(os.path.join(args.root,"owner-finalization-failure-proof.json"),{"outcome":"failed","finalizationFailure":finalization_failure,"ownerDrivenRuntimeShutdown":shutdown_requested,"runtimeChildExit":runtime_exit,"retainedOwnedRuntime":False,"runtimeChildStillLiveAfterClose":runtime.poll() is None,"actualExitObserved":True})
if __name__=="__main__": main()
