"""Matching-host candidate acquisition/build and durable ownership pipeline.
All receipt bodies and subprocess outputs remain private except the unchanged
five public custody projections. Source review is never an execution receipt.
"""
import argparse, hashlib, json, os, pathlib, platform, shutil, stat, subprocess, sys
CORE="2633c07be25512d0a84f9bfa28de6be5edff35e8"
POLICY="159d644c161cf532c94d3bfe17ed55e32bf94c5d2843928945c450f6d8140c12"

def digest(data): return hashlib.sha256(data).hexdigest()
def file_hash(path):
    with open(path,"rb") as stream: return hashlib.file_digest(stream,"sha256").hexdigest()
def write(root,name,value):
    with open(os.path.join(root,name),"x",encoding="utf-8",newline="") as stream:
        json.dump(value,stream,separators=(",",":")); stream.flush(); os.fsync(stream.fileno())
def parents(path):
    current=pathlib.Path(path).absolute()
    while True:
        entry=os.lstat(current)
        if stat.S_ISLNK(entry.st_mode) or getattr(entry,"st_file_attributes",0)&0x400: raise RuntimeError("linked custody path")
        if current.parent==current: return
        current=current.parent

def run(argv,cwd,env):
    # Actual children are directly waited; no log text enters public evidence.
    child=subprocess.Popen(argv,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    stdout,stderr=child.communicate()
    if child.returncode!=0: raise RuntimeError("native acquisition/build child failed")
    return stdout

def birth(pid):
    if sys.platform=="win32":
        from tui28_windows_identity import process_birth
        return process_birth(pid)
    if sys.platform!="linux": raise RuntimeError("scoped host denied")
    with open("/proc/%d/stat"%pid,encoding="utf-8") as stream: fields=stream.read().rsplit(") ",1)[1].split()
    return {"platform":"linux","parentPID":int(fields[1]),"startTimeTicks":fields[19],"image":os.readlink("/proc/%d/exe"%pid)}

def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--root",required=True); parser.add_argument("--source",required=True); parser.add_argument("--version",required=True); parser.add_argument("--controller-pid",required=True,type=int); parser.add_argument("--node",required=True); args=parser.parse_args()
    host={"win32":"win32","linux":"linux"}.get(sys.platform)
    if host is None or platform.machine().lower() not in ("amd64","x86_64"): raise RuntimeError("actual host/architecture denied")
    source=os.path.abspath(args.source); parents(source); parent=os.path.dirname(os.path.abspath(args.root)); parents(parent)
    os.mkdir(args.root); root=os.path.abspath(args.root); parents(root)
    workspace=os.path.join(root,"workspace"); registry=os.path.join(root,"registry"); instances=os.path.join(registry,"instances.json"); ports=os.path.join(registry,"ports.json")
    if len({workspace,instances,ports})!=3 or any(os.path.lexists(item) for item in (workspace,instances,ports)): raise RuntimeError("owned initial absence denied")
    os.mkdir(registry)
    tools={name:shutil.which(name) for name in ("git","go","npm")}; tools["node"]=os.path.abspath(args.node);tools["python"]=sys.executable
    tools={name:os.path.realpath(image) if image else None for name,image in tools.items()}
    if any(not value for value in tools.values()): raise RuntimeError("required tool missing")
    tool_refs=[]
    for name,image in tools.items():
        parents(image); entry=os.stat(image)
        if not stat.S_ISREG(entry.st_mode) or not entry.st_size: raise RuntimeError("tool image denied")
        tool_refs.append({"name":name,"image":image,"sha256":file_hash(image),"size":entry.st_size})
    writer=birth(os.getpid()); controller=birth(args.controller_pid)
    if os.getppid()!=args.controller_pid or writer["parentPID"]!=args.controller_pid or os.path.normcase(os.path.realpath(controller["image"]))!=os.path.normcase(os.path.realpath(args.node)): raise RuntimeError("actual controller/writer custody denied")
    env={key:os.environ[key] for key in ("APPDATA","COMSPEC","LOCALAPPDATA","PATHEXT","PATH","SYSTEMROOT","TEMP","TMP","USERPROFILE","WINDIR","LANG","LC_ALL","HOME","SHELL") if key in os.environ}
    home=os.path.join(root,"user-home");os.mkdir(home)
    env["HOME"]=home;env["USERPROFILE"]=home
    if host=="win32":
        env["APPDATA"]=os.path.join(home,"roaming");env["LOCALAPPDATA"]=os.path.join(home,"local");os.mkdir(env["APPDATA"]);os.mkdir(env["LOCALAPPDATA"])
    env.update({"GOFLAGS":"","GOWORK":"off","CGO_ENABLED":"0","GOCACHE":os.path.join(root,"go-cache"),"GOMODCACHE":os.path.join(root,"go-modules"),"SERVICE_LASSO_WORKSPACE_ROOT":workspace,"SERVICE_LASSO_INSTANCE_REGISTRY_PATH":instances,"SERVICE_LASSO_HOST_PORT_REGISTRY_PATH":ports})
    commit=run([tools["git"],"rev-parse","HEAD"],source,env).decode().strip(); tree=run([tools["git"],"rev-parse","HEAD^{tree}"],source,env).decode().strip(); dirty=run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],source,env)
    if dirty or commit!=os.environ.get("GITHUB_SHA") or os.environ.get("GITHUB_REF")!="refs/heads/develop" or args.version.split("-")[-1]!=commit[:7]: raise RuntimeError("native clean source denied")
    policy=os.path.join(source,".governance","project","ga-platform-scope.json")
    if file_hash(policy)!=POLICY: raise RuntimeError("native source policy denied")
    tracked=run([tools["git"],"ls-files","-z"],source,env).split(b"\0"); inventory=[]
    for relative in tracked:
        if not relative: continue
        name=relative.decode("utf-8"); file=os.path.join(source,name);parents(file); entry=os.stat(file)
        if not stat.S_ISREG(entry.st_mode): raise RuntimeError("nonregular tracked source")
        inventory.append({"name":name,"sha256":file_hash(file),"size":entry.st_size})
    inventory_bytes=json.dumps(inventory,separators=(",",":"),sort_keys=True).encode()
    write(root,"owner-private-input-custody.json",{"classification":"owner-private","producerPID":args.controller_pid,"producerBirth":controller,"writerPID":os.getpid(),"writerBirth":writer,"tools":tool_refs,"sourceCommit":commit,"sourceTree":tree,"inventory":inventory,"ownedPaths":[workspace,instances,ports],"environment":env})
    write(root,"input-custody.json",{"schemaVersion":1,"kind":"tui20-native-input-custody","source":{"tuiCommit":commit,"tuiTree":tree,"tuiDirtyHash":digest(dirty),"tuiInventoryHash":digest(inventory_bytes)},"core":{"requestedCommit":CORE},"ownership":{"threeDistinctPaths":True,"allParentsNonLink":True,"registriesInitiallyAbsent":True},"verification":{"freshEnvironment":True,"requiredToolsVerified":sorted(tools)}})
    # First acquisition cannot proceed until the actual parent independently
    # observes this live child and compares its OS birth/image/parent tuple.
    print(json.dumps({"event":"input-held","writerPID":os.getpid(),"writerBirth":writer,"producerPID":args.controller_pid,"sourceCommit":commit},separators=(",",":")),flush=True)
    if sys.stdin.readline(16)!="admit\n": raise RuntimeError("parent observation handoff denied")
    native_python=sys.executable
    if host=="win32":
        venv=os.path.join(root,"conpty-python")
        run([sys.executable,"-s","-m","venv",venv],source,env)
        native_python=os.path.join(venv,"Scripts","python.exe")
        run([native_python,"-s","-m","pip","install","--require-hashes","-r",os.path.join(source,"scripts","requirements-conpty.txt")],source,env)
        write(root,"owner-private-conpty-input.json",{"classification":"owner-private","pythonImage":native_python,"pythonSha256":file_hash(native_python),"requirementsSha256":file_hash(os.path.join(source,"scripts","requirements-conpty.txt"))})
    core=os.path.join(root,"core-source");os.mkdir(core)
    run([tools["git"],"init","-q"],core,env);run([tools["git"],"remote","add","origin","https://github.com/service-lasso/service-lasso.git"],core,env)
    run([tools["git"],"fetch","--no-tags","origin","develop"],core,env);run([tools["git"],"fetch","--depth=1","origin",CORE],core,env);run([tools["git"],"checkout","--detach","-q","FETCH_HEAD"],core,env)
    run([tools["git"],"merge-base","--is-ancestor",CORE,"origin/develop"],core,env)
    core_commit=run([tools["git"],"rev-parse","HEAD"],core,env).decode().strip(); core_tree=run([tools["git"],"rev-parse","HEAD^{tree}"],core,env).decode().strip(); core_dirty=run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],core,env)
    if core_commit!=CORE or core_dirty: raise RuntimeError("Core clean source denied")
    write(root,"core-source-binding.json",{"schemaVersion":1,"kind":"tui20-native-core-source-binding","coreCommit":CORE,"coreTree":core_tree,"coreDirtyHash":digest(core_dirty)})
    npm=tools["npm"]
    # npm.cmd is a literal trusted tool path on Windows; invoke cmd explicitly
    # with fixed command arguments and no caller-controlled command text.
    if sys.platform=="win32":
        run([env["COMSPEC"],"/d","/s","/c",'"'+npm+'" ci'],core,env);run([env["COMSPEC"],"/d","/s","/c",'"'+npm+'" run build'],core,env)
    else: run([npm,"ci"],core,env);run([npm,"run","build"],core,env)
    if run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],core,env): raise RuntimeError("Core build altered clean source")
    run([tools["node"],os.path.join(source,"scripts","assert-go-source-provenance.mjs")],source,env)
    run([tools["go"],"test","-mod=readonly","./..."],source,env)
    executable=os.path.join(root,"service-lasso-tui.exe" if host=="win32" else "service-lasso-tui")
    run([tools["go"],"build","-mod=readonly","-buildvcs=true","-trimpath","-ldflags=-s -w","-o",executable,"./cmd/service-lasso-tui"],source,env)
    vcs=run([tools["go"],"version","-m",executable],source,env).decode(); lines={line.strip() for line in vcs.splitlines()}
    if "build\tvcs.revision="+commit not in lines or "build\tvcs.modified=false" not in lines or "build\tGOARCH=amd64" not in lines or "build\tGOOS="+("windows" if host=="win32" else "linux") not in lines: raise RuntimeError("native compiled VCS identity denied")
    binary={"sha256":file_hash(executable),"size":os.stat(executable).st_size}
    write(root,"binary-digest.json",{"schemaVersion":1,"kind":"tui20-native-binary-digest",**binary});write(root,"build-output.json",{"schemaVersion":1,"kind":"tui20-native-build-output","tuiCommit":commit,"nativeBinary":binary})
    owner="tui28-native-windows-five-action.py" if host=="win32" else "tui28-native-linux-five-action.py"
    run([native_python,"-s",os.path.join(source,"scripts","tui28-native-owner-observer.py"),"--owner",os.path.join(source,"scripts",owner),"--root",root,"--executable",executable,"--source-commit",commit,"--core-commit",CORE,"--runtime-script",os.path.join(source,"scripts","tui20-native-runtime.mjs"),"--node",tools["node"]],source,env)
    if file_hash(executable)!=binary["sha256"] or run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],source,env) or run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],core,env): raise RuntimeError("post-native source/binary custody denied")
if __name__=="__main__": main()
