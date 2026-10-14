"""Windows ConPTY lifecycle evidence with memory-only bearer credentials."""
import argparse, ctypes, hashlib, json, os, select, stat, time, urllib.request, sys, queue, threading
from tui28_json import strict_load, strict_loads
from tui28_windows_identity import process_birth
from ctypes import wintypes
from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess

ALLOWED_ENV=("APPDATA","COMSPEC","LOCALAPPDATA","PATHEXT","PATH","SYSTEMROOT","TEMP","TMP","USERPROFILE","WINDIR")
SAFE_CONFIGURATION_DIAGNOSTIC="connection configuration unavailable; check profile settings"
def assert_safe_missing_credential(text,env):
    """Inspect captured bytes in memory; never include private values in failures."""
    if SAFE_CONFIGURATION_DIAGNOSTIC not in text: raise RuntimeError("safe configuration diagnostic unavailable")
    forbidden=['"missing"','SERVICE_LASSO_MISSING_TOKEN']
    for key in ("SERVICE_LASSO_API_TOKEN","SERVICE_LASSO_DENIED_TOKEN","SERVICE_LASSO_INVALID_TOKEN","SERVICE_LASSO_CONNECTIONS_CONFIG"):
        forbidden.append(key)
        if env.get(key): forbidden.append(env[key])
    if any(value in text for value in forbidden): raise RuntimeError("configuration diagnostic privacy denied")
def write_json(path,value):
    with open(path,"w",encoding="utf-8",newline="") as f: json.dump(value,f,separators=(",",":")); f.flush(); os.fsync(f.fileno())
def hold_candidate(executable,source,expected):
    if len(source)!=40: raise RuntimeError("candidate source identity invalid")
    executable=os.path.abspath(executable); meta=os.lstat(executable)
    if not stat.S_ISREG(meta.st_mode) or os.name!="nt": raise RuntimeError("candidate executable invalid")
    k=ctypes.WinDLL("kernel32",use_last_error=True); k.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,wintypes.LPVOID,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]; k.CreateFileW.restype=wintypes.HANDLE
    handle=k.CreateFileW(executable,0x80000000,0x00000001,None,3,0x80,None)
    if handle==wintypes.HANDLE(-1).value: raise OSError(ctypes.get_last_error(),"candidate handle acquisition failed")
    # Hold the containing directory without delete sharing as well: the named
    # execution path cannot be moved to a replacement parent while bytes stay
    # locked. Child reads/writes remain allowed in the already-owned directory.
    k.CloseHandle.argtypes=[wintypes.HANDLE];k.CloseHandle.restype=wintypes.BOOL
    directory=k.CreateFileW(os.path.dirname(executable),0x80,0x00000003,None,3,0x02000000,None)
    if directory==wintypes.HANDLE(-1).value:
        k.CloseHandle(handle);raise OSError(ctypes.get_last_error(),"candidate parent handle acquisition failed")
    import msvcrt
    f=os.fdopen(msvcrt.open_osfhandle(handle,os.O_RDONLY),"rb"); digest=hashlib.sha256()
    try:
        while True:
            chunk=f.read(1024*1024)
            if not chunk: break
            digest.update(chunk)
        if digest.hexdigest()!=expected: raise RuntimeError("held candidate differs from observer bytes")
        return executable,f,{"sourceCommit":source,"binarySHA256":digest.hexdigest()},directory
    except Exception: f.close();k.CloseHandle(directory);raise
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
def drain(process,transcript):
    try: chunk=process.tui28_queue.get(timeout=.1)
    except queue.Empty: return ""
    if chunk is None:
        if getattr(process,"tui28_reader_failed",False): raise RuntimeError("native terminal reader failed")
        return None
    # pywinpty supplies decoded text, not original ConPTY bytes.
    transcript[0]+=chunk
    if len(transcript[0])>4*1024*1024: raise RuntimeError("terminal quota")
    return chunk
def wait_for(process,expected,timeout,transcript,start=0):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        if all(x in transcript[0][start:] for x in expected): return
        readable=True
        if readable and drain(process,transcript) is None:
            reason=terminal_exit(process)
            if reason is not None: raise RuntimeError("terminal exited before expected state: "+reason)
            time.sleep(.05)
    raise RuntimeError("missing terminal state")
