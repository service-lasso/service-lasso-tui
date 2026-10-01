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
if (!root || !executable || !sourceCommit) throw new Error("root, executable, and source commit are required");
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
const helperArgument = arg("--helper");
const helper = helperArgument ? path.resolve(helperArgument) : path.join(path.dirname(fileURLToPath(import.meta.url)), "tui20-native-five-action.py");
const child = spawn(python, [helper, "--root", root, "--executable", executable, "--source-commit", sourceCommit, "--core-commit", coreCommit], { stdio: "inherit", env: { ...process.env, SERVICE_LASSO_TUI20_TOKEN: token, SERVICE_LASSO_TUI20_DENIED_TOKEN: deniedToken, SERVICE_LASSO_TUI20_API_URL: server.url } });
const [code, signal] = await once(child, "exit");
await writeFile(path.join(root, "core-parent-exit.json"), JSON.stringify({ childExit: Number.isInteger(code) ? code : null, childSignal: signal ?? null, coreStopRequested: true }), { encoding: "utf8", mode: 0o600 });
await server.stop(); await new Promise((resolve) => jwks.close(resolve)); process.exitCode = code ?? 1;
