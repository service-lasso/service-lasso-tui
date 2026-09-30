import { createHash } from "node:crypto";
import { execFile, spawn } from "node:child_process";
import { mkdtemp, mkdir, readFile, rm, stat, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { promisify } from "node:util";

const execFileAsync = promisify(execFile);
const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const helper = path.join(repoRoot, "scripts", "verify-release-asset-conpty.py");
const candidate = Object.freeze({
  version: "2026.9.30-97fafb0",
  sourceCommit: "97fafb04c69fce8efdd245eb186e6dfb9915485d",
  manifestSha256: "efaa8ed7d433ef6aee4f800efc9b0880a4ae7fa989012bd90191d6afc2cfea04",
  archiveSha256: "b8838f245d4b1d39cac0b51ed2ad14ffd0237779f1a5e9d3e61358066370e479",
  archive: "service-lasso-tui-2026.9.30-97fafb0-win32-amd64.zip",
  executable: "service-lasso-tui.exe",
  assets: Object.freeze([
    { platform: "darwin-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-darwin-amd64.tar.gz", sha256: "5d6df8cfa18771e159b7af34c1c5c00d70ae5eef1ddf727896ae2afb30b63638", executable: "service-lasso-tui" },
    { platform: "darwin-arm64", name: "service-lasso-tui-2026.9.30-97fafb0-darwin-arm64.tar.gz", sha256: "a9523555416a1b332107a9a92cfecc2aa3b7cdd23caab1063540496e4e20b33c", executable: "service-lasso-tui" },
    { platform: "linux-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-linux-amd64.tar.gz", sha256: "238a8e3f92ae5f9cf28c5cd29af91b698addb9bfa546a7b31c7f7a71b7c33e70", executable: "service-lasso-tui" },
    { platform: "win32-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-win32-amd64.zip", sha256: "b8838f245d4b1d39cac0b51ed2ad14ffd0237779f1a5e9d3e61358066370e479", executable: "service-lasso-tui.exe" },
  ]),
});
const coreDevelop = "10e4d72b75c66977ad1dd629991a27443ffc0fd3";
let stage = "setup";
let failureReason;

class CorePreflightFailure extends Error {
  constructor(reason) {
    super(reason);
    this.reason = reason;
  }
}

async function preflightStep(reason, operation) {
  try {
    return await operation();
  } catch (error) {
    if (error instanceof CorePreflightFailure) throw error;
    throw new CorePreflightFailure(reason);
  }
}

const sha256 = value => createHash("sha256").update(value).digest("hex");
const run = (command, args, options = {}) => execFileAsync(command, args, { cwd: options.cwd, windowsHide: true, timeout: options.timeout ?? 120000 });

export function assertCandidateManifest(manifest) {
  const asset = manifest?.assets?.find(item => item?.platform === "win32-amd64");
  const completeAssetBinding = Array.isArray(manifest?.assets) && manifest.assets.length === candidate.assets.length &&
    candidate.assets.every(expected => manifest.assets.some(actual =>
      actual?.platform === expected.platform && actual?.name === expected.name &&
      actual?.sha256 === expected.sha256 && actual?.executable === expected.executable
    ));
  if (
    manifest?.schemaVersion !== 1 || manifest?.kind !== "develop-prerelease-candidate" ||
    manifest?.source?.repository !== "service-lasso/service-lasso-tui" ||
    manifest?.source?.ref !== "refs/heads/develop" || manifest?.source?.commit !== candidate.sourceCommit ||
    manifest?.release?.tag !== `candidate-${candidate.version}` || manifest?.release?.prerelease !== true ||
    manifest?.version !== candidate.version || manifest?.checksumManifest?.name !== "SHA256SUMS.txt" ||
    manifest?.checksumManifest?.sha256 !== "638ad5e54e06dcb894a4579872e788ddc4521cd3ffca46fb06574c8b78de1cf5" ||
    asset?.name !== candidate.archive || asset?.sha256 !== candidate.archiveSha256 || asset?.executable !== candidate.executable ||
    !completeAssetBinding
  ) throw new Error("candidate manifest does not bind the expected Windows release asset");
}

export function parseArgs(argv) {
  let coreRoot;
  let coreKind = "source-built";
  for (let index = 0; index < argv.length; index += 1) {
    if (argv[index] === "--core-root" && argv[index + 1]) {
      coreRoot = path.resolve(argv[index + 1]);
      index += 1;
    } else if (argv[index] === "--core-kind" && ["source-built", "packaged"].includes(argv[index + 1])) {
      coreKind = argv[index + 1];
      index += 1;
    } else {
      throw new Error("usage");
    }
  }
  if (!coreRoot) throw new Error("usage");
  return { coreRoot, coreKind };
}

async function assertRuntimeDist(coreRoot, reason) {
  await preflightStep(reason, async () => {
    const entries = await Promise.all([
      stat(path.join(coreRoot, "package.json")),
      stat(path.join(coreRoot, "packages", "core", "index.js")),
      stat(path.join(coreRoot, "dist", "server", "index.js")),
    ]);
    if (entries.some(entry => !entry.isFile())) throw new Error("invalid runtime dist");
  });
}

export async function prepareSourceBuiltCore({ coreRoot, tempRoot, command = run }) {
  const { stdout: suppliedHead } = await preflightStep("source_identity_unavailable", () => command("git", ["-C", coreRoot, "rev-parse", "HEAD"]));
  if (suppliedHead.trim() !== coreDevelop) throw new CorePreflightFailure("source_identity_mismatch");

  const isolatedRoot = path.join(tempRoot, "core-source");
  // A normal clone (rather than a worktree) keeps npm's dependency and build
  // writes out of the caller-provided checkout and its Git common directory.
  await preflightStep("source_clone_failed", () => command("git", ["clone", "--no-local", "--no-checkout", coreRoot, isolatedRoot]));
  await preflightStep("isolated_checkout_failed", () => command("git", ["-C", isolatedRoot, "checkout", "--detach", coreDevelop]));
  const { stdout: isolatedHead } = await preflightStep("isolated_identity_unavailable", () => command("git", ["-C", isolatedRoot, "rev-parse", "HEAD"]));
  if (isolatedHead.trim() !== coreDevelop) throw new CorePreflightFailure("isolated_identity_mismatch");
  await preflightStep("dependency_install_failed", () => command("npm", ["ci"], { cwd: isolatedRoot, timeout: 300_000 }));
  await preflightStep("source_build_failed", () => command("npm", ["run", "build"], { cwd: isolatedRoot, timeout: 300_000 }));
  await assertRuntimeDist(isolatedRoot, "runtime_dist_unavailable");
  return { coreRoot: isolatedRoot, evidence: "source-built" };
}

export async function prepareCoreRuntime({ coreKind, coreRoot, tempRoot, command = run }) {
  // Retain the historical selector only as a closed preflight failure.  There
  // is no installed Core package identity plus digest contract to verify here.
  if (coreKind === "packaged") throw new CorePreflightFailure("packaged_runtime_invalid");
  return prepareSourceBuiltCore({ coreRoot, tempRoot, command });
}

async function download(url, destination) {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`download failed with ${response.status}`);
  await writeFile(destination, Buffer.from(await response.arrayBuffer()), { flag: "wx" });
}