def close_terminal(process,transcript,expected="terminal_exited_zero"):
    if process.isalive(): process.write("q")
    end=time.monotonic()+10
    while time.monotonic()<end:
        readable=True
        if readable: drain(process,transcript)
        reason=terminal_exit(process)
        if reason is not None:
            observed=process.wait()
            if observed!=getattr(process,"exitstatus",None): raise RuntimeError("actual terminal wait mismatch")
            if reason!=expected: raise RuntimeError("unexpected terminal exit: "+reason)
            return reason
    raise RuntimeError("terminal exit was not observed")
def request_json(url,token,path):
    req=urllib.request.Request(url+path,headers={"Authorization":"Bearer "+token})
    with urllib.request.urlopen(req,timeout=15) as response: return strict_load(response)
def records(url,token): return request_json(url,token,"/api/operator/lifecycle/operations")["operations"]
def stable_hash(value): return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(",",":")).encode()).hexdigest()
def unrelated_snapshot(url,token):
    result=request_json(url,token,"/api/services"); entries=result.get("services",result) if isinstance(result,dict) else result
    service=next((x for x in entries if isinstance(x,dict) and x.get("id")=="tui20-unrelated"),None)
    availability=request_json(url,token,"/api/operator/lifecycle/services/tui20-unrelated/availability")
    if service is None: raise RuntimeError("unrelated service missing")
    runtime=availability.get("runtime",{}) if isinstance(availability,dict) else {}
    # Keep safe lifecycle fields, never an opaque full response hash.
    return {"servicePresent":True,"id":service.get("id"),"enabled":service.get("enabled"),"runtimeState":runtime.get("state"),"running":runtime.get("running"),"availability":availability.get("available") if isinstance(availability,dict) else None}
def audit_events(url,token):
    body=request_json(url,token,"/api/audit")
    return body.get("events",body.get("audit",[])) if isinstance(body,dict) else body
def denial_audit_count(url,token):
    events=audit_events(url,token)
    return sum(1 for event in events if isinstance(event,dict) and event.get("action")=="mcp.action.denied" and event.get("actor")=="tui20-native-operator")
def detail(p,t):
    p.write("/"); p.write("tui20-fixture"); p.write("\r"); wait_for(p,("tui20-fixture",),20,t); p.write("\r"); wait_for(p,("Lifecycle:","tui20-fixture"),20,t)
def action(p,t,key,name,url,token):
    known={record.get("operationId") for record in records(url,token)}; observed_running=None
    start=len(t[0]); p.write(key); wait_for(p,("Core preview: "+name,"Confirm "+name),45,t,start); frozen=t[0]; p.write("rj?q"); time.sleep(.3)
    if "Confirm "+name not in t[0] or "Core preview: "+name not in t[0]: raise RuntimeError("confirmation changed by blocked input")
    p.write("y"); p.write("r")
    # A restart can temporarily take the API connection down after Core has
    # accepted the operation. Reconnect reads the retained operation; it never
    # sends the lifecycle key again.
    deadline=time.monotonic()+90
    while time.monotonic()<deadline:
        if "Core operation" in t[0][start:] and "succeeded." in t[0][start:]: break
        if observed_running is None:
            try: current=records(url,token)
            except Exception: current=[]
            for record in current:
                identifier=record.get("operationId")
                if identifier not in known and record.get("status") in ("accepted","running") and record.get("cancellationSupported") is False and ("Core operation "+identifier+": ") in t[0][start:]:
                    if "z request Core cancellation" in t[0][start:]: raise RuntimeError("unsupported running action advertised cancellation")
                    p.write("z"); observed_running={"operationId":identifier,"observedStatus":record["status"],"renderedOperation":True,"cancellationAdvertised":False,"key":"z"}
        if "Runtime API unavailable:" in t[0][start:]:
            p.write("r")
        readable=True
        if readable and drain(p,t) is None:
            raise RuntimeError("terminal closed during retained operation readback")
    else: raise RuntimeError("retained operation readback unavailable")
    if t[0].count("Core operation")<frozen.count("Core operation")+1: raise RuntimeError("operation result was not rendered")
    return observed_running
