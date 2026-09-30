import { execFile } from "node:child_process";
import { mkdir, mkdtemp, rm, stat } from "node:fs/promises";
import { createServer } from "node:net";
import os from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { promisify } from "node:util";

const execFileAsync = promisify(execFile);
const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const pinnedCoreDevelop = "d9e2ae799244317940c862fe1261dfd22b7bdda1";

function parseArgs(argv) {
  const marker = argv.indexOf("--core-root");
  if (marker < 0 || !argv[marker + 1] || argv.length !== 2) {
    throw new Error("usage");
  }
  return { coreRoot: path.resolve(argv[marker + 1]) };
}

async function run(command, args, options = {}) {
  return execFileAsync(command, args, { cwd: options.cwd, windowsHide: true, timeout: options.timeout ?? 120000 });
}

async function reserveUnavailableLoopbackURL() {
  const listener = createServer();
  await new Promise((resolve, reject) => {
    listener.once("error", reject);
    listener.listen({ host: "127.0.0.1", port: 0 }, resolve);
  });
  const address = listener.address();
  await new Promise((resolve, reject) => listener.close(error => error ? reject(error) : resolve()));
  if (!address || typeof address === "string") {
    throw new Error("unavailable loopback port was not allocated");
  }
  return `http://127.0.0.1:${address.port}`;
}

function parseProbe(stdout, mode) {
  try {
    const result = JSON.parse(stdout.trim());
    if (result?.ok === true && result.mode === mode && result.exit === "q") {
      return result;
    }
  } catch {
    // Child output is intentionally not included in the evidence record.
  }
  throw new Error("ConPTY probe did not complete its bounded assertions.");
}

async function main() {
  if (process.platform !== "win32") {
    console.log(JSON.stringify({ ok: true, classification: "not_applicable", platform: process.platform }));
    return;
  }
  const { coreRoot } = parseArgs(process.argv.slice(2));
  await stat(path.join(coreRoot, "package.json"));
  const { stdout: coreHead } = await run("git", ["-C", coreRoot, "rev-parse", "HEAD"]);
  if (coreHead.trim() !== pinnedCoreDevelop) {
    throw new Error("Core source is not the pinned develop revision.");
  }
  const { stdout: coreDirty } = await run("git", ["-C", coreRoot, "status", "--porcelain"]);
  if (coreDirty.trim() !== "") {
    throw new Error("Core source must be clean for direct acceptance.");
  }

  const tempRoot = await mkdtemp(path.join(os.tmpdir(), "service-lasso-tui-real-core-"));
  let apiServer;
  try {
    const executable = path.join(tempRoot, "service-lasso-tui.exe");
    await run("go", ["build", "-o", executable, "./cmd/service-lasso-tui"], { cwd: repoRoot });
    const helper = path.join(repoRoot, "scripts", "verify-real-core-conpty.py");
    const unavailableURL = await reserveUnavailableLoopbackURL();
    const unavailable = parseProbe((await run("python", [helper, "--executable", executable, "--mode", "unavailable", "--api-url", unavailableURL])).stdout, "unavailable");
    const core = await import(pathToFileURL(path.join(coreRoot, "packages", "core", "index.js")).href);
    const servicesRoot = path.join(tempRoot, "services");
    const workspaceRoot = path.join(tempRoot, "workspace");
    await Promise.all([mkdir(servicesRoot), mkdir(workspaceRoot)]);
    apiServer = await core.startApiServer({
      port: 0,
      servicesRoot,
      workspaceRoot,
    });
    const connected = parseProbe((await run("python", [helper, "--executable", executable, "--mode", "connected", "--api-url", apiServer.url])).stdout, "connected");
    console.log(JSON.stringify({ ok: true, evidence: "direct-real-core-conpty", coreDevelop: pinnedCoreDevelop, platform: "win32-amd64", unavailable: unavailable.mode, connectedDashboard: "rendered", navigation: connected.navigation, exit: connected.exit }));
  } finally {
    await apiServer?.stop();
    await rm(tempRoot, { recursive: true, force: true });
  }
}

main().catch(() => {
  console.log(JSON.stringify({ ok: false, stage: "bounded-acceptance" }));
  process.exitCode = 1;
});
