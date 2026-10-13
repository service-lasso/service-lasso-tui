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

class NativeChildFailure(RuntimeError):
    def __init__(self,observation):
        super().__init__("native acquisition/build child failed")
        self.observation=observation

def run(argv,cwd,env,observation=None):
    # Private process-local retention is not an independent durable owner.
    if observation is None: observation=[]
    item={"process":None,"stdout":None,"stderr":None,"returncode":None,"original":None,"completed":False}
    observation.append(item)
    try: child=subprocess.Popen(argv,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    except BaseException as original:
        item["original"]=original
        raise
    item["process"]=child
    try: stdout,stderr=child.communicate()
    except BaseException as original:
        item["original"]=original
        raise
    item.update({"stdout":stdout,"stderr":stderr,"returncode":child.returncode,"completed":True})
    if child.returncode!=0: raise NativeChildFailure(item)
    return stdout

run_child=run

def main(observation=None):
    if observation is None: observation={}
    children=[]; observation["children"]=children
    def run(argv,cwd,env): return run_child(argv,cwd,env,children)
    return _main_attempt(run)

def birth(pid):
    if sys.platform=="win32":
        from tui28_windows_identity import process_birth
        return process_birth(pid)
    if sys.platform!="linux": raise RuntimeError("scoped host denied")
    with open("/proc/%d/stat"%pid,encoding="utf-8") as stream: fields=stream.read().rsplit(") ",1)[1].split()
    return {"platform":"linux","parentPID":int(fields[1]),"startTimeTicks":fields[19],"image":os.readlink("/proc/%d/exe"%pid)}

def _main_attempt(run):
    parser=argparse.ArgumentParser(); parser.add_argument("--root",required=True); parser.add_argument("--source",required=True); parser.add_argument("--version",required=True); parser.add_argument("--controller-pid",required=True,type=int); parser.add_argument("--node",required=True); parser.add_argument("--windows-query-profile"); args=parser.parse_args()
    host={"win32":"win32","linux":"linux"}.get(sys.platform)
    if host is None or platform.machine().lower() not in ("amd64","x86_64"): raise RuntimeError("actual host/architecture denied")
    if host=="win32":
        if not args.windows_query_profile or not os.path.isabs(args.windows_query_profile): raise RuntimeError("reviewed Windows query profile required")
        parents(args.windows_query_profile)
        if not os.path.isfile(args.windows_query_profile): raise RuntimeError("query profile input missing")
    elif args.windows_query_profile is not None: raise RuntimeError("foreign Windows query input")
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
    # Explicit source-selected baseline; GOENV=off alone does not clear ambient inputs.
    # Exact tool/source/overlay custody is independently admitted before this producer runs.
    goroot=os.path.dirname(os.path.dirname(tools["go"]));parents(goroot)
    if not os.path.isfile(os.path.join(goroot,"VERSION")) or not os.path.isdir(os.path.join(goroot,"src")) or not os.path.isdir(os.path.join(goroot,"pkg","tool")):
        raise RuntimeError("held Go tool root shape denied")
    env.update({"GOROOT":goroot,"GOOS":"windows" if host=="win32" else "linux","GOARCH":"amd64","GOAMD64":"v1","GOENV":"off","GOTOOLCHAIN":"local","GOFIPS140":"off","GOEXPERIMENT":"","GOFLAGS":"","GOWORK":"off","CGO_ENABLED":"0","GOCACHEPROG":"","GOCACHE":os.path.join(root,"go-cache"),"GOMODCACHE":os.path.join(root,"go-modules"),"SERVICE_LASSO_WORKSPACE_ROOT":workspace,"SERVICE_LASSO_INSTANCE_REGISTRY_PATH":instances,"SERVICE_LASSO_HOST_PORT_REGISTRY_PATH":ports})
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
    overlay_flags=[]
    if host=="win32":
        # Extend the physically existing allowAsmABI lower package. Never write
        # the installed toolchain or depend on discovery of an overlay-only directory.
        replacements={};overlay_inputs=[]
        generated_query_go=os.path.join(root,"servicelasso_query_regions_windows_amd64.go")
        generated_query_header=os.path.join(root,"windows-query-regions.h")
        query_association=json.loads(run([tools["node"],os.path.join(source,"scripts","emit-windows-query-regions.mjs"),
            args.windows_query_profile,generated_query_go,generated_query_header],source,env))
        if query_association.get("kind")!="windows-query-build-association" or query_association.get("sourceCommit")!=commit or \
            query_association.get("profileSHA256")!=file_hash(args.windows_query_profile) or \
            query_association.get("goSHA256")!=file_hash(generated_query_go) or \
            query_association.get("headerSHA256")!=file_hash(generated_query_header): raise RuntimeError("query emitted byte association changed")
        # This record binds build bytes only; genuine containing reservation,
        # exact provider/profile custody and full native factory remain required.
        write(root,"owner-private-query-build-association.json",query_association)
        for name in ("servicelasso_root_windows_amd64.go","servicelasso_call_windows_amd64.go","servicelasso_call_windows_amd64.s",
                     "servicelasso_endpoint_windows_amd64.go","servicelasso_endpoint_windows_amd64.s","servicelasso_arguments_windows_amd64.go",
                     "servicelasso_private_windows_amd64.go",
                     "servicelasso_adopt_windows_amd64.go",
                     "servicelasso_suspend_windows_amd64.go","servicelasso_suspend_windows_amd64.s",
                     "servicelasso_creator_memory_windows_amd64.go",
                     "servicelasso_creator_memory_windows_amd64.s",
                     "servicelasso_creator_extent_windows_amd64.go",
                     "servicelasso_creator_extent_windows_amd64.s",
                     "servicelasso_creator_tls_windows_amd64.go",
                     "servicelasso_creator_tls_windows_amd64.s",
                     "servicelasso_creator_export_windows_amd64.go",
                     "servicelasso_creator_export_windows_amd64.s",
                     "servicelasso_creator_delay_windows_amd64.go",
                     "servicelasso_creator_import_windows_amd64.go",
                     "servicelasso_creator_import_windows_amd64.s",
                     "servicelasso_creator_relocation_windows_amd64.go",
                     "servicelasso_creator_relocation_windows_amd64.s",
                     "servicelasso_creator_nativepe_windows_amd64.go",
                     "servicelasso_creator_nativepe_windows_amd64.s",
                     "servicelasso_creator_view_windows_amd64.go",
                     "servicelasso_creator_pe_windows_amd64.go",
                     "servicelasso_creator_read_windows_amd64.go",
                     "servicelasso_creator_read_windows_amd64.s",
                     "servicelasso_creator_metadata_windows_amd64.go",
                     "servicelasso_creator_metadata_windows_amd64.s",
                     "servicelasso_creator_file_windows_amd64.go",
                     "servicelasso_creator_file_windows_amd64.s",
                     "servicelasso_creator_name_windows_amd64.go",
                     "servicelasso_creator_name_windows_amd64.s",
                     "servicelasso_creator_modules_windows_amd64.go",
                     "servicelasso_creator_modules_windows_amd64.s",
                     "servicelasso_creator_image_windows_amd64.go",
                     "servicelasso_creator_image_windows_amd64.s",
                     "servicelasso_mapping_windows_amd64.go","servicelasso_mapping_windows_amd64.s",
                     "servicelasso_reader_windows_amd64.go","servicelasso_reader_windows_amd64.s",
                     "servicelasso_creator_pe_windows_amd64_test.go",
                     "servicelasso_decode_binding_windows_amd64.go","servicelasso_decode_binding_windows_amd64_test.go",
                     "servicelasso_export_windows_amd64_test.go","servicelasso_return_windows_amd64_test.go",
                     "servicelasso_arguments_windows_amd64_test.go","servicelasso_private_windows_amd64_test.go"):
            relative=os.path.join("src","internal","runtime","syscall","windows",name)
            backing=os.path.join(source,"toolchain-overlay","go1.26.0",relative);parents(backing)
            if not os.path.isfile(backing): raise RuntimeError("owned root overlay backing missing")
            target=os.path.join(goroot,relative)
            if not os.path.isdir(os.path.dirname(target)): raise RuntimeError("physical lower package missing")
            replacements[target]=backing
            overlay_inputs.append({"target":target,"backing":backing,"backingSHA256":file_hash(backing)})
        os_relative=os.path.join("src","os","servicelasso_suspend_windows_amd64.go")
        os_backing=os.path.join(source,"toolchain-overlay","go1.26.0",os_relative);parents(os_backing)
        os_target=os.path.join(goroot,os_relative)
        if not os.path.isfile(os_backing) or not os.path.isdir(os.path.dirname(os_target)): raise RuntimeError("owned physical os overlay backing missing")
        replacements[os_target]=os_backing
        overlay_inputs.append({"target":os_target,"backing":os_backing,"backingSHA256":file_hash(os_backing)})
        query_target=os.path.join(goroot,"src","internal","runtime","syscall","windows","servicelasso_query_regions_windows_amd64.go")
        replacements[query_target]=generated_query_go
        overlay_inputs.append({"target":query_target,"backing":generated_query_go,"backingSHA256":file_hash(generated_query_go)})
        write(root,"windows-owned-overlay.json",{"Replace":replacements})
        overlay_flags=["-overlay="+os.path.join(root,"windows-owned-overlay.json")]
        write(root,"owner-private-overlay-input.json",{"classification":"owner-private","goroot":goroot,"inputs":overlay_inputs})
    run([tools["go"],"test","-mod=readonly",*overlay_flags,"./..."],source,env)
    executable=os.path.join(root,"service-lasso-tui.exe" if host=="win32" else "service-lasso-tui")
    # COFF locator construction requires the genuine internal symbol table.
    # Full owned-overlay/payload-first/native-owner-second wiring remains mandatory.
    run([tools["go"],"build","-mod=readonly","-buildvcs=true","-trimpath",*overlay_flags,"-ldflags=-linkmode=internal","-o",executable,"./cmd/service-lasso-tui"],source,env)
    vcs=run([tools["go"],"version","-m",executable],source,env).decode(); lines={line.strip() for line in vcs.splitlines()}
    if "build\tvcs.revision="+commit not in lines or "build\tvcs.modified=false" not in lines or "build\tGOARCH=amd64" not in lines or "build\tGOOS="+("windows" if host=="win32" else "linux") not in lines: raise RuntimeError("native compiled VCS identity denied")
    binary={"sha256":file_hash(executable),"size":os.stat(executable).st_size}
    write(root,"binary-digest.json",{"schemaVersion":1,"kind":"tui20-native-binary-digest",**binary});write(root,"build-output.json",{"schemaVersion":1,"kind":"tui20-native-build-output","tuiCommit":commit,"nativeBinary":binary})
    owner="tui28-native-windows-five-action.py" if host=="win32" else "tui28-native-linux-five-action.py"
    run([native_python,"-s",os.path.join(source,"scripts","tui28-native-owner-observer.py"),"--owner",os.path.join(source,"scripts",owner),"--root",root,"--executable",executable,"--source-commit",commit,"--core-commit",CORE,"--runtime-script",os.path.join(source,"scripts","tui20-native-runtime.mjs"),"--node",tools["node"]],source,env)
    if file_hash(executable)!=binary["sha256"] or run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],source,env) or run([tools["git"],"status","--porcelain=v1","--untracked-files=all"],core,env): raise RuntimeError("post-native source/binary custody denied")
if __name__=="__main__": main()