async function reserveUnavailableLoopbackURL() {
  const { createServer } = await import("node:net");
  const listener = createServer(socket => socket.destroy());
  await new Promise((resolve, reject) => { listener.once("error", reject); listener.listen({ host: "127.0.0.1", port: 0 }, resolve); });
  const address = listener.address();
  if (!address || typeof address === "string") throw new Error("unavailable loopback port was not allocated");
  return {
    port: address.port,
    url: `http://127.0.0.1:${address.port}`,
    close: () => new Promise((resolve, reject) => listener.close(error => error ? reject(error) : resolve())),
  };
}

function parseProbe(stdout, mode) {
  try {
    const result = JSON.parse(stdout.trim());
    if (result?.ok === true && result.mode === mode && result.exit === "q") return result;
    if (result?.ok === false && typeof result.stage === "string") {
      stage = `${mode}-${result.stage}`;
    }
  } catch {}
  throw new Error("release-asset ConPTY probe did not complete its bounded assertions");
}

function startReconnectProbe(executable, apiURL, readyPath, reconnectPath, shutdownRequestPath, shutdownAcknowledgementPath) {
  const child = spawn("python", [
    helper,
    "--executable", executable,
    "--mode", "reconnect",
    "--api-url", apiURL,
    "--ready-file", readyPath,
    "--reconnect-file", reconnectPath,
    "--shutdown-request-file", shutdownRequestPath,
    "--shutdown-acknowledgement-file", shutdownAcknowledgementPath,
  ], { windowsHide: true, stdio: ["ignore", "pipe", "pipe"] });
  let stdout = "";
  child.stdout.setEncoding("utf8");
  child.stdout.on("data", chunk => { stdout += chunk; });
  const exited = new Promise(resolve => child.once("close", resolve));
  const completed = new Promise((resolve, reject) => {
    child.once("error", reject);
    child.once("close", code => {
      try {
        if (code !== 0) throw new Error("release-asset ConPTY reconnect probe failed");
        resolve(parseProbe(stdout, "reconnect"));
      } catch (error) {
        reject(error);
      }
    });
  });
  const probe = { child, completed, exited, shutdownRequestPath, shutdownAcknowledgementPath, completedSuccessfully: false };
  completed.then(() => { probe.completedSuccessfully = true; }, () => undefined);
  return probe;
}

