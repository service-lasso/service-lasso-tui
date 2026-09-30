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
let acceptanceStage = "setup";

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
  // Keep the listener open for the entire unavailable probe. Closing an
  // ephemeral listener before spawning the child lets another process claim
  // that port and turns this bounded negative check into an accidental live
  // runtime probe. Reset accepted connections so the TUI observes an
  // unavailable API promptly while ownership of the port is retained.
  const listener = createServer(socket => socket.destroy());
  await new Promise((resolve, reject) => {
    listener.once("error", reject);
    listener.listen({ host: "127.0.0.1", port: 0 }, resolve);
  });
  const address = listener.address();
  if (!address || typeof address === "string") {
    await new Promise((resolve, reject) => listener.close(error => error ? reject(error) : resolve()));
    throw new Error("unavailable loopback port was not allocated");
  }
  return {
    url: `http://127.0.0.1:${address.port}`,
    async close() {
      if (!listener.listening) {
        return;
      }
      await new Promise((resolve, reject) => listener.close(error => error ? reject(error) : resolve()));
    },
  };
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

function normalizedArchitecture(value) {
  return String(value ?? "").trim().toLowerCase().replace(/[^a-z0-9]/g, "");
}

export function isAmd64Architecture(value) {
  return ["amd64", "x64", "x8664"].includes(normalizedArchitecture(value));
}

export function assertAmd64Evidence({ hostArchitecture, nodeArchitecture, helperArchitecture }) {
  if (!isAmd64Architecture(hostArchitecture)) throw new Error("The Windows host architecture is not AMD64.");
  if (nodeArchitecture !== "x64") throw new Error("The Node process architecture is not x64.");
  if (!isAmd64Architecture(helperArchitecture?.machine) || helperArchitecture?.pointerBits !== 64) throw new Error("The ConPTY helper architecture is not AMD64.");
}

async function getWindowsHostArchitecture() {
  const { stdout } = await run("powershell", ["-NoProfile", "-NonInteractive", "-Command", "[System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()"]);
  return stdout.trim();
}

async function getHelperArchitecture(helper) {
  const { stdout } = await run("python", [helper, "--architecture"]);
  try {
    const result = JSON.parse(stdout.trim());
    if (result?.ok === true && result.architecture) return result.architecture;
  } catch {
    // Helper output is intentionally not included in the evidence record.
  }
  throw new Error("ConPTY helper did not report its architecture.");
}

async function main() {
  if (process.platform !== "win32") {
    console.log(JSON.stringify({ ok: true, classification: "not_applicable", platform: process.platform }));
    return;
  }
  const { coreRoot } = parseArgs(process.argv.slice(2));
  acceptanceStage = "architecture";
  const helper = path.join(repoRoot, "scripts", "verify-real-core-conpty.py");
  const hostArchitecture = await getWindowsHostArchitecture();
  const helperArchitecture = await getHelperArchitecture(helper);
  assertAmd64Evidence({ hostArchitecture, nodeArchitecture: process.arch, helperArchitecture });
  acceptanceStage = "core-source";
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
  let unavailableReservation;
  try {
    acceptanceStage = "build";
    const executable = path.join(tempRoot, "service-lasso-tui.exe");
    await run("go", ["build", "-o", executable, "./cmd/service-lasso-tui"], { cwd: repoRoot });
    acceptanceStage = "unavailable";
    unavailableReservation = await reserveUnavailableLoopbackURL();
    const unavailable = parseProbe((await run("python", [helper, "--executable", executable, "--mode", "unavailable", "--api-url", unavailableReservation.url])).stdout, "unavailable");
    await unavailableReservation.close();
    unavailableReservation = undefined;
    acceptanceStage = "core-import";
    const core = await import(pathToFileURL(path.join(coreRoot, "packages", "core", "index.js")).href);
    const servicesRoot = path.join(tempRoot, "services");
    const workspaceRoot = path.join(tempRoot, "workspace");
    await Promise.all([mkdir(servicesRoot), mkdir(workspaceRoot)]);
    acceptanceStage = "core-start";
    apiServer = await core.startApiServer({
      port: 0,
      servicesRoot,
      workspaceRoot,
    });
    acceptanceStage = "connected";
    const connected = parseProbe((await run("python", [helper, "--executable", executable, "--mode", "connected", "--api-url", apiServer.url])).stdout, "connected");
    console.log(JSON.stringify({ ok: true, evidence: "direct-real-core-conpty", coreDevelop: pinnedCoreDevelop, platform: "win32-amd64", architecture: { host: normalizedArchitecture(hostArchitecture), node: process.arch, helper: helperArchitecture }, unavailable: unavailable.mode, connectedDashboard: "rendered", navigation: connected.navigation, exit: connected.exit }));
  } finally {
    try {
      await apiServer?.stop();
    } finally {
      try {
        await unavailableReservation?.close();
      } finally {
        await rm(tempRoot, { recursive: true, force: true });
      }
    }
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  main().catch(() => {
    console.log(JSON.stringify({ ok: false, stage: acceptanceStage }));
    process.exitCode = 1;
  });
}