def _main_attempt(observation):
    parser=argparse.ArgumentParser(); parser.add_argument("--root",required=True); parser.add_argument("--executable",required=True); parser.add_argument("--source-commit",required=True); parser.add_argument("--core-commit",required=True); parser.add_argument("--external-runtime",action="store_true"); args=parser.parse_args()
    if not args.external_runtime: raise RuntimeError("external observer required")
    ready=strict_loads(sys.stdin.readline(1048577))
    paths={"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True}
    with open(os.path.join(args.root,"ready.json"),encoding="utf-8") as stream: runtime_receipt=strict_load(stream)
    if runtime_receipt.get("coreCommit")!=args.core_commit or runtime_receipt.get("runtimePathReceipt")!=paths: raise RuntimeError("Core fixture receipt invalid")
    token=ready.get("token"); denied=ready.get("deniedToken"); url=ready.get("url")
    if not token or not denied or not url: raise RuntimeError("owned in-memory credential handoff unavailable")
    with open(os.path.join(args.root,"connections.json"),encoding="utf-8") as f: connections=strict_load(f)
    env={key:os.environ[key] for key in ALLOWED_ENV if os.environ.get(key)}; env.update({"TERM":"xterm-256color","SERVICE_LASSO_API_TOKEN":token,"SERVICE_LASSO_DENIED_TOKEN":denied,"SERVICE_LASSO_INVALID_TOKEN":"tui20-invalid-token","SERVICE_LASSO_CONNECTIONS_CONFIG":os.path.join(args.root,"connections.json")})
    executable,held,identity,held_directory=hold_candidate(args.executable,args.source_commit,ready["_binarySHA256"]); observation["candidate"]={"file":held,"directory":held_directory}; outcome={"outcome":"failed","candidateIdentity":identity,"coreCommit":args.core_commit,"runtimePathReceipt":paths,"terminals":[]}; open_processes=[]
    def launch(profile,label):
        # Retained v1 screen/route assertions use the explicit compatibility
        # client; current installed v2 acceptance requires a separate caller.
        p=PtyProcess.spawn([executable,"--compat-v1","--profile",profile],cwd=os.path.dirname(executable),env=env,dimensions=(44,150),backend=Backend.ConPTY)
        open_processes.append(p) # Ownership precedes birth/reader/persistence.
        observation["processes"].append(p)
        p.tui28_label=label; p.tui28_queue=queue.Queue(maxsize=64); p.tui28_birth=None
        p.tui28_birth=process_birth(p.pid)
        if p.tui28_birth["parentPID"]!=os.getpid() or os.path.normcase(os.path.abspath(p.tui28_birth["image"]))!=os.path.normcase(executable): raise RuntimeError("actual terminal image/parent custody denied")
        def consume():
            while True:
                try: chunk=p.read(4096)
                except EOFError:
                    if p.isalive(): time.sleep(.05);continue
                    p.tui28_queue.put(None);return
                except Exception as original:
                    observation["originals"].append({"phase":"terminal_reader","process":p,"original":original})
                    p.tui28_reader_failed=True;p.tui28_queue.put(None);return
                if not chunk:
                    if p.isalive(): time.sleep(.05);continue
                    p.tui28_queue.put(None);return
                p.tui28_queue.put(chunk)
        p.tui28_reader=threading.Thread(target=consume); p.tui28_reader.start()
        write_json(os.path.join(args.root,"external-owner-live.json"),{"externalOwnerPID":os.getpid(),"coreRuntimePID":ready["_runtimePID"],"sourceCommit":args.source_commit,"binarySHA256":identity["binarySHA256"],"ownerBirth":process_birth(os.getpid()),"runtimeBirth":ready["_runtimeBirth"],"tuiChildPID":p.pid,"tuiChildBirth":p.tui28_birth,"phase":"live_terminal"})
        return p,[""]
    def finish(p,t,label,expected="terminal_exited_zero"):
        outcome["terminals"].append({"terminal":label,"exit":close_terminal(p,t,expected)})
        complete_reader(p,t,observation)
        write_json(os.path.join(args.root,"external-owner-live.json"),{"externalOwnerPID":os.getpid(),"coreRuntimePID":ready["_runtimePID"],"sourceCommit":args.source_commit,"binarySHA256":identity["binarySHA256"],"ownerBirth":process_birth(os.getpid()),"runtimeBirth":ready["_runtimeBirth"],"tuiChildPID":p.pid,"tuiChildBirth":p.tui28_birth,"phase":"normal_q_exit"})
        open_processes.remove(p)
    try:
        # Real Windows sharing denials protect the held execution bytes. The
        # confined replacement fixture is retained on failure, never swept.
        wrote=False
        try:
            with open(executable,"r+b") as writer: writer.write(b"X"); writer.flush(); os.fsync(writer.fileno()); wrote=True
        except PermissionError: pass
        replacement=os.path.join(args.root,"owned-candidate-replacement.exe")
        with open(replacement,"xb") as stream: stream.write(b"owned denied replacement\n");stream.flush();os.fsync(stream.fileno())
        replaced=False
        try: os.replace(replacement,executable);replaced=True
        except PermissionError: pass
        if wrote or replaced: raise RuntimeError("held Windows candidate mutation was not denied")
        write_json(os.path.join(args.root,"owner-private-windows-mutation.json"),{"classification":"owner-private","sourceCommit":args.source_commit,"binarySHA256":identity["binarySHA256"],"inPlaceWriteDenied":True,"replacementDenied":True})
        before_unrelated=unrelated_snapshot(url,token); adverse=[]
        p,t=launch("missing","missing-credential"); before=len(records(url,token)); audit_before=denial_audit_count(url,token); wait_for(p,(SAFE_CONFIGURATION_DIAGNOSTIC,),30,t); finish(p,t,"missing-credential","terminal_exit_code_2"); assert_safe_missing_credential(t[0],env); after=len(records(url,token)); audit_after=denial_audit_count(url,token); adverse.append({"case":"missing-credential","beforeOperationCount":before,"afterOperationCount":after,"noOperation":after==before,"coreDeniedAuditBefore":audit_before,"coreDeniedAuditAfter":audit_after,"coreDeniedAuditDelta":audit_after-audit_before})
        for profile,label in (("invalid","invalid-credential"),("denied","scope-denied")):
            p,t=launch(profile,label); before=len(records(url,token)); audit_before=denial_audit_count(url,token); wait_for(p,("Runtime identity:",),30,t); detail(p,t); p.write("i"); wait_for(p,("Runtime API unavailable:",),30,t); finish(p,t,label); after=len(records(url,token)); audit_after=denial_audit_count(url,token); adverse.append({"case":label,"beforeOperationCount":before,"afterOperationCount":after,"noOperation":after==before,"coreDeniedAuditBefore":audit_before,"coreDeniedAuditAfter":audit_after,"coreDeniedAuditDelta":audit_after-audit_before})
        expected_denials={"missing-credential":0,"invalid-credential":0,"scope-denied":5}
        if not all(x["noOperation"] and x["coreDeniedAuditDelta"]==expected_denials[x["case"]] for x in adverse): raise RuntimeError("adverse lifecycle receipt invalid")
        p,t=launch("native","allowed"); wait_for(p,("Runtime identity:",),30,t); detail(p,t)
        before_escape=records(url,token); start=len(t[0]); p.write("i"); wait_for(p,("Core preview: install","Confirm install"),45,t,start); p.write("\x1b"); wait_for(p,("Confirmation cancelled.",),20,t,start)
        if records(url,token)!=before_escape: raise RuntimeError("Escape submitted an operation")
        start=len(t[0]); p.write("?"); wait_for(p,("n narrow","r reconnect"),20,t,start); start=len(t[0]); p.setwinsize(24,50); wait_for(p,("Service Lasso TUI","n narrow"),20,t,start); p.setwinsize(44,150); p.write("v"); detail(p,t)
        known_ids={record.get("operationId") for record in records(url,token)}; per_action=[]; running_proofs=[]
        expected_by_ui={"install":"service_install","config":"service_configure","start":"service_start","stop":"service_stop","restart":"service_restart"}
        for key,name in (("i","install"),("c","config"),("s","start"),("x","stop"),("R","restart")):
            running=action(p,t,key,name,url,token)
            if running is not None: running_proofs.append(running)
            added=[record for record in records(url,token) if record.get("operationId") not in known_ids]
            if len(added)!=1 or added[0].get("action")!=expected_by_ui[name] or added[0].get("targetIds")!=["tui20-fixture"]: raise RuntimeError("per-action durable readback invalid")
            known_ids.add(added[0]["operationId"]); per_action.append({key:added[0].get(key) for key in ("operationId","action","targetIds","status","outcome","cancellationSupported")})
        if not running_proofs: raise RuntimeError("actual running cancellation boundary was not observed")
        write_json(os.path.join(args.root,"keyboard-private-running-cancellation.json"),{"classification":"owner-private","observations":running_proofs})
        before_reload=records(url,token); p.write("l"); wait_for(p,("reload is unavailable",),15,t)
        if len(records(url,token))!=len(before_reload) or "z cancel" in t[0]: raise RuntimeError("reload or cancellation contract changed")
        p.write("r"); wait_for(p,("Lifecycle:",),20,t); finish(p,t,"allowed")
        p,t=launch("native","reconnect"); wait_for(p,("Runtime identity:",),30,t); detail(p,t); finish(p,t,"reconnect")
        all_records=records(url,token); audit=[{key:r.get(key) for key in ("operationId","action","targetIds","status","outcome","cancellationSupported")} for r in all_records if r.get("targetIds")==["tui20-fixture"]]
        audit_matches=[]
        for record in audit:
            matches=[event for event in audit_events(url,token) if isinstance(event,dict) and event.get("action")=="mcp.operation.succeeded" and event.get("actor")=="tui20-native-operator" and event.get("subject")==record["operationId"] and isinstance(event.get("metadata"),dict) and event["metadata"].get("operationId")==record["operationId"] and event["metadata"].get("action")==record["action"] and event["metadata"].get("targetIds")==record["targetIds"]]
            audit_matches.append({"operationId":record["operationId"],"coreAuditCount":len(matches)})
        if sorted(audit,key=lambda row:row["operationId"])!=sorted(per_action,key=lambda row:row["operationId"]) or any(row["coreAuditCount"]!=1 for row in audit_matches): raise RuntimeError("Core action audit invalid")
        expected=["service_restart","service_stop","service_start","service_configure","service_install"]
        if len(audit)!=5 or sorted(x["action"] for x in audit)!=sorted(expected) or any(not x["operationId"] or x["cancellationSupported"] or x["outcome"]!="succeeded" for x in audit) or len(all_records)!=len(before_reload): raise RuntimeError("durable action audit invalid")
        after_unrelated=unrelated_snapshot(url,token)
        if after_unrelated!=before_unrelated: raise RuntimeError("unrelated runtime or lifecycle state changed")
        write_json(os.path.join(args.root,"operation-audit.json"),{"operations":audit,"coreAudit":audit_matches}); outcome.update({"outcome":"succeeded","actions":["install","config","start","stop","restart"],"adverseAudit":adverse,"reloadDenied":True,"cancellation":{"advertised":False,"supportedActionTested":False},"reconnect":{"completedOperationNoReplay":True,"pendingReconciliation":"blocked_core_1553_no_adapter"},"unrelatedService":{**before_unrelated,"unchangedAfterFiveActions":True},"fixtureOperationCount":len(audit),"keyboard":{"previewEscapeNoSubmission":True,"helpRendered":True,"nativeResizeRendered":True,"unsupportedRunningCancellationDenied":True}})
    except Exception as error:
        observation["originals"].append({"phase":"lifecycle","original":error})
        observation["primaryPresent"]=True; observation["primary"]=error
        # Error text can carry a URL or server response; retain a closed reason only.
        outcome["failureReason"]="native_harness_assertion_failed" if isinstance(error,RuntimeError) else "native_harness_unexpected_failure"
        raise
    except BaseException as primary:
        observation["originals"].append({"phase":"lifecycle_control","original":primary})
        observation["primaryPresent"]=True; observation["primary"]=primary
        raise
    finally:
        # Persist the failed primary before recovering any still-live terminal.
        primary_sink_failed=False
        try: write_json(os.path.join(args.root,"native-exit-receipt.json"),outcome)
        except OSError as original:
            observation["originals"].append({"phase":"primary_receipt","original":original})
            primary_sink_failed=True
        for p in open_processes:
            try:
                if p.isalive(): p.write("q")
                # No second deadline, terminate or force-close. Retain Core and
                # locked executable under the durable parent until actual exit.
                while p.isalive(): drain(p,[""]); time.sleep(.05)
                observed=p.wait()
                if observed!=getattr(p,"exitstatus",None): raise RuntimeError("unowned terminal wait")
                outcome["terminals"].append({"terminal":p.tui28_label,"exit":terminal_exit(p) or "terminal_unknown"})
            except Exception as original:
                observation["originals"].append({"phase":"terminal_recovery","process":p,"original":original})
                # Unknown observation is never accepted. The exception keeps
                # the owner alive rather than releasing a still-live child.
                if p.isalive():
                    while p.isalive(): time.sleep(.1)
                    p.wait()
                outcome["terminals"].append({"terminal":p.tui28_label,"exit":"terminal_unknown"})
            # Reader ownership survives both process exit and recovery failure.
            # An unknown join propagates before either candidate handle closes.
            complete_reader(p,[""],observation)
        cleanup_failure="finalization_failure" if primary_sink_failed else None
        cleanup={"outcome":"succeeded","closedReason":"held_candidate_handle_closed","recoveryRetained":False}
        try:
            held.close()
            kernel=ctypes.WinDLL("kernel32",use_last_error=True);kernel.CloseHandle.argtypes=[wintypes.HANDLE];kernel.CloseHandle.restype=wintypes.BOOL
            if not kernel.CloseHandle(held_directory): raise OSError("candidate parent handle close failed")
        except Exception as original:
            observation["originals"].append({"phase":"candidate_cleanup","original":original})
            cleanup_failure="cleanup_failed";cleanup={"outcome":"failed","closedReason":"descriptor_close_failed","recoveryRetained":False}
        try: write_json(os.path.join(args.root,"native-cleanup-receipt.json"),cleanup)
        except OSError as original:
            observation["originals"].append({"phase":"cleanup_receipt","original":original})
            cleanup_failure="cleanup_receipt_persistence_failed"
        try: write_json(os.path.join(args.root,"native-exit-receipt.json"),outcome)
        except OSError as original:
            observation["originals"].append({"phase":"final_receipt","original":original})
            cleanup_failure=cleanup_failure or "finalization_failure"
        write_json(os.path.join(args.root,"external-owner-finalization.json"),{"externalOwnerPID":os.getpid(),"ownerBirth":process_birth(os.getpid()),"runtimePID":ready["_runtimePID"],"runtimeBirth":ready["_runtimeBirth"],"sourceCommit":args.source_commit,"binarySHA256":identity["binarySHA256"],"finalizationFailure":cleanup_failure,"liveTuiChildRetained":False,"primaryOutcome":outcome["outcome"],**({"cleanupOutcome":cleanup} if cleanup_failure=="cleanup_receipt_persistence_failed" else {})})
def complete_reader(process,transcript,observation):
    if getattr(process,"tui28_reader_joined",False): return
    if getattr(process,"tui28_reader_join_failed",False):
        raise process.tui28_reader_join_original
    failures=[]
    while process.tui28_reader.is_alive() or not process.tui28_queue.empty():
        try: drain(process,transcript)
        except Exception as original:
            observation["originals"].append({"phase":"reader_drain","process":process,"original":original})
            failures.append(original)
    try: process.tui28_reader.join()
    except BaseException as original:
        process.tui28_reader_join_failed=True
        process.tui28_reader_join_original=original
        observation["originals"].append({"phase":"reader_join","process":process,"original":original})
        raise
    process.tui28_reader_joined=True
    if failures: raise failures[0]

def main(observation=None):
    # Optional caller retention is process-local, never a durable native sink.
    if observation is None: observation={}
    observation.update({"originals":[],"processes":[],"candidate":None,"primaryPresent":False,"primary":None})
    try: return _main_attempt(observation)
    except BaseException as original:
        if observation["primaryPresent"]:
            if original is not observation["primary"]:
                observation["originals"].append({"phase":"finalization","original":original})
            raise observation["primary"]
        observation["originals"].append({"phase":"invocation","original":original})
        raise

if __name__=="__main__": main()