async function waitForFile(file, timeoutMs = 20_000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      await stat(file);
      return;
    } catch (error) {
      if (error?.code !== "ENOENT") throw error;
    }
    await new Promise(resolve => setTimeout(resolve, 50));
  }
  throw new Error("release-asset ConPTY probe did not reach unavailable state");
}

async function waitForAcknowledgement(file, timeoutMs = 5_000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      if ((await readFile(file, "utf8")) === "closed\n") return true;
      return false;
    } catch (error) {
      if (error?.code !== "ENOENT") return false;
    }
    await new Promise(resolve => setTimeout(resolve, 50));
  }
  return false;
}

async function waitForExit(exited, timeoutMs = 5_000) {
  if (!exited) return false;
  return Promise.race([
    Promise.resolve(exited).then(() => true, () => false),
    new Promise(resolve => setTimeout(() => resolve(false), timeoutMs)),
  ]);
}

export async function stopProbe(probe, { write = writeFile, waitForAck = waitForAcknowledgement, waitForHelperExit = waitForExit } = {}) {
  if (!probe?.child) return true;
  // Never address a process by PID here. The helper owns the PtyProcess and
  // must explicitly confirm that its own close completed before its temporary
  // extraction can be removed.
  if (probe.child.exitCode !== null) return probe.completedSuccessfully === true;
  if (!probe.shutdownRequestPath || !probe.shutdownAcknowledgementPath) return false;
  try {
    await write(probe.shutdownRequestPath, "close\n", { flag: "wx" });
  } catch {
    return false;
  }
  if (!await waitForAck(probe.shutdownAcknowledgementPath)) return false;
  return waitForHelperExit(probe.exited);
}

export async function cleanupResources({ probe, apiServer, unavailable, tempRoot }, { stop = stopProbe, remove = rm } = {}) {
  let confirmed = true;
  try {
    confirmed = (await stop(probe)) === true;
  } catch {
    confirmed = false;
  }
  for (const close of [() => apiServer?.stop(), () => unavailable?.close()]) {
    try {
      await close();
    } catch {
      confirmed = false;
    }
  }
  if (confirmed) {
    try {
      await remove(tempRoot, { recursive: true, force: true });
    } catch {
      confirmed = false;
    }
  }
  return confirmed;
}

export function cleanupOutcome(primaryError, cleanupConfirmed) {
  if (primaryError) return primaryError;
  return cleanupConfirmed ? undefined : new Error("cleanup_unconfirmed");
}

