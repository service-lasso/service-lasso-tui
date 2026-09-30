import { createHash } from "node:crypto";
import { execFile } from "node:child_process";
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
});
const coreDevelop = "10e4d72b75c66977ad1dd629991a27443ffc0fd3";
let stage = "setup";

const sha256 = value => createHash("sha256").update(value).digest("hex");
const run = (command, args, options = {}) => execFileAsync(command, args, { cwd: options.cwd, windowsHide: true, timeout: options.timeout ?? 120000 });

export function assertCandidateManifest(manifest) {
  const asset = manifest?.assets?.find(item => item?.platform === "win32-amd64");
  if (
    manifest?.schemaVersion !== 1 || manifest?.kind !== "develop-prerelease-candidate" ||
    manifest?.source?.repository !== "service-lasso/service-lasso-tui" ||
    manifest?.source?.ref !== "refs/heads/develop" || manifest?.source?.commit !== candidate.sourceCommit ||
    manifest?.release?.tag !== `candidate-${candidate.version}` || manifest?.release?.prerelease !== true ||
    manifest?.version !== candidate.version || manifest?.checksumManifest?.name !== "SHA256SUMS.txt" ||
    manifest?.checksumManifest?.sha256 !== "638ad5e54e06dcb894a4579872e788ddc4521cd3ffca46fb06574c8b78de1cf5" ||
    asset?.name !== candidate.archive || asset?.sha256 !== candidate.archiveSha256 || asset?.executable !== candidate.executable
  ) throw new Error("candidate manifest does not bind the expected Windows release asset");
}

function parseArgs(argv) {
  const marker = argv.indexOf("--core-root");
  if (marker < 0 || !argv[marker + 1] || argv.length !== 2) throw new Error("usage");
  return { coreRoot: path.resolve(argv[marker + 1]) };
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
  return { url: `http://127.0.0.1:${address.port}`, close: () => new Promise((resolve, reject) => listener.close(error => error ? reject(error) : resolve())) };
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

async function runProbe(executable, mode, apiURL) {
  try {
    return parseProbe((await run("python", [helper, "--executable", executable, "--mode", mode, "--api-url", apiURL])).stdout, mode);
  } catch (error) {
    return parseProbe(error?.stdout ?? "", mode);
  }
}

async function main() {
  if (process.platform !== "win32") {
    console.log(JSON.stringify({ ok: true, classification: "not_applicable", platform: process.platform }));
    return;
  }
  const { coreRoot } = parseArgs(process.argv.slice(2));
  stage = "core-head";
  await stat(path.join(coreRoot, "package.json"));
  const { stdout: coreHead } = await run("git", ["-C", coreRoot, "rev-parse", "HEAD"]);
  if (coreHead.trim() !== coreDevelop) throw new Error("Core source is not the pinned develop revision");

  const tempRoot = await mkdtemp(path.join(os.tmpdir(), "service-lasso-tui-release-asset-"));
  let apiServer; let unavailable;
  try {
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
    await runProbe(executable, "unavailable", unavailable.url);
    await unavailable.close(); unavailable = undefined;
    stage = "core-start";
    const core = await import(pathToFileURL(path.join(coreRoot, "packages", "core", "index.js")).href);
    const servicesRoot = path.join(tempRoot, "services"); const workspaceRoot = path.join(tempRoot, "workspace");
    await Promise.all([mkdir(servicesRoot), mkdir(workspaceRoot)]);
    apiServer = await core.startApiServer({ port: 0, servicesRoot, workspaceRoot, noAutostart: true });
    stage = "connected";
    const connected = await runProbe(executable, "connected", apiServer.url);
    console.log(JSON.stringify({ ok: true, classification: "direct-release-asset-conpty-read", candidate: { version: candidate.version, sourceCommit: candidate.sourceCommit, manifestSha256: candidate.manifestSha256, archiveSha256: candidate.archiveSha256, executable: candidate.executable }, coreDevelop, platform: "win32-amd64", unavailable: "rendered", connectedDashboard: "rendered", navigation: connected.navigation, terminal: { narrowResize: connected.narrowResize }, exit: connected.exit }));
  } finally {
    await apiServer?.stop();
    await unavailable?.close();
    await rm(tempRoot, { recursive: true, force: true });
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main().catch(() => { console.log(JSON.stringify({ ok: false, stage })); process.exitCode = 1; });
