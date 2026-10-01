import { createServer } from "node:http";
import { once } from "node:events";
import { pathToFileURL } from "node:url";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";

const root = process.argv[2];
if (!root) throw new Error("evidence root is required");
const core = path.join(root, "core-source");
const { exportJWK, generateKeyPair, SignJWT } = await import(pathToFileURL(path.join(core, "node_modules", "jose", "dist", "webapi", "index.js")).href);
const { startApiServer } = await import(pathToFileURL(path.join(core, "dist", "server", "index.js")).href);
const { writeExecutableFixtureService } = await import(pathToFileURL(path.join(core, "tests", "test-helpers.js")).href);

const servicesRoot = path.join(root, "services");
const workspaceRoot = path.join(root, "workspace");
const tokenPath = path.join(root, "private-token.json");
const connectionsPath = path.join(root, "connections.json");
const stopPath = path.join(root, "stop-core");
process.env.SERVICE_LASSO_WORKSPACE_ROOT = workspaceRoot;
process.env.SERVICE_LASSO_INSTANCE_REGISTRY_PATH = path.join(root, "registry", "instances.json");
process.env.SERVICE_LASSO_HOST_PORT_REGISTRY_PATH = path.join(root, "registry", "ports.json");
await mkdir(servicesRoot, { recursive: true });
await mkdir(workspaceRoot, { recursive: true });
await writeExecutableFixtureService(servicesRoot, "tui20-fixture", { autoExitMs: null });
await writeExecutableFixtureService(servicesRoot, "tui20-unrelated", { autoExitMs: null });

const issuer = "https://tui20-native-fixture.invalid";
const audience = "tui20-native-fixture";
const keyId = "tui20-native-fixture";
const { privateKey, publicKey } = await generateKeyPair("RS256");
const jwk = await exportJWK(publicKey);
Object.assign(jwk, { kid: keyId, alg: "RS256", use: "sig" });
const jwks = createServer((request, response) => {
  if (request.url !== "/jwks") { response.statusCode = 404; response.end(); return; }
  response.setHeader("content-type", "application/json");
  response.end(JSON.stringify({ keys: [jwk] }));
});
jwks.listen(0, "127.0.0.1");
await once(jwks, "listening");
const address = jwks.address();
if (!address || typeof address === "string") throw new Error("JWKS listener unavailable");
const now = Math.floor(Date.now() / 1000);
const token = await new SignJWT({ client_id: "tui20-native-client", scope: "service-lasso:read service-lasso:lifecycle:write service-lasso:config:write service-lasso:update:write" })
  .setProtectedHeader({ alg: "RS256", kid: keyId }).setIssuer(issuer).setAudience(audience)
  .setSubject("tui20-native-operator").setIssuedAt(now).setExpirationTime(now + 900).sign(privateKey);
const deniedToken = await new SignJWT({ client_id: "tui20-native-denied-client", scope: "service-lasso:read" })
  .setProtectedHeader({ alg: "RS256", kid: keyId }).setIssuer(issuer).setAudience(audience)
  .setSubject("tui20-native-denied-operator").setIssuedAt(now).setExpirationTime(now + 900).sign(privateKey);
const server = await startApiServer({
  port: 0,
  servicesRoot,
  workspaceRoot,
  mcpHttpIdentity: { env: {
    SERVICE_LASSO_MCP_MODE: "guarded",
    SERVICE_LASSO_MCP_OAUTH_ISSUER: issuer,
    SERVICE_LASSO_MCP_OAUTH_JWKS_URI: `http://127.0.0.1:${address.port}/jwks`,
    SERVICE_LASSO_MCP_RESOURCE_URI: "https://tui20-native-fixture.invalid/api/mcp",
    SERVICE_LASSO_MCP_OAUTH_AUDIENCE: audience,
  } },
});
await writeFile(tokenPath, JSON.stringify({ token, deniedToken }), { encoding: "utf8", mode: 0o600 });
await writeFile(connectionsPath, JSON.stringify({ defaultProfile: "native", profiles: { native: {
  tokenEnv: "SERVICE_LASSO_API_TOKEN", url: server.url, authMode: "oauth-bearer",
  scopes: ["service-lasso:read", "service-lasso:lifecycle:write"],
}, denied: {
  tokenEnv: "SERVICE_LASSO_DENIED_TOKEN", url: server.url, authMode: "oauth-bearer",
  scopes: ["service-lasso:read", "service-lasso:lifecycle:write"],
}, invalid: {
  tokenEnv: "SERVICE_LASSO_INVALID_TOKEN", url: server.url, authMode: "oauth-bearer",
  scopes: ["service-lasso:read", "service-lasso:lifecycle:write"],
}, missing: {
  tokenEnv: "SERVICE_LASSO_MISSING_TOKEN", url: server.url, authMode: "oauth-bearer",
  scopes: ["service-lasso:read", "service-lasso:lifecycle:write"],
} } }), { encoding: "utf8", mode: 0o600 });
await writeFile(path.join(root, "ready.json"), JSON.stringify({ coreCommit: "93d9d343a058d296069c017d17f4f8d1fc1505ea", services: ["tui20-fixture", "tui20-unrelated"] }), { encoding: "utf8", mode: 0o600 });

while (true) {
  try { await readFile(stopPath); break; } catch { await new Promise((resolve) => setTimeout(resolve, 250)); }
}
await server.stop();
await new Promise((resolve) => jwks.close(resolve));
