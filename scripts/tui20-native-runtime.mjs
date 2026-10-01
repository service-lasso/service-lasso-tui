// Real Core/JWKS process. It is a child of the external POSIX owner, never
// of the volatile acceptance controller.
import { createServer } from "node:http";
import { once } from "node:events";
import { pathToFileURL } from "node:url";
import { mkdir, readFile, stat, writeFile } from "node:fs/promises";
import path from "node:path";
const arg = name => { const i = process.argv.indexOf(name); return i < 0 ? undefined : process.argv[i + 1]; };
const root = arg("--root"), coreCommit = arg("--core-commit"), invalidReadyReceipt = arg("--invalid-ready-receipt") === "true" || process.argv.includes("--invalid-ready-receipt"), readyMode = arg("--ready-mode") ?? "normal", shutdownPipeFailure = process.argv.includes("--shutdown-pipe-failure");
if (!root || !coreCommit) throw new Error("runtime root and Core commit are required");
const core = path.join(root, "core-source"), servicesRoot = path.join(root, "services"), workspaceRoot = path.join(root, "workspace"), instanceRegistryPath = path.join(root, "registry", "instances.json"), hostPortRegistryPath = path.join(root, "registry", "ports.json");
// Core evaluates configuration during module loading. Establish all three
// phase-owned paths before importing any Core or Core dependency module.
const ownedRuntimeEnvironment = Object.freeze({ SERVICE_LASSO_WORKSPACE_ROOT: workspaceRoot, SERVICE_LASSO_INSTANCE_REGISTRY_PATH: instanceRegistryPath, SERVICE_LASSO_HOST_PORT_REGISTRY_PATH: hostPortRegistryPath });
Object.assign(process.env, ownedRuntimeEnvironment);
if (Object.entries(ownedRuntimeEnvironment).some(([key,value]) => process.env[key] !== value)) throw new Error("owned Core runtime environment unavailable before import");
const { exportJWK, generateKeyPair, SignJWT } = await import(pathToFileURL(path.join(core, "node_modules", "jose", "dist", "webapi", "index.js")).href);
const { startApiServer } = await import(pathToFileURL(path.join(core, "dist", "server", "index.js")).href);
const { writeExecutableFixtureService } = await import(pathToFileURL(path.join(core, "tests", "test-helpers.js")).href);
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
const [instanceRegistry, portRegistry, runtimeInstance] = await Promise.all([readFile(instanceRegistryPath, "utf8"), readFile(hostPortRegistryPath, "utf8"), readFile(path.join(workspaceRoot, ".service-lasso", "runtime-instance.json"), "utf8")]);
if (!JSON.parse(runtimeInstance).instance?.instanceId || !JSON.parse(instanceRegistry).instances?.length || !JSON.parse(portRegistry).allocations?.length) throw new Error("Core runtime-path readback unavailable"); await Promise.all([stat(workspaceRoot), stat(instanceRegistryPath), stat(hostPortRegistryPath)]);
await writeFile(path.join(root, "ready.json"), JSON.stringify({ coreCommit, runtimePathReceipt: invalidReadyReceipt ? { coreReadback: false } : { coreReadback: true, uniqueOwnedPaths: ["workspaceRoot", "instanceRegistryPath", "hostPortRegistryPath"], runtimeInstanceBound: true } }), { encoding: "utf8", mode: 0o600 });
if (readyMode === "normal") process.stdout.write(JSON.stringify({ event: "ready", url: server.url, token, deniedToken, jwksPort: address.port }) + "\n");
if (readyMode === "eof") process.stdout.end();
if (shutdownPipeFailure) {
  // This remains the actual Core/JWKS runtime.  Its control pipe is closed and
  // it exits naturally after its own bounded teardown, so the owner must not
  // substitute a signal or an unobserved exit claim.
  process.stdin.destroy();
  setTimeout(async () => { await server.stop(); await new Promise(resolve => jwks.close(resolve)); process.exit(0); }, 250).unref();
}
process.stdin.setEncoding("utf8"); await new Promise(resolve => process.stdin.once("data", resolve));
await server.stop(); await new Promise(resolve => jwks.close(resolve));
