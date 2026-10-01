import assert from "node:assert/strict";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
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
  assert.match(owner, /OwnedRuntimeAcquisitionFailure/);
  assert.doesNotMatch(owner, /runtime\.kill\(\)/);
  assert.match(controller, /owner-birth\.json/);
  assert.match(controller, /owner-close\.json/);
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
