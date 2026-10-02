import ast
import os
import pathlib
import runpy
import sys
import tempfile
import unittest
from unittest.mock import patch
import types


def load_posix_harness():
    # The receipt-order functions are platform-neutral; provide import stubs so
    # this controlled recorder-path test also runs on Windows source CI.
    if sys.platform == "linux":
        return runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py")))
    with patch.dict(sys.modules, {"fcntl": types.ModuleType("fcntl"), "pty": types.ModuleType("pty")}):
        return runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py")))


class NativeFiveActionHarnessTests(unittest.TestCase):
    @unittest.skipUnless(sys.platform in ("linux","darwin"), "requires actual POSIX runtime child and wait")
    def test_actual_direct_owner_entrypoint_retains_runtime_on_every_acquisition_error(self):
        # Core/JWKS behavior is a fixture; main/start_owned_runtime/finalization,
        # the real child handle and real wait are the production ownership path.
        harness=load_posix_harness()
        for boundary in ("birth","select","read","parse","shape"):
            with self.subTest(boundary=boundary), tempfile.TemporaryDirectory() as root:
                candidate=pathlib.Path(root,"candidate"); candidate.write_bytes(b"preflight fixture never activated")
                script=pathlib.Path(root,"runtime.py")
                line="not-json" if boundary=="parse" else "[]" if boundary=="shape" else '{"event":"ready","url":"fixture","token":"fixture","deniedToken":"fixture","jwksPort":1}'
                script.write_text("import sys,pathlib\nprint("+repr(line)+",flush=True)\nfor line in sys.stdin:\n    if line=='close\\n': break\npathlib.Path("+repr(str(pathlib.Path(root,"actual-close")))+").write_text('closed')\n",encoding="utf-8")
                actual_popen=harness["subprocess"].Popen; children=[]
                class FailedRead:
                    def __init__(self,stream): self.stream=stream
                    def fileno(self): return self.stream.fileno()
                    def readline(self): raise OSError("controlled runtime read error")
                def launch(*args,**kwargs):
                    child=actual_popen(*args,**kwargs)
                    command=args[0] if args else kwargs.get("args",[])
                    is_runtime=isinstance(command,(list,tuple)) and len(command)>1 and command[1]==str(script)
                    if is_runtime: children.append(child)
                    if is_runtime and boundary=="read": child.stdout=FailedRead(child.stdout)
                    return child
                def failed_birth(pid): raise OSError("controlled runtime birth error")
                def failed_select(*args): raise OSError("controlled runtime select error")
                argv=["owner","--root",root,"--executable",str(candidate),"--source-commit","a"*40,"--core-commit","b"*40,"--node",sys.executable,"--runtime-script",str(script)]
                with patch.object(sys,"argv",argv), patch.object(harness["subprocess"],"Popen",launch):
                    with patch.dict(harness["main"].__globals__,{"process_birth":failed_birth} if boundary=="birth" else {}), patch.object(harness["select"],"select",failed_select if boundary=="select" else harness["select"].select):
                        failure=OSError if boundary in ("birth","select","read") else harness["OwnedRuntimeAcquisitionFailure"] if boundary=="parse" else AttributeError
                        with self.assertRaises(failure): harness["main"]()
                self.assertEqual(len(children),1)
                self.assertEqual(children[0].returncode,0,"actual owner must wait its actual child")
                self.assertEqual(pathlib.Path(root,"actual-close").read_text(),"closed")
                import json
                receipt=json.loads(pathlib.Path(root,"owner-runtime-recovery.json").read_text())
                self.assertTrue(receipt["actualExitObserved"])
                self.assertEqual(receipt["runtimeChildExit"],"terminal_exited_zero")
                self.assertEqual(receipt["runtimePID"],children[0].pid)
                self.assertEqual(receipt["runtimeBirthAbsentObserved"],boundary!="birth")
                if boundary=="birth":
                    self.assertIsNone(receipt["runtimeBirth"])
                    self.assertIsNone(receipt["runtimeChildStillLiveAfterClose"])
                    self.assertEqual(receipt["outcome"],"exit_observed_birth_absence_unproven")
                for child in children:
                    child.stdout.stream.close() if boundary=="read" else child.stdout.close()

    def test_harness_records_closed_exit_and_adverse_audits_without_persisting_tokens(self):
        source = pathlib.Path(__file__).with_name("tui20-native-five-action.py").read_text(encoding="utf-8")
        ast.parse(source)
        self.assertIn('("reload is unavailable",)', source)
        self.assertIn('bounded renderer assertion consumes terminal bytes in memory only', source)
        self.assertNotIn('append_terminal', source)
        self.assertIn('"targetIds","status","outcome","cancellationSupported"', source)
        self.assertIn('"completedOperationNoReplay":True', source)
        self.assertIn('connection profile "missing" credential is unavailable', source)
        self.assertIn('terminal_exited_zero', source)
        self.assertIn('CreateFileW', source)
        self.assertIn('"adverseAudit"', source)
        self.assertIn('"coreDeniedAuditDelta"', source)
        self.assertIn('"unrelatedService"', source)
        self.assertIn('"runtimeState"', source)
        self.assertIn('expected_denials', source)
        self.assertIn('retained operation readback unavailable', source)
        self.assertNotIn('private-token.json', source)

    def test_core_fixture_defines_only_owned_profiles_and_services(self):
        source = pathlib.Path(__file__).with_name("tui20-native-runtime.mjs").read_text(encoding="utf-8")
        self.assertIn('"tui20-fixture"', source)
        self.assertIn('"tui20-unrelated"', source)
        self.assertIn('["invalid", "SERVICE_LASSO_INVALID_TOKEN"]', source)
        self.assertIn('["missing", "SERVICE_LASSO_MISSING_TOKEN"]', source)
        self.assertNotIn('private-token.json', source)
        self.assertIn('runtimePathReceipt', source)
        self.assertIn('coreReadback', source)
        self.assertLess(source.index('Object.assign(process.env, ownedRuntimeEnvironment)'), source.index('const { exportJWK, generateKeyPair, SignJWT }'))
        self.assertIn('SERVICE_LASSO_WORKSPACE_ROOT: workspaceRoot', source)
        self.assertIn('SERVICE_LASSO_INSTANCE_REGISTRY_PATH: instanceRegistryPath', source)
        self.assertIn('SERVICE_LASSO_HOST_PORT_REGISTRY_PATH: hostPortRegistryPath', source)
        controller = pathlib.Path(__file__).with_name("tui20-native-core.mjs").read_text(encoding="utf-8")
        self.assertIn('tui20-native-runtime.mjs', controller)
        self.assertIn('--adverse-controller-crash', controller)

    def test_posix_harness_uses_a_real_pty_and_retains_the_required_boundaries(self):
        source = pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py").read_text(encoding="utf-8")
        ast.parse(source)
        self.assertIn("pty.openpty()", source)
        self.assertIn('"terminal_exited_zero"', source)
        self.assertIn('terminal_signaled', source)
        self.assertIn('live_child_unresolved', source)
        self.assertIn('external recovery owner must observe a live child before finalization', source)
        self.assertIn('ChildReapedUnowned', source)
        self.assertNotIn('self.child.kill()', source)
        self.assertIn('os.open(executable,os.O_RDONLY|getattr(os,"O_NOFOLLOW",0))', source)
        self.assertIn('libc.fexecve', source)
        self.assertIn('os.fchdir(launch["directoryFD"])', source)
        self.assertIn('os.execve("./"+launch["leaf"],argv,env)', source)
        self.assertIn('"reparse"', source)
        self.assertIn('"inplace-content-mutation"', source)
        self.assertIn('linux_sealed_execution', source)
        self.assertIn('darwin_system_immutable_execution', source)
        self.assertIn('F_SEAL_WRITE', source)
        self.assertIn('schg', source)
        self.assertIn('system-immutable-held-directory-execve', source)
        self.assertIn('existing-writer in-place write', source)
        self.assertIn('Darwin immutable leaf digest mismatch', source)
        self.assertIn('persist_primary_outcome(root,outcome,writer)', source)
        self.assertIn('finalize_native_outcome(args.root,outcome', source)
        self.assertIn('Every post-start preflight stays inside this owner boundary', source)
        self.assertIn('owner-preflight-cleanup.json', source)
        self.assertIn('runtime is not None and not live_child', source)
        self.assertIn('OwnedRuntimeAcquisitionFailure', source)
        self.assertNotIn('runtime.kill()', source)
        self.assertIn('owner-finalization-failure-proof.json', source)
        self.assertIn('external-owner-finalization.json', source)
        self.assertIn('select.select([runtime.stdout],[],[],10)', source)
        self.assertIn('owner-runtime-recovery.json', source)
        self.assertIn('runtime.wait()', source)
        self.assertIn('process_birth(runtime.pid)', source)
        self.assertIn('injected native finalization cleanup failure', source)
        self.assertIn('native-cleanup-receipt.json', source)
        self.assertIn('"adverseAudit"', source)
        self.assertIn('len(set(operation_ids))!=5', source)
        self.assertIn('"mcp.operation.succeeded"', source)
        self.assertIn('"coreAuditCount"', source)
        self.assertIn('"completedOperationNoReplay":True', source)
        self.assertIn('"unrelatedService"', source)
        self.assertIn('"blocked_core_1553_no_adapter"', source)
        self.assertNotIn("private-token.json", source)

    def test_native_workflow_records_private_actual_custody_and_closed_public_receipts_before_core_fetch(self):
        workflow = pathlib.Path(__file__).parents[1] / ".github" / "workflows" / "ci.yml"
        source = workflow.read_text(encoding="utf-8")
        start = source.index("prepare_phase()")
        private_custody = source.index('TUI20_PRIVATE_CUSTODY="$phase/private-input-custody.json"', start)
        public_custody = source.index('TUI20_PUBLIC_CUSTODY="$phase/input-custody.json"', start)
        core_import = source.index('git -C "$phase/core-source" init -q', start)
        core_binding = source.index('TUI20_CORE_BINDING="$phase/core-source-binding.json"', start)
        core_dependencies = source.index('(cd "$phase/core-source" && npm ci && npm run build)', start)
        self.assertLess(private_custody, core_import)
        self.assertLess(public_custody, core_import)
        self.assertLess(core_binding, core_dependencies)
        for required in (
            'env -i PATH="$PATH"',
            'SERVICE_LASSO_WORKSPACE_ROOT="$phase/workspace"',
            'SERVICE_LASSO_INSTANCE_REGISTRY_PATH="$phase/registry/instances.json"',
            'SERVICE_LASSO_HOST_PORT_REGISTRY_PATH="$phase/registry/ports.json"',
            'test ! -e "$instances" && test ! -e "$ports"',
            'while test "$current" != "/"; do test ! -L "$current"',
            'bash dirname mkdir env git sha256sum cut xargs awk wc uname ps readlink node npm go python3',
            'go-compile', 'literalCommands', 'allParentsNonLink', 'registriesInitiallyAbsent',
            'git -C $phase/core-source init -q', 'go build -mod=readonly -buildvcs=true -trimpath',
            'kind":"tui20-native-input-custody', 'kind":"tui20-native-core-source-binding',
            'kind":"tui20-native-build-output', 'write_fsync', 'os.fsync',
        ):
            self.assertIn(required, source)
        uploaded = source[source.index('name: tui20-native-${{ matrix.name }}-lifecycle-${{ env.CI_SOURCE_SHA }}'):]
        self.assertIn('input-custody.json', uploaded)
        self.assertIn('core-source-binding.json', uploaded)
        self.assertIn('build-output.json', uploaded)
        self.assertIn('native-public-projection.json', uploaded)
        self.assertNotIn('private-input-custody.json', uploaded)
        self.assertNotIn('head-tree.json', uploaded)
        self.assertNotIn('native-terminal.txt', uploaded)
        for private_name in ('ready.json', 'owner-birth.json', 'owner-close.json',
                             'core-parent-exit.json', 'external-owner-live.json',
                             'owner-death-recovery.json', 'operation-audit.json'):
            self.assertNotIn(private_name, uploaded)
        self.assertIn("bash -euo pipefail <<'TUI20_PHASE'", source)
        self.assertIn("awk '{print $22}' /proc/$$/stat", source)
        self.assertIn('current="$(dirname "$current")"', source)
        self.assertIn('            test ! -L "$current"', source)
        self.assertNotIn('awk "{print \\\\$22}"', source)

    def test_observer_keeps_lineage_private_then_closes_its_owned_runtime(self):
        observer = pathlib.Path(__file__).with_name("tui20-native-owner-observer.py").read_text(encoding="utf-8")
        for phase in ('"initial"', '"witness"', '"unresolved"', '"closed"'):
            self.assertIn('write_private(args.root,'+phase, observer)
        self.assertIn('adverse_kill_observed=(status == -9', observer)
        self.assertIn('write_private(args.root,"unresolved"', observer)
        self.assertIn('result=close_runtime(runtime)', observer)
        self.assertLess(observer.index('write_private(args.root,"unresolved"'), observer.index('result=close_runtime(runtime)', observer.index('write_private(args.root,"unresolved"')))
        self.assertIn('native-public-projection.json', observer)
        self.assertIn('runtimeStdoutClosed', observer)
        self.assertIn('runtimeStderrClosed', observer)
        self.assertIn('ownerStdoutClosed', observer)
        self.assertIn('ownerStderrClosed', observer)

    def test_native_pty_bytes_never_persist_to_a_file(self):
        posix = pathlib.Path(__file__).with_name("tui20-native-posix-five-action.py").read_text(encoding="utf-8")
        windows = pathlib.Path(__file__).with_name("tui20-native-five-action.py").read_text(encoding="utf-8")
        self.assertIn('Terminal bytes are required in memory', posix)
        self.assertIn('self.text+=chunk', posix)
        self.assertNotIn('native-terminal.txt', posix)
        self.assertNotIn('self.log', posix)
        self.assertNotIn('with open(self.log', posix)
        self.assertIn('bounded renderer assertion consumes terminal bytes in memory only', windows)
        self.assertNotIn('native-terminal.txt', windows)
        self.assertNotIn('append_terminal', windows)

    @unittest.skipUnless(sys.platform == "linux", "requires Linux memfd seals")
    def test_linux_kernel_seal_rejects_an_in_place_write(self):
        harness = load_posix_harness()
        with tempfile.NamedTemporaryFile() as candidate:
            candidate.write(b"trusted native executable bytes")
            candidate.flush()
            held, identity, _ = harness["hold_candidate"](candidate.name, "a" * 40)
            sealed = None
            try:
                sealed, receipt = harness["linux_sealed_execution"](held, identity)
                self.assertEqual(receipt["mechanism"], "memfd-fexecve-seals")
                with self.assertRaises(OSError):
                    os.pwrite(sealed, b"X", 0)
            finally:
                if sealed is not None:
                    os.close(sealed)
                os.close(held)

    def test_cleanup_failure_is_recorded_after_primary_without_replacing_it(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "succeeded", "terminals": [{"exit": "terminal_exited_zero"}]}
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        cleanup = lambda *_: {"outcome":"failed","closedReason":"leaf_release_failed","recoveryRetained":True}
        actual = harness["finalize_native_outcome"]("controlled-root", primary, None, object(), -1, cleanup=cleanup, writer=record)
        self.assertEqual(actual["closedReason"], "leaf_release_failed")
        self.assertEqual(writes, [("native-exit-receipt.json", primary), ("native-cleanup-receipt.json", actual)])

    @unittest.skipUnless(os.name == "posix", "requires POSIX held recovery directory")
    def test_natural_darwin_cleanup_return_reaches_actual_owner_and_observer_gate(self):
        # Controlled terminal/runtime behavior is not native acceptance. The
        # actual main -> finalize -> Darwin release natural return and actual
        # observer main gate run; retained filesystem recovery is inspected.
        import io, json
        harness=load_posix_harness()
        observer=runpy.run_path(str(pathlib.Path(__file__).with_name("tui20-native-owner-observer.py")))
        with tempfile.TemporaryDirectory() as root:
            candidate=pathlib.Path(root,"candidate"); candidate.write_bytes(b"cleanup gate fixture")
            directory=pathlib.Path(root,"protected"); directory.mkdir()
            leaf=directory/"launch"; leaf.write_bytes(candidate.read_bytes())
            recovery={"path":str(leaf),"directory":str(directory),"leafFD":os.open(leaf,os.O_RDONLY),"directoryFD":os.open(directory,os.O_RDONLY),"systemImmutable":0x20000}
            birth={"platform":"fixture","startTime":"controlled"}
            paths={"coreReadback":True,"uniqueOwnedPaths":["workspaceRoot","instanceRegistryPath","hostPortRegistryPath"],"runtimeInstanceBound":True}
            pathlib.Path(root,"ready.json").write_text(json.dumps({"coreCommit":"b"*40,"runtimePathReceipt":paths}))
            handoff={"url":"fixture","token":"fixture","deniedToken":"fixture","jwksPort":1,"_runtimePID":202,"_runtimeBirth":birth}
            class Terminal:
                def __init__(self,*_): self.child=types.SimpleNamespace(pid=303,tui20_birth=birth,reaped_unowned=False,poll=lambda:0); self.exit_reason=None
                def wait(self,*_): pass
                def close(self): self.exit_reason="terminal_exited_zero"; return self.exit_reason
            argv=["owner","--root",root,"--executable",str(candidate),"--source-commit","a"*40,"--core-commit","b"*40,"--external-runtime","--adverse-controller-crash","--controller-pid","404"]
            chflags=[]
            def denied_release(command,**_): chflags.append(command); return types.SimpleNamespace(returncode=1)
            try:
                with patch.object(sys,"argv",argv), patch.object(sys,"stdin",io.StringIO(json.dumps(handoff)+"\n")), patch.object(harness["subprocess"],"run",denied_release), patch.object(harness["urllib"].request,"urlopen",lambda *_args,**_kwargs:io.BytesIO(b"{}")), patch.dict(harness["main"].__globals__,{"Terminal":Terminal,"bound_execution":lambda *_:(recovery,recovery,{"platform":"darwin"}),"detail":lambda *_:None,"controller_failed":lambda *_:True,"request":lambda *_:{},"process_birth":lambda *_:birth}):
                    harness["main"]()
                primary=json.loads(pathlib.Path(root,"native-exit-receipt.json").read_text())
                cleanup=json.loads(pathlib.Path(root,"native-cleanup-receipt.json").read_text())
                finalization=json.loads(pathlib.Path(root,"external-owner-finalization.json").read_text())
                self.assertEqual(primary["outcome"],"succeeded")
                self.assertEqual(primary["terminals"][0]["exit"],"terminal_exited_zero")
                self.assertEqual(cleanup,{"outcome":"failed","closedReason":"leaf_release_failed","recoveryRetained":True})
                self.assertEqual(finalization["primaryOutcome"],"succeeded")
                self.assertEqual(finalization["finalizationFailure"],"cleanup_failed")
                self.assertEqual(chflags,[["sudo","-n","/usr/bin/chflags","noschg",str(leaf)]])
                self.assertEqual(leaf.read_bytes(),candidate.read_bytes())
                self.assertTrue(directory.is_dir())
                os.fstat(recovery["leafFD"]); os.fstat(recovery["directoryFD"])
                # Consume the real owner's receipt through the actual observer
                # aggregate path. Child handles are explicit controlled fixtures.
                owner=types.SimpleNamespace(pid=os.getpid(),stdin=io.StringIO(),stdout=io.StringIO(),stderr=io.StringIO(),wait=lambda:0)
                runtime=types.SimpleNamespace(pid=202,tui20_birth=birth)
                closed={"runtimeChildExit":"terminal_exited_zero","shutdownRequested":True,"actualExitObserved":True,"runtimeBirthAbsentObserved":True,"runtimeChildStillLiveAfterClose":False,"runtimeStdoutClosed":True,"runtimeStderrClosed":True}
                def start(_args,retain): retain(runtime); return runtime,dict(handoff)
                observer_argv=["observer","--owner","controlled-owner","--root",root,"--executable",str(candidate),"--source-commit","a"*40,"--core-commit","b"*40,"--runtime-script","controlled-runtime","--node","controlled-node"]
                with patch.object(sys,"argv",observer_argv), patch.object(observer["subprocess"],"Popen",lambda *_args,**_kwargs:owner), patch.dict(observer["main"].__globals__,{"start_runtime":start,"process_birth":lambda *_:birth,"birth_absent":lambda *_:True,"close_runtime":lambda *_:closed}):
                    with self.assertRaises(SystemExit) as denied: observer["main"]()
                self.assertEqual(denied.exception.code,1)
                projection=json.loads(pathlib.Path(root,"native-public-projection.json").read_text())
                self.assertEqual(projection["result"],"failed")
                self.assertFalse(projection["actionsPassed"])
                self.assertTrue(projection["ownedRuntimeClosed"])
                proof=json.loads(pathlib.Path(root,"owner-finalization-failure-proof.json").read_text())
                self.assertEqual(proof["finalizationFailure"],"cleanup_failed")
                self.assertEqual(json.loads(pathlib.Path(root,"native-exit-receipt.json").read_text()),primary)
                self.assertTrue(leaf.exists(),"aggregate denial must retain failed cleanup recovery")
            finally:
                os.close(recovery["leafFD"]); os.close(recovery["directoryFD"])

    @unittest.skipUnless(sys.platform == "linux", "requires actual Linux sealed bytes and pwrite")
    def test_actual_replacement_probe_separates_inode_and_digest_and_restores_mutated_bytes(self):
        harness = load_posix_harness()
        with tempfile.TemporaryDirectory() as root:
            candidate=pathlib.Path(root,"candidate")
            original=b"actual mutable source bytes for contract regression"
            candidate.write_bytes(original)
            held,identity,inode=harness["hold_candidate"](str(candidate),"a"*40)
            sealed,_=harness["linux_sealed_execution"](held,identity)
            observations=[]
            # Only terminal rendering is controlled here. The actual probe
            # replaces the path and mutates/restores the real held source inode;
            # every launch verifies the actual kernel-sealed copy's bytes.
            class ObservedTerminal:
                def __init__(self,launch,executable,profile,env):
                    self_case.assertEqual(harness["sha256_fd"](launch),identity["binarySHA256"])
                    self_case.assertEqual((os.fstat(held).st_dev,os.fstat(held).st_ino),inode)
                    observations.append(harness["sha256_fd"](held))
                def wait(self,need,seconds): pass
                def close(self,expected): return harness["terminal_exit_reason"](expected)
            self_case=self
            try:
                with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":ObservedTerminal}):
                    results=harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                self.assertEqual([item["attack"] for item in results],["rename","reparse","inplace-content-mutation"])
                self.assertEqual(observations[:2],[identity["binarySHA256"]]*2)
                self.assertNotEqual(observations[2],identity["binarySHA256"])
                self.assertEqual(candidate.read_bytes(),original)
                self.assertEqual(harness["sha256_fd"](held),identity["binarySHA256"])
                self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-held-original")))
                with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":ObservedTerminal}):
                    repeated=harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                self.assertEqual(len(repeated),3)
                self.assertEqual(candidate.read_bytes(),original)
                self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-held-original")))
                class FailingMutationTerminal(ObservedTerminal):
                    def wait(self,need,seconds):
                        if observations[-1]!=identity["binarySHA256"]: raise RuntimeError("controlled mutation launch assertion")
                with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":FailingMutationTerminal}):
                    with self.assertRaisesRegex(RuntimeError,"controlled mutation launch assertion"):
                        harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                self.assertEqual(candidate.read_bytes(),original,"actual source byte must restore even when the mutation launch assertion fails")
                self.assertEqual(harness["sha256_fd"](held),identity["binarySHA256"])
                self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-held-original")))
                for failure_at in (1,2):
                    class FailingReplacementTerminal(ObservedTerminal):
                        count=0
                        def wait(self,need,seconds):
                            type(self).count+=1
                            if self.count==failure_at: raise RuntimeError("controlled replacement interruption")
                    with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":FailingReplacementTerminal}):
                        with self.assertRaisesRegex(RuntimeError,"controlled replacement interruption"):
                            harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                    self.assertEqual(candidate.read_bytes(),original)
                    self.assertEqual((candidate.stat().st_dev,candidate.stat().st_ino),inode)
                    self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-held-original")))
                    self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-untrusted-replacement")))
                staged=pathlib.Path(root,".tui20-untrusted-replacement.link")
                actual_replace=os.replace
                for substitute in (False,True):
                    reached=[]
                    foreign_staged=pathlib.Path(root,"foreign-staged-link")
                    if substitute: os.symlink("foreign target must remain",foreign_staged)
                    def fail_staged_rename(source,target):
                        if str(source)==str(staged):
                            self.assertTrue(staged.is_symlink(),"failure must occur after actual staged symlink creation")
                            reached.append(os.lstat(staged).st_ino)
                            if substitute: actual_replace(foreign_staged,staged)
                            raise OSError("controlled staged rename failure")
                        return actual_replace(source,target)
                    with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":ObservedTerminal}), patch.object(os,"replace",fail_staged_rename):
                        with self.assertRaisesRegex(OSError,"controlled staged rename failure"):
                            harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                    self.assertEqual(len(reached),1)
                    self.assertEqual(candidate.read_bytes(),original)
                    self.assertEqual((candidate.stat().st_dev,candidate.stat().st_ino),inode)
                    self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-held-original")))
                    self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-untrusted-replacement")))
                    if substitute:
                        self.assertTrue(staged.is_symlink())
                        self.assertEqual(os.readlink(staged),"foreign target must remain")
                        with self.assertRaisesRegex(RuntimeError,"probe paths already exist"):
                            harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                        # This is the test-created foreign fixture, never probe cleanup.
                        staged.unlink()
                    else:
                        self.assertFalse(os.path.lexists(staged))
                        with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":ObservedTerminal}):
                            repeated=harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                        self.assertEqual(len(repeated),3)
                        self.assertEqual(candidate.read_bytes(),original)
                        self.assertFalse(os.path.lexists(staged))
                        self.assertFalse(os.path.lexists(pathlib.Path(root,".tui20-held-original")))
                foreign=pathlib.Path(root,"foreign-alias")
                foreign.write_bytes(b"unowned replacement alias must remain")
                class ForeignAliasTerminal(ObservedTerminal):
                    def wait(self,need,seconds):
                        os.replace(foreign,pathlib.Path(root,".tui20-held-original"))
                        raise RuntimeError("controlled foreign alias substitution")
                with patch.dict(harness["bound_replacement_probe"].__globals__,{"Terminal":ForeignAliasTerminal}):
                    with self.assertRaisesRegex(RuntimeError,"original alias identity changed"):
                        harness["bound_replacement_probe"](sealed,held,str(candidate),inode,identity,"missing",{})
                self.assertEqual(pathlib.Path(root,".tui20-held-original").read_bytes(),b"unowned replacement alias must remain")
            finally:
                os.close(sealed); os.close(held)

    def test_cleanup_receipt_writer_failure_cannot_replace_primary_outcome(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "failed", "failureReason": "native_harness_assertion_failed"}
        def record(path, value):
            writes.append((pathlib.Path(path).name, value))
            if path.endswith("native-cleanup-receipt.json"): raise OSError("controlled recorder failure")
        cleanup = lambda *_: {"outcome":"failed","closedReason":"protected_artifact_removal_failed","recoveryRetained":True}
        actual = harness["finalize_native_outcome"]("controlled-root", primary, None, object(), -1, cleanup=cleanup, writer=record)
        self.assertEqual(actual["recoveryRetained"], True)
        self.assertEqual(writes[0], ("native-exit-receipt.json", primary))

    def test_injected_cleanup_failure_can_be_observed_without_rewriting_primary(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "succeeded", "terminals": [{"exit": "terminal_exited_zero"}]}
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        def failing_cleanup(*_): raise RuntimeError("injected native finalization cleanup failure")
        with self.assertRaisesRegex(RuntimeError, "injected native finalization cleanup failure"):
            harness["finalize_native_outcome"]("controlled-root", primary, None, None, -1, cleanup=failing_cleanup, writer=record)
        self.assertEqual(writes, [("native-exit-receipt.json", primary)])

    def test_negative_owned_child_status_is_signaled_not_nonzero(self):
        harness = load_posix_harness()
        self.assertEqual(harness["terminal_exit_reason"](-9), "terminal_signaled")

    def test_finalization_rejects_a_live_child_without_external_recovery(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "failed", "terminals": [{"exit": "terminal_unknown"}]}
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        with self.assertRaisesRegex(RuntimeError, "external recovery owner"):
            harness["finalize_native_outcome"]("controlled-root", primary, object(), object(), -1, live_child=True, writer=record)
        self.assertEqual(writes, [])

    def test_partial_darwin_activation_is_carried_to_closed_cleanup(self):
        harness = load_posix_harness()
        writes = []
        primary = {"outcome": "failed", "heldExecutableBinding": {"partialActivationRecovery": True}}
        recovery = object()
        def record(path, value): writes.append((pathlib.Path(path).name, value))
        def rollback(launch_fd, darwin_protected, held, live_child):
            self.assertIs(launch_fd, recovery)
            self.assertIs(darwin_protected, recovery)
            self.assertFalse(live_child)
            return {"outcome":"failed","closedReason":"parent_release_failed","recoveryRetained":True}
        actual = harness["finalize_native_outcome"]("controlled-root", primary, recovery, recovery, -1, cleanup=rollback, writer=record)
        self.assertTrue(actual["recoveryRetained"])
        self.assertEqual(writes, [("native-exit-receipt.json", primary), ("native-cleanup-receipt.json", actual)])

    @unittest.skipUnless(os.name == "posix", "requires POSIX directory descriptors")
    def test_controlled_darwin_leaf_activation_parent_failure_and_rollback_failure_retains_recovery(self):
        harness = load_posix_harness()
        module_os, module_subprocess, module_stat = harness["os"], harness["subprocess"], harness["stat"]
        actual_open, actual_fstat = module_os.open, module_os.fstat
        state = {"leafActive": False}
        with tempfile.TemporaryDirectory() as root:
            candidate_path = pathlib.Path(root, "candidate")
            candidate_path.write_bytes(b"trusted native executable bytes")
            held = actual_open(str(candidate_path), module_os.O_RDONLY)
            identity = {"binarySHA256": harness["sha256_fd"](held)}
            def run(command, **_kwargs):
                action, target = command[-2:]
                if action == "schg" and target.endswith("launch"):
                    state["leafActive"] = True
                    return types.SimpleNamespace(returncode=0)
                return types.SimpleNamespace(returncode=1)
            def open_protected(path, flags, *args):
                if state["leafActive"] and path.endswith("launch") and flags & module_os.O_RDWR:
                    raise OSError("controlled immutable denial")
                return actual_open(path, flags, *args)
            def fstat_immutable(fd):
                value = actual_fstat(fd)
                return types.SimpleNamespace(st_dev=value.st_dev, st_ino=value.st_ino, st_flags=getattr(module_stat, "SF_IMMUTABLE", 0x00020000))
            def pwrite_denied(fd, data, offset):
                if state["leafActive"]:
                    raise OSError("controlled immutable denial")
                return len(data)
            with patch.object(module_os, "stat_result", types.SimpleNamespace(st_flags=1)), patch.object(module_os, "open", open_protected), patch.object(module_os, "fstat", fstat_immutable), patch.object(module_os, "pwrite", pwrite_denied, create=True), patch.object(module_subprocess, "run", run):
                with self.assertRaises(harness["DarwinActivationFailure"]) as raised:
                    harness["darwin_system_immutable_execution"](held, identity, root)
                recovery = raised.exception.recovery
                primary = {"outcome":"failed", "heldExecutableBinding":{"partialActivationRecovery":True}}
                writes = []
                actual = harness["finalize_native_outcome"](root, primary, recovery, recovery, held, writer=lambda path, value: writes.append((pathlib.Path(path).name, value)))
            self.assertEqual(actual, {"outcome":"failed","closedReason":"leaf_release_failed","recoveryRetained":True})
            self.assertEqual(writes, [("native-exit-receipt.json", primary), ("native-cleanup-receipt.json", actual)])
            module_os.close(recovery["leafFD"])
            module_os.close(recovery["directoryFD"])


if __name__ == "__main__":
    unittest.main()
