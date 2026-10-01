// The OAuth credentials stay in this parent process and are passed only in
// child memory.  No credential file is created in the retained evidence root.
import { createServer } from "node:http";
import { once } from "node:events";
import { fileURLToPath, pathToFileURL } from "node:url";
import { mkdir, readFile, stat, writeFile } from "node:fs/promises";
import { spawn } from "node:child_process";
import path from "node:path";
const arg = (name) => { const i = process.argv.indexOf(name); return i < 0 ? undefined : process.argv[i + 1]; };
const root = arg("--root") ?? process.argv[2], executable = arg("--executable"), sourceCommit = arg("--source-commit"), coreCommit = arg("--core-commit") ?? "2633c07be25512d0a84f9bfa28de6be5edff35e8", python = arg("--python") ?? "python";
if (!root) throw new Error("root is required");
const helperArgument = arg("--helper");
const helper = helperArgument ? path.resolve(helperArgument) : path.join(path.dirname(fileURLToPath(import.meta.url)), "tui20-native-five-action.py");
if (arg("--recovery-self-test")) {
  await mkdir(root, { recursive: true });
  const listener = (name) => createServer((req, res) => {
    if (req.url !== `/${name}`) { res.statusCode = 404; res.end(); return; }
    res.setHeader("content-type", "application/json"); res.end(JSON.stringify({ owner: "tui20-external-recovery-parent", dependency: name }));
  });
  const startListener = async (name) => {
    const server = listener(name); server.listen(0, "127.0.0.1"); await once(server, "listening");
    const address = server.address(); if (!address || typeof address === "string") throw new Error(`${name} listener unavailable`);
    return { name, server, url: `http://127.0.0.1:${address.port}/${name}`, port: address.port };
  };
  const [coreDependency, jwksDependency] = await Promise.all([startListener("core"), startListener("jwks")]);
  const child = spawn(python, [helper, "--root", root, "--recovery-owner-self-test", "--core-url", coreDependency.url, "--jwks-url", jwksDependency.url], { stdio: "inherit" });
  const [code, signal] = await once(child, "exit");
  const ownerProof = JSON.parse(await readFile(path.join(root, "recovery-owner-proof.json"), "utf8"));
  if (!ownerProof.helperCrashObserved || !ownerProof.childLiveAfterHelperCrash || !ownerProof.immutableExecutionHeld || !ownerProof.dependenciesLiveAfterHelperCrash || ownerProof.ownedExitObserved !== "terminal_exited_zero") throw new Error("external recovery owner proof is incomplete");
  await new Promise((resolve) => coreDependency.server.close(resolve)); await new Promise((resolve) => jwksDependency.server.close(resolve));
  await writeFile(path.join(root, "recovery-parent-proof.json"), JSON.stringify({ recoveryOwnerExitObserved: true, recoveryOwnerExit: Number.isInteger(code) ? code : null, recoveryOwnerSignal: signal ?? null, corePort: coreDependency.port, jwksPort: jwksDependency.port, coreStopAfterOwnerExit: true, jwksStopAfterOwnerExit: true }), { encoding: "utf8", mode: 0o600 });
  process.exitCode = code ?? 1;
} else {
if (!executable || !sourceCommit) throw new Error("executable and source commit are required");
const core = path.join(root, "core-source"), servicesRoot = path.join(root, "services"), workspaceRoot = path.join(root, "workspace"), instanceRegistryPath = path.join(root, "registry", "instances.json"), hostPortRegistryPath = path.join(root, "registry", "ports.json");
const { exportJWK, generateKeyPair, SignJWT } = await import(pathToFileURL(path.join(core, "node_modules", "jose", "dist", "webapi", "index.js")).href);
const { startApiServer } = await import(pathToFileURL(path.join(core, "dist", "server", "index.js")).href);
const { writeExecutableFixtureService } = await import(pathToFileURL(path.join(core, "tests", "test-helpers.js")).href);
process.env.SERVICE_LASSO_WORKSPACE_ROOT = workspaceRoot;
process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH = instanceRegistryPath;
process.env.SERVICE_LASSO_HOST_PORT_REGISTRY_PATH = hostPortRegistryPath;
await mkdir(servicesRoot, { recursive: true }); await mkdir(workspaceRoot, { recursive: true });
await writeExecutableFixtureService(servicesRoot, "tui20-fixture", { autoExitMs: null }); await writeExecutableFixtureService(servicesRoot, "tui20-unrelated", { autoExitMs: null });
const issuer = "https://tui20-native-fixture.invalid", audience = "tui20-native-fixture", keyId = "tui20-native-fixture";
const { privateKey, publicKey } = await generateKeyPair("RS256"); const jwk = await exportJWK(publicKey); Object.assign(jwk, { kid: keyId, alg: "RS256", use: "sig" });
const jwks = createServer((req, res) => { if (req.url !== "/jwks") { res.statusCode = 404; res.end(); return; } res.setHeader("content-type", "application/json"); res.end(JSON.stringify({ keys: [jwk] })); }); jwks.listen(0, "127.0.0.1"); await once(jwks, "listening");
const address = jwks.address(); if (!address || typeof address === "string") throw new Error("JWKS listener unavailable"); const now = Math.floor(Date.now() / 1000);
const sign = (client, scope) => new SignJWT({ client_id: client, scope }).setProtectedHeader({ alg: "RS256", kid: keyId }).setIssuer(issuer).setAudience(audience).setSubject("tui20-native-operator").setIssuedAt(now).setExpirationTime(now + 900).sign(privateKey);
const [token, deniedToken] = await Promise.all([sign("tui20-native-client", "service-lasso:read service-lasso:lifecycle:write service-lasso:config:write service-lasso:update:write"), sign("tui20-native-denied-client", "service-lasso:read")]);
const server = await startApiServer({ port: 0, servicesRoot, workspaceRoot, mcpHttpIdentity: { env: { SERVICE_LASSO_MCP_MODE: "guarded", SERVICE_LASSO_MCP_OAUTH_ISSUER: issuer, SERVICE_LASSO_MCP_OAUTH_JWKS_URI: `http://127.0.0.1:${address.port}/jwks`, SERVICE_LASSO_MCP_RESOURCE_URI: "https://tui20-native-fixture.invalid/api/mcp", SERVICE_LASSO_MCP_OAUTH_AUDIENCE: audience } } });
const profiles = { defaultProfile: "native", profiles: {} }; for (const [name, tokenEnv] of [["native", "SERVICE_LASSO_API_TOKEN"], ["denied", "SERVICE_LASSO_DENIED_TOKEN"], ["invalid", "SERVICE_LASSO_INVALID_TOKEN"], ["missing", "SERVICE_LASSO_MISSING_TOKEN"]]) profiles.profiles[name] = { tokenEnv, url: server.url, authMode: "oauth-bearer", scopes: ["service-lasso:read", "service-lasso:lifecycle:write"] };
await writeFile(path.join(root, "connections.json"), JSON.stringify(profiles), { encoding: "utf8", mode: 0o600 });
// Read Core-owned materialization after startup.  This is deliberately a
// readback, rather than a declaration made by the harness before Core runs.
const corePathReadback = async () => {
  const [instanceRegistry, portRegistry, runtimeInstance] = await Promise.all([
    readFile(instanceRegistryPath, "utf8"), readFile(hostPortRegistryPath, "utf8"),
    readFile(path.join(workspaceRoot, ".service-lasso", "runtime-instance.json"), "utf8"),
  ]);
  const instance = JSON.parse(runtimeInstance).instance;
  if (!instance?.instanceId || !JSON.parse(instanceRegistry).instances?.length || !JSON.parse(portRegistry).allocations?.length) throw new Error("Core runtime-path readback unavailable");
  await Promise.all([stat(workspaceRoot), stat(instanceRegistryPath), stat(hostPortRegistryPath)]);
  return { coreReadback: true, uniqueOwnedPaths: ["workspaceRoot", "instanceRegistryPath", "hostPortRegistryPath"], runtimeInstanceBound: true };
};
const runtimePathReceipt = await corePathReadback();
await writeFile(path.join(root, "ready.json"), JSON.stringify({ coreCommit, runtimePathReceipt }), { encoding: "utf8", mode: 0o600 });
const recoveryOwner = spawn(python, [helper, "--root", root, "--executable", executable, "--source-commit", sourceCommit, "--core-commit", coreCommit], { stdio: "inherit", env: { ...process.env, SERVICE_LASSO_TUI20_TOKEN: token, SERVICE_LASSO_TUI20_DENIED_TOKEN: deniedToken, SERVICE_LASSO_TUI20_API_URL: server.url } });
const [code, signal] = await once(recoveryOwner, "exit");
await writeFile(path.join(root, "core-parent-exit.json"), JSON.stringify({ recoveryOwnerExit: Number.isInteger(code) ? code : null, recoveryOwnerSignal: signal ?? null, coreStopAfterRecoveryOwnerExit: true }), { encoding: "utf8", mode: 0o600 });
await server.stop(); await new Promise((resolve) => jwks.close(resolve)); process.exitCode = code ?? 1;
}