async function main() {
  if (process.platform !== "win32") {
    console.log(JSON.stringify({ ok: true, classification: "not_applicable", platform: process.platform }));
    return;
  }
  const { coreRoot: suppliedCoreRoot, coreKind } = parseArgs(process.argv.slice(2));

  const tempRoot = await mkdtemp(path.join(os.tmpdir(), "service-lasso-tui-release-asset-"));
  let apiServer; let unavailable; let probe; let primaryError;
  try {
    stage = "core-runtime-preflight";
    const coreRuntime = await prepareCoreRuntime({ coreKind, coreRoot: suppliedCoreRoot, tempRoot });
    stage = "download";
    const base = `https://github.com/service-lasso/service-lasso-tui/releases/download/candidate-${candidate.version}`;
    const manifestPath = path.join(tempRoot, "candidate-manifest.json");
    const archivePath = path.join(tempRoot, candidate.archive);
    await download(`${base}/candidate-manifest.json`, manifestPath);
    await download(`${base}/${candidate.archive}`, archivePath);
    stage = "digest";
    const [manifestBytes, archiveBytes] = await Promise.all([readFile(manifestPath), readFile(archivePath)]);
    if (sha256(manifestBytes) !== candidate.manifestSha256 || sha256(archiveBytes) !== candidate.archiveSha256) throw new Error("downloaded release asset digest mismatch");
    assertCandidateManifest(JSON.parse(manifestBytes));
    stage = "extract";
    const extractRoot = path.join(tempRoot, "extract");
    await mkdir(extractRoot);
    const { stdout: archiveEntries } = await run("tar", ["-tf", archivePath]);
    if (archiveEntries.trim() !== candidate.executable) throw new Error("archive does not contain only the expected executable path");
    await run("tar", ["-xf", archivePath, "-C", extractRoot]);
    const executable = path.join(extractRoot, candidate.executable);
    await stat(executable);
    stage = "unavailable";
    unavailable = await reserveUnavailableLoopbackURL();
    const readyPath = path.join(tempRoot, "probe-unavailable-ready");
    const reconnectPath = path.join(tempRoot, "probe-reconnect");
    const shutdownRequestPath = path.join(tempRoot, "probe-shutdown-request");
    const shutdownAcknowledgementPath = path.join(tempRoot, "probe-shutdown-acknowledgement");
    probe = startReconnectProbe(executable, unavailable.url, readyPath, reconnectPath, shutdownRequestPath, shutdownAcknowledgementPath);
    await waitForFile(readyPath);
    const loopbackPort = unavailable.port;
    await unavailable.close(); unavailable = undefined;
    stage = "core-start";
    const core = await import(pathToFileURL(path.join(coreRuntime.coreRoot, "packages", "core", "index.js")).href);
    const servicesRoot = path.join(tempRoot, "services"); const workspaceRoot = path.join(tempRoot, "workspace");
    await Promise.all([mkdir(servicesRoot), mkdir(workspaceRoot)]);
    apiServer = await core.startApiServer({ host: "127.0.0.1", port: loopbackPort, servicesRoot, workspaceRoot, noAutostart: true });
    // The listener was explicitly released above; preserve its exact loopback port
    // for the same extracted candidate process that rendered the unavailable state.
    await writeFile(reconnectPath, "ready\n", { flag: "wx" });
    stage = "reconnect";
    const connected = await probe.completed;
    console.log(JSON.stringify({ ok: true, classification: "direct-release-asset-source-built-core-conpty-read", candidate: { version: candidate.version, sourceCommit: candidate.sourceCommit, manifestSha256: candidate.manifestSha256, archiveSha256: candidate.archiveSha256, executable: candidate.executable }, core: { evidence: coreRuntime.evidence, sourceCommit: coreDevelop }, platform: "win32-amd64", unavailable: "rendered", connectedDashboard: "rendered", reconnect: connected.reconnect, navigation: connected.navigation, terminal: { narrowResize: connected.narrowResize }, exit: connected.exit }));
  } catch (error) {
    primaryError = error;
    if (stage === "core-runtime-preflight") failureReason = error instanceof CorePreflightFailure ? error.reason : "preflight_unclassified";
    throw error;
  } finally {
    const cleanupConfirmed = await cleanupResources({ probe, apiServer, unavailable, tempRoot });
    const cleanupError = cleanupOutcome(primaryError, cleanupConfirmed);
    if (cleanupError && cleanupError !== primaryError) {
      stage = "cleanup-unconfirmed";
      failureReason = "cleanup_unconfirmed";
      throw cleanupError;
    }
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main().catch(() => { console.log(JSON.stringify({ ok: false, stage, ...(failureReason ? { reason: failureReason } : {}) })); process.exitCode = 1; });
