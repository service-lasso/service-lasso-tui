import assert from "node:assert/strict";
import { chmod, cp, mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import { spawn } from "node:child_process";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import test from "node:test";

const scripts = path.dirname(fileURLToPath(import.meta.url));

test("native owner architecture is shared by Linux and Darwin and requires real adverse proof", async () => {
  const [controller, owner, runtime] = await Promise.all(["tui20-native-core.mjs", "tui20-native-posix-five-action.py", "tui20-native-runtime.mjs"].map(name => readFile(path.join(scripts, name), "utf8")));
  assert.match(controller, /--adverse-controller-crash/);
  assert.match(controller, /process\.kill\(process\.pid, "SIGKILL"\)/);
  assert.match(owner, /start_owned_runtime/);
  assert.match(owner, /TUI child did not survive actual controller failure/);
  assert.match(owner, /coreAndJwksLiveAfterFailure/);
  assert.match(owner, /naturalChildExit/);
  assert.match(owner, /Every post-start preflight stays inside this owner boundary/);
  assert.match(owner, /owner-preflight-cleanup\.json/);
  assert.match(owner, /owner-finalization-failure-proof\.json/);
  assert.match(owner, /external-owner-finalization\.json/);
  assert.match(owner, /select\.select\(\[runtime\.stdout\],\[\],\[\],10\)/);
  assert.match(owner, /owner-runtime-recovery\.json/);
  assert.match(owner, /OwnedRuntimeAcquisitionFailure/);
  assert.doesNotMatch(owner, /runtime\.kill\(\)/);
  assert.match(controller, /tui20-native-owner-observer\.py/);
  assert.doesNotMatch(controller, /writeFile\(path\.join\(root, "owner-birth/);
  assert.match(runtime, /shutdownPipeFailure/);
  assert.match(runtime, /invalidReadyReceipt/);
  assert.match(runtime, /ownedRuntimeEnvironment/);
  assert.ok(runtime.indexOf("Object.assign(process.env, ownedRuntimeEnvironment)") < runtime.indexOf("const { exportJWK, generateKeyPair, SignJWT }"));
  assert.match(owner, /sys_platform\(\)=="linux"/);
  assert.match(owner, /sys_platform\(\)=="darwin"/);
  assert.match(runtime, /startApiServer/);
  assert.match(runtime, /await server\.stop\(\)/);
});

test("runtime establishes each owned Core path before the first Core import", async () => {
  const root = await mkdtemp(path.join(tmpdir(), "tui20-runtime-order-"));
  const core = path.join(root, "core-source");
  const write = async (relative, contents) => {
    const target = path.join(core, relative);
    await mkdir(path.dirname(target), { recursive: true });
    await writeFile(target, contents, "utf8");
  };
  try {
    await write("node_modules/jose/package.json", '{"type":"module"}');
    await write("node_modules/jose/dist/webapi/index.js", `
      import { writeFile } from "node:fs/promises";
      await writeFile(process.env.TUI20_OBSERVED_ENV_PATH, JSON.stringify({ workspace: process.env.SERVICE_LASSO_WORKSPACE_ROOT, instances: process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH, ports: process.env.SERVICE_LASSO_HOST_PORT_REGISTRY_PATH }));
      export const generateKeyPair = async () => ({ privateKey: {}, publicKey: {} });
      export const exportJWK = async () => ({});
      export class SignJWT { setProtectedHeader(){ return this; } setIssuer(){ return this; } setAudience(){ return this; } setSubject(){ return this; } setIssuedAt(){ return this; } setExpirationTime(){ return this; } async sign(){ return "fixture"; } }
    `);
    await write("dist/server/index.js", `
      import { mkdir, writeFile } from "node:fs/promises";
      import path from "node:path";
      export const startApiServer = async ({ workspaceRoot }) => {
        await mkdir(process.env.SERVICE_LASSO_WORKSPACE_ROOT + "/.service-lasso", { recursive: true });
        await mkdir(path.dirname(process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH), { recursive: true });
        await writeFile(process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH, JSON.stringify({ instances:[{}] }));
        await writeFile(process.env.SERVICE_LASSO_HOST_PORT_REGISTRY_PATH, JSON.stringify({ allocations:[{}] }));
        await writeFile(workspaceRoot + "/.service-lasso/runtime-instance.json", JSON.stringify({ instance:{ instanceId:"fixture" } }));
        return { url:"http://127.0.0.1:1", stop: async () => {} };
      };
    `);
    await write("tests/test-helpers.js", 'export const writeExecutableFixtureService = async () => {};');
    const observed = path.join(root, "observed.json");
    const child = spawn(process.execPath, [path.join(scripts, "tui20-native-runtime.mjs"), "--root", root, "--core-commit", "a".repeat(40)], { env: { ...process.env, TUI20_OBSERVED_ENV_PATH: observed, SERVICE_LASSO_WORKSPACE_ROOT: "wrong-workspace", SERVICE_LASSO_INSTANCE_REGISTRY_PATH: "wrong-instances", SERVICE_LASSO_HOST_PORT_REGISTRY_PATH: "wrong-ports" }, stdio: ["pipe", "pipe", "pipe"] });
    let stdout = "", stderr = "";
    child.stdout.on("data", value => { stdout += value; }); child.stderr.on("data", value => { stderr += value; });
    await new Promise((resolve, reject) => { const timeout = setTimeout(() => reject(new Error(`runtime did not become ready; ${stderr}`)), 3000); child.stdout.once("data", () => { clearTimeout(timeout); resolve(); }); child.once("error", reject); child.once("exit", code => { clearTimeout(timeout); reject(new Error(`runtime exited before ready: ${code}; ${stderr}`)); }); });
    await new Promise(resolve => setTimeout(resolve, 20));
    child.stdin.end("close\n");
    const [code] = await new Promise((resolve, reject) => { const timeout = setTimeout(() => { child.kill(); reject(new Error(`runtime did not close; ${stderr}`)); }, 3000); child.once("exit", (...value) => { clearTimeout(timeout); resolve(value); }); });
    assert.equal(code, 0, stderr);
    assert.match(stdout, /"event":"ready"/);
    assert.deepEqual(JSON.parse(await readFile(observed, "utf8")), { workspace: path.join(root, "workspace"), instances: path.join(root, "registry", "instances.json"), ports: path.join(root, "registry", "ports.json") });
  } finally { await rm(root, { recursive: true, force: true }); }
});

test("actual controller observer external-owner finalization retains failure and directly observes runtime closure", { skip: !["linux", "darwin"].includes(process.platform) }, async t => {
  const root = await mkdtemp(path.join(tmpdir(), "tui20-external-finalization-"));
  t.after(() => rm(root, { recursive: true, force: true }));
  const put = async (relative, content) => {
    const target = path.join(root, "core-source", relative);
    await mkdir(path.dirname(target), { recursive: true });
    await writeFile(target, content);
  };
  // Core imports are explicit fixtures. Controller, durable observer, runtime
  // child and external owner are the actual production entry points. Preflight
  // fails before candidate activation; this does not claim native acceptance.
  await put("package.json", '{"type":"module"}');
  await put("node_modules/jose/package.json", '{"type":"module"}');
  await put("node_modules/jose/dist/webapi/index.js", `
    export const generateKeyPair = async () => ({ privateKey:{}, publicKey:{} });
    export const exportJWK = async () => ({});
    export class SignJWT { setProtectedHeader(){return this;} setIssuer(){return this;} setAudience(){return this;} setSubject(){return this;} setIssuedAt(){return this;} setExpirationTime(){return this;} async sign(){return "fixture";} }
  `);
  await put("dist/server/index.js", `
    import { mkdir, writeFile } from "node:fs/promises";
    import path from "node:path";
    export const startApiServer = async ({workspaceRoot}) => {
      await mkdir(workspaceRoot+"/.service-lasso",{recursive:true});
      await mkdir(path.dirname(process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH),{recursive:true});
      await writeFile(process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH,JSON.stringify({instances:[{}]}));
      await writeFile(process.env.SERVICE_LASSO_HOST_PORT_REGISTRY_PATH,JSON.stringify({allocations:[{}]}));
      await writeFile(workspaceRoot+"/.service-lasso/runtime-instance.json",JSON.stringify({instance:{instanceId:"fixture"}}));
      return {url:"http://127.0.0.1:1",stop:async()=>{await writeFile(workspaceRoot+"/actual-stop.json","closed");}};
    };
  `);
  await put("tests/test-helpers.js", 'export const writeExecutableFixtureService = async () => {};');
  const binary = path.join(root, "candidate-fixture");
  await writeFile(binary, "preflight never activates this fixture executable\n");
  const child = spawn(process.execPath, [path.join(scripts,"tui20-native-core.mjs"),"--root",root,"--executable",binary,"--source-commit","a".repeat(40),"--core-commit","b".repeat(40),"--python","python3","--invalid-ready-receipt","true","--inject-finalization-cleanup-failure","true"], {stdio:["ignore","pipe","pipe"]});
  let stderr=""; child.stderr.on("data",bytes=>{stderr+=bytes;}); child.stdout.resume();
  const [code,signal] = await new Promise((resolve,reject)=>{child.once("exit",(...values)=>resolve(values));child.once("error",reject);});
  assert.equal(signal,null);
  assert.equal(code,1,"preflight failure must remain failed even after observed cleanup");
  const read = async name => JSON.parse(await readFile(path.join(root,name),"utf8"));
  const outcome=await read("external-owner-finalization.json"), proof=await read("owner-finalization-failure-proof.json"), closed=await read("owner-private-closed.json"), initial=await read("owner-private-initial.json"), witness=await read("owner-private-witness.json"), projection=await read("native-public-projection.json");
  assert.equal(outcome.primaryOutcome,"failed");
  assert.equal(outcome.finalizationFailure,"injected_cleanup_failure");
  assert.equal(proof.finalizationFailure,outcome.finalizationFailure);
  assert.equal(proof.ownerPID,witness.recoveryOwnerPID);
  assert.equal(proof.ownerBirth.platform,process.platform);
  assert.equal(proof.runtimePID,initial.runtimePID);
  assert.deepEqual(proof.runtimeBirth,initial.runtimeBirth);
  assert.equal(proof.observerPID,initial.observerPID);
  assert.equal(proof.ownerDrivenRuntimeShutdown,true);
  assert.equal(proof.actualExitObserved,true);
  assert.equal(proof.runtimeChildExit,"terminal_exited_zero");
  assert.equal(proof.runtimeChildStillLiveAfterClose,false);
  assert.equal(proof.retainedOwnedRuntime,false);
  assert.equal(closed.reason,"owner_finalization_failed");
  assert.equal(closed.runtimeStdoutClosed,true);
  assert.equal(closed.runtimeStderrClosed,true);
  assert.equal(projection.result,"failed");
  assert.equal(projection.actionsPassed,false);
  assert.equal(projection.ownedRuntimeClosed,true);
  assert.equal(await readFile(path.join(root,"workspace/actual-stop.json"),"utf8"),"closed");
  for (const pid of [proof.ownerPID,proof.runtimePID,proof.observerPID]) assert.throws(()=>process.kill(pid,0),{code:"ESRCH"},"actual production children must be absent after parent wait");
  // Negative owner fixtures exercise the actual controller/observer/runtime
  // edge. They never substitute for the actual-owner positive path above.
  for (const [name,body,consume=true] of [["missing","sys.exit(0)"],["malformed","json.dump({},open(os.path.join(root,'external-owner-finalization.json'),'w')); sys.exit(0)"],["crashed","os.kill(os.getpid(),signal.SIGKILL)"],["exit-before-read","os.close(0); sys.exit(0)",false],["crash-before-read","os.close(0); os.kill(os.getpid(),signal.SIGKILL)",false],["witness-write-failure","sys.stdin.readline(); sys.exit(0)",false]]) {
    const phase=path.join(root,name);
    await mkdir(phase);
    await cp(path.join(root,"core-source"),path.join(phase,"core-source"),{recursive:true});
    const helper=path.join(phase,"negative-owner.py");
    await writeFile(helper,`import json,os,signal,sys\nroot=sys.argv[sys.argv.index('--root')+1]\n${consume ? "sys.stdin.readline()" : ""}\n${body}\n`);
    if(name==="witness-write-failure") await mkdir(path.join(phase,"owner-private-witness.json"));
    const failed=spawn(process.execPath,[path.join(scripts,"tui20-native-core.mjs"),"--root",phase,"--executable",binary,"--source-commit","a".repeat(40),"--core-commit","b".repeat(40),"--python","python3","--helper",helper],{stdio:["ignore","pipe","pipe"]});
    failed.stdout.resume(); failed.stderr.resume();
    const [failureCode,failureSignal]=await new Promise((resolve,reject)=>{failed.once("exit",(...values)=>resolve(values));failed.once("error",reject);});
    assert.equal(failureSignal,null);
    assert.equal(failureCode,1,name);
    const failureProjection=JSON.parse(await readFile(path.join(phase,"native-public-projection.json"),"utf8"));
    const failureClosure=JSON.parse(await readFile(path.join(phase,"owner-private-closed.json"),"utf8"));
    assert.notEqual(failureProjection.result,"succeeded",name);
    assert.equal(failureProjection.actionsPassed,false,name);
    assert.equal(failureClosure.actualExitObserved,true,name);
    assert.equal(failureClosure.runtimeChildStillLiveAfterClose,false,name);
    assert.equal(failureClosure.runtimeBirthAbsentObserved,true,name);
    const actualOwner=JSON.parse(await readFile(path.join(phase,"owner-private-owner-closed.json"),"utf8"));
    assert.equal(actualOwner.actualWaitObserved,true,name);
    assert.ok(actualOwner.recoveryOwnerActualExit!==null || actualOwner.recoveryOwnerActualSignal!==null,name);
    assert.equal(await readFile(path.join(phase,"workspace/actual-stop.json"),"utf8"),"closed",name);
    for(const pid of [actualOwner.recoveryOwnerPID,failureClosure.runtimePID]) assert.throws(()=>process.kill(pid,0),{code:"ESRCH"},name);
    await assert.rejects(()=>readFile(path.join(phase,"owner-finalization-failure-proof.json")),{code:"ENOENT"},name);
  }
  // Fault instrumentation affects only the named OS boundary. It executes
  // unchanged observer main() and real Popen children via the real controller;
  // waits, runtime EOF teardown and receipts are never substituted.
  for(const name of ["owner-launch-error","runtime-birth-error","owner-birth-error","handoff-write-error"]){
    const phase=path.join(root,name); await mkdir(phase);
    await cp(path.join(root,"core-source"),path.join(phase,"core-source"),{recursive:true});
    const runner=path.join(phase,"observer-boundary.py"), launcher=path.join(phase,"python-launcher.sh");
    await writeFile(runner,`import runpy,sys,subprocess,json,os\nsource=sys.argv.pop(1)\nnamespace=runpy.run_path(source)\nglobals=namespace['main'].__globals__\noriginal_birth=globals['process_birth']\noriginal_popen=subprocess.Popen\nbirth_count=0\nroot=sys.argv[sys.argv.index('--root')+1]\ndef reached(boundary):\n    with open(os.path.join(root,'fault-boundary.json'),'w') as stream: json.dump({'boundary':boundary},stream)\ndef birth(pid):\n    global birth_count\n    birth_count+=1\n    if ('${name}'=='runtime-birth-error' and birth_count==1) or ('${name}'=='owner-birth-error' and birth_count==3):\n        reached('runtime-birth' if birth_count==1 else 'owner-birth')\n        raise OSError('controlled birth observation error')\n    return original_birth(pid)\ndef launch(*args,**kwargs):\n    command=args[0] if args else kwargs.get('args',[])\n    is_owner=isinstance(command,(list,tuple)) and '--external-runtime' in command and len(command)>1 and command[1]==sys.argv[sys.argv.index('--owner')+1]\n    if '${name}'=='owner-launch-error' and is_owner:\n        reached('owner-launch')\n        raise OSError('controlled owner launch error')\n    child=original_popen(*args,**kwargs)\n    if '${name}'=='handoff-write-error' and is_owner:\n        original=child.stdin\n        class FailedWrite:\n            def write(self,*args):\n                reached('owner-handoff-write')\n                raise BrokenPipeError('controlled handoff write error')\n            def close(self): return original.close()\n        child.stdin=FailedWrite()\n    return child\nglobals['process_birth']=birth\nsubprocess.Popen=launch\nnamespace['main']()\n`);
    await writeFile(launcher,`#!/bin/sh\nexec python3 '${runner.replaceAll("'","'\\''")}' "$@"\n`); await chmod(launcher,0o700);
    const failed=spawn(process.execPath,[path.join(scripts,"tui20-native-core.mjs"),"--root",phase,"--executable",binary,"--source-commit","a".repeat(40),"--core-commit","b".repeat(40),"--python",launcher,"--invalid-ready-receipt","true"],{stdio:["ignore","pipe","pipe"]});
    failed.stdout.resume();failed.stderr.resume();
    const [code,signal]=await new Promise((resolve,reject)=>{failed.once("exit",(...values)=>resolve(values));failed.once("error",reject);});
    assert.equal(signal,null);assert.equal(code,1,name);
    const boundary=JSON.parse(await readFile(path.join(phase,"fault-boundary.json"),"utf8"));
    assert.deepEqual(boundary,{boundary:{"owner-launch-error":"owner-launch","runtime-birth-error":"runtime-birth","owner-birth-error":"owner-birth","handoff-write-error":"owner-handoff-write"}[name]},name);
    const closure=JSON.parse(await readFile(path.join(phase,"owner-private-closed.json"),"utf8"));
    const projection=JSON.parse(await readFile(path.join(phase,"native-public-projection.json"),"utf8"));
    assert.equal(closure.actualExitObserved,true,name);
    assert.equal(closure.runtimeChildExit,"terminal_exited_zero",name);
    assert.equal(closure.runtimeBirthAbsentObserved,name!=="runtime-birth-error",name);
    assert.equal(projection.result,"failed",name);assert.equal(projection.actionsPassed,false,name);
    assert.equal(projection.ownedRuntimeClosed,name!=="runtime-birth-error",name);
    assert.throws(()=>process.kill(closure.runtimePID,0),{code:"ESRCH"},name);
    assert.equal(await readFile(path.join(phase,"workspace/actual-stop.json"),"utf8"),"closed",name);
    if(name!=="owner-launch-error" && name!=="runtime-birth-error"){
      const ownerClosure=JSON.parse(await readFile(path.join(phase,"owner-private-owner-closed.json"),"utf8"));
      assert.equal(ownerClosure.actualWaitObserved,true,name);
      assert.equal(ownerClosure.ownerBirthAbsentObserved,name!=="owner-birth-error",name);
      assert.throws(()=>process.kill(ownerClosure.recoveryOwnerPID,0),{code:"ESRCH"},name);
    }
  }
});
