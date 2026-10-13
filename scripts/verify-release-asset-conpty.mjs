import { createHash, randomUUID } from "node:crypto";
import { execFile, spawn } from "node:child_process";
import { mkdtemp, mkdir, open, readFile, rm, stat, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { promisify } from "node:util";

const execFileAsync = promisify(execFile);
const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const helper = path.join(repoRoot, "scripts", "verify-release-asset-conpty.py");
const nativeReceiptWriter = path.join(repoRoot, "scripts", "verify-release-asset-receipts.py");
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
const helperReceiptName = "helper-outcome.json";
const nodeReceiptName = "node-exit-outcome.json";
const receiptStages = new Set(["launch", "direct-constructor", "startup", "wait-reconnect", "reconnect", "navigation", "resize-observation", "exit", "helper-exit"]);
const receiptOutcomes = new Set(["normal", "error", "timeout"]);
const receiptReasons = new Set(["completed", "stage_failed", "timed_out", "shutdown_requested", "terminal_exited_zero", "terminal_exit_code_1", "terminal_exit_code_2", "terminal_exited_nonzero", "terminal_signaled", "terminal_unknown", "helper_exit_nonzero", "helper_exit_signal", "helper_exit_spawn_error"]);
const terminalCloseReasons = new Set(["terminal_exited_zero", "terminal_exit_code_1", "terminal_exit_code_2", "terminal_exited_nonzero", "terminal_signaled", "terminal_unknown"]);
const startupBoundaries = new Set(["unclassified", "api_url_invalid", "api_url_scheme", "api_url_userinfo", "api_url_query_or_fragment", "api_token_transport", "api_client_error", "program_run_error", "program_run_killed", "program_run_panic", "program_run_interrupted"]);
let stage = "setup";
let failureReason;

class CorePreflightFailure extends Error {
  constructor(reason, ...original) {
    super(reason, original.length ? { cause: original[0] } : undefined);
    this.reason = reason;
  }
}

async function preflightStep(reason, operation) {
  try {
    return await operation();
  } catch (error) {
    if (error instanceof CorePreflightFailure) throw error;
    throw new CorePreflightFailure(reason, error);
  }
}

const sha256 = value => createHash("sha256").update(value).digest("hex");
const run = (command, args, options = {}) => execFileAsync(command, args, { cwd: options.cwd, windowsHide: true, timeout: options.timeout ?? 120000 });

export function npmCommand(operation, platform = process.platform) {
  // Node cannot directly spawn npm.cmd through execFile on Windows. Keep the
  // supported command lines entirely fixed so the caller-provided Core path
  // never becomes shell input.
  const commands = {
    ci: { npmArgs: ["ci"], windowsCommand: "npm.cmd ci" },
    build: { npmArgs: ["run", "build"], windowsCommand: "npm.cmd run build" },
  };
  const selected = commands[operation];
  if (!selected) throw new Error("unsupported npm preflight operation");
  return platform === "win32"
    ? { program: "cmd.exe", args: ["/d", "/s", "/c", selected.windowsCommand] }
    : { program: "npm", args: selected.npmArgs };
}

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

export function assertExtractedCandidateBuildMetadata(metadata, sourceCommit = candidate.sourceCommit) {
  if (typeof metadata !== "string" || !/^[a-f0-9]{40}$/u.test(sourceCommit) ||
    !metadata.includes(`vcs.revision=${sourceCommit}`) || !metadata.includes("vcs.modified=false")) {
    throw new Error("extracted candidate executable does not carry the clean candidate VCS identity");
  }
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

export async function prepareSourceBuiltCore({ coreRoot, tempRoot, command = run, platform = process.platform }) {
  const { stdout: suppliedHead } = await preflightStep("source_identity_unavailable", () => command("git", ["-C", coreRoot, "rev-parse", "HEAD"]));
  if (suppliedHead.trim() !== coreDevelop) throw new CorePreflightFailure("source_identity_mismatch");
  // The supplied checkout is an object source only. A named branch or dirty
  // state could change the object set during the isolated fetch, so require a
  // clean detached checkout without consulting any tracking or production ref.
  const { stdout: suppliedStatus } = await preflightStep("source_identity_unavailable", () => command("git", ["-C", coreRoot, "status", "--porcelain=v1", "--untracked-files=all"]));
  if (suppliedStatus.trim()) throw new CorePreflightFailure("source_identity_dirty");
  const { stdout: suppliedBranch } = await preflightStep("source_identity_unavailable", () => command("git", ["-C", coreRoot, "branch", "--show-current"]));
  if (suppliedBranch.trim()) throw new CorePreflightFailure("source_identity_attached");

  const isolatedRoot = path.join(tempRoot, "core-source");
  // Build in an isolated repository, without cloning a default branch or
  // tags. The only admitted object is the exact pinned commit; the supplied
  // checkout's governed, exact detached HEAD is the admitted source identity;
  // never consult a possibly stale local origin/develop tracking ref.
  await preflightStep("source_clone_failed", () => command("git", ["init", isolatedRoot]));
  await preflightStep("source_clone_failed", () => command("git", ["-C", isolatedRoot, "fetch", "--no-tags", "--depth=1", coreRoot, coreDevelop]));
  const { stdout: fetchedHead } = await preflightStep("isolated_identity_unavailable", () => command("git", ["-C", isolatedRoot, "rev-parse", "FETCH_HEAD"]));
  if (fetchedHead.trim() !== coreDevelop) throw new CorePreflightFailure("isolated_identity_mismatch");
  await preflightStep("isolated_checkout_failed", () => command("git", ["-C", isolatedRoot, "checkout", "--detach", coreDevelop]));
  const { stdout: isolatedHead } = await preflightStep("isolated_identity_unavailable", () => command("git", ["-C", isolatedRoot, "rev-parse", "HEAD"]));
  if (isolatedHead.trim() !== coreDevelop) throw new CorePreflightFailure("isolated_identity_mismatch");
  const npmCi = npmCommand("ci", platform);
  await preflightStep("dependency_install_failed", () => command(npmCi.program, npmCi.args, { cwd: isolatedRoot, timeout: 300_000 }));
  const npmBuild = npmCommand("build", platform);
  await preflightStep("source_build_failed", () => command(npmBuild.program, npmBuild.args, { cwd: isolatedRoot, timeout: 300_000 }));
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

export function parseProbe(stdout, mode, expectedIdentity) {
  let original; let originalPresent = false;
  try {
    const result = JSON.parse(stdout.trim());
    const expectedKeys = mode === "unavailable"
      ? ["directConstructor", "exit", "mode", "ok", "receipt"]
      : ["directConstructor", "exit", "mode", "narrowResize", "navigation", "ok", "receipt", "reconnect"];
    const actualKeys = Object.keys(result ?? {}).sort();
    const successReceipt = result?.receipt;
    if (
      result?.ok === true && result.mode === mode &&
      actualKeys.length === expectedKeys.length && actualKeys.every((key, index) => key === expectedKeys[index]) &&
      mode === "unavailable" && result.exit === "q" &&
      validateReceipt(successReceipt).stage === "exit" && successReceipt.outcome === "normal" && successReceipt.closedReason === "completed" &&
      matchesCandidateIdentity(successReceipt.candidateIdentity, expectedIdentity) &&
      validDirectConstructor(result.directConstructor, successReceipt.candidateIdentity)
    ) return result;
    if (
      result?.ok === true && result.mode === mode &&
      actualKeys.length === expectedKeys.length && actualKeys.every((key, index) => key === expectedKeys[index]) &&
      mode === "reconnect" &&
      result.reconnect === "r" && Array.isArray(result.navigation) && result.navigation.length === 2 &&
      result.navigation[0] === "d" && result.navigation[1] === "?" &&
      result.narrowResize === "help-screen-rendered-after-50-columns" && result.exit === "q" &&
      validateReceipt(successReceipt).stage === "exit" && successReceipt.outcome === "normal" && successReceipt.closedReason === "completed" &&
      matchesCandidateIdentity(successReceipt.candidateIdentity, expectedIdentity) &&
      validDirectConstructor(result.directConstructor, successReceipt.candidateIdentity)
    ) return result;
    const failureKeys = Object.keys(result ?? {}).sort();
    const hasDirectConstructor = Object.hasOwn(result ?? {}, "directConstructor");
    if (
      result?.ok === false && typeof result.stage === "string" &&
      (hasDirectConstructor ? failureKeys.join(",") === "directConstructor,ok,receipt,stage" : failureKeys.join(",") === "ok,receipt,stage") &&
      validateReceipt(result.receipt).stage === result.stage &&
      (!hasDirectConstructor || validCandidateIdentity(result.receipt.candidateIdentity) && validDirectConstructor(result.directConstructor, result.receipt.candidateIdentity))
    ) {
      stage = `${mode}-${result.stage}`;
    }
  } catch (error) { original = error; originalPresent = true; }
  throw new Error("release-asset ConPTY probe did not complete its bounded assertions", originalPresent ? { cause: original } : undefined);
}

export function validateReceipt(receipt) {
  const keys = Object.keys(receipt ?? {}).sort();
  const hasIdentity = validCandidateIdentity(receipt?.candidateIdentity);
  const expected = hasIdentity
    ? ["candidateIdentity", "closedReason", "outcome", "stage", ...(Object.hasOwn(receipt ?? {}, "startupBoundary") ? ["startupBoundary"] : [])]
    : ["closedReason", "outcome", "stage", ...(Object.hasOwn(receipt ?? {}, "startupBoundary") ? ["startupBoundary"] : [])];
  const hasClosedShape = keys.length === expected.length && keys.every((key, index) => key === expected[index]);
  const hasStartupBoundary = Object.hasOwn(receipt ?? {}, "startupBoundary");
  if (!receipt || !hasClosedShape || !receiptStages.has(receipt.stage) || !receiptOutcomes.has(receipt.outcome) || !receiptReasons.has(receipt.closedReason) || (hasStartupBoundary && (receipt.stage !== "startup" || receipt.outcome !== "error" || !terminalCloseReasons.has(receipt.closedReason) || !startupBoundaries.has(receipt.startupBoundary)))) {
    throw new Error("invalid reconnect receipt");
  }
  return receipt;
}

function validCandidateIdentity(identity) {
  return identity && Object.keys(identity).length === 2 &&
    typeof identity.sourceCommit === "string" && typeof identity.binarySHA256 === "string" &&
    /^[a-f0-9]{40}$/u.test(identity.sourceCommit) && /^[a-f0-9]{64}$/u.test(identity.binarySHA256);
}

function matchesCandidateIdentity(identity, expected) {
  return validCandidateIdentity(identity) && (expected === undefined ||
    validCandidateIdentity(expected) && identity.sourceCommit === expected.sourceCommit && identity.binarySHA256 === expected.binarySHA256);
}

function validDirectConstructor(directConstructor, candidateIdentity) {
  return directConstructor && Object.keys(directConstructor).sort().join(",") === "candidateIdentity,exit,startupBoundary" &&
    directConstructor.exit === "exit_code_2" && directConstructor.startupBoundary === "api_url_invalid" &&
    validCandidateIdentity(directConstructor.candidateIdentity) &&
    directConstructor.candidateIdentity.sourceCommit === candidateIdentity.sourceCommit &&
    directConstructor.candidateIdentity.binarySHA256 === candidateIdentity.binarySHA256;
}

function startWriterRequest(writer, message) {
  return new Promise((resolve, reject) => {
    writer.requests.push({ resolve, reject });
    writer.child.stdin.write(`${JSON.stringify(message)}\n`, error => {
      if (!error) return;
      const request = writer.requests.shift();
      request?.reject(error);
    });
  });
}

export async function startNativeReceiptWriter(baseRoot, { spawnProcess = spawn, writers = [] } = {}) {
  const child = spawnProcess("python", [nativeReceiptWriter, "--base-root", baseRoot], { windowsHide: true, stdio: ["pipe", "pipe", "ignore"] });
  const writer = { child, requests: [], buffer: "", parserFailures: [], childFailures: [], streamFailures: [], startupFailures: [], eventFailures: [], eventHandovers: new WeakMap() };
  writers.push(writer);
  const rejectPending = error => {
    while (writer.requests.length) writer.requests.shift().reject(error);
  };
  let rejectReady;
  child.on("error", original => { writer.childFailures.push(original); writer.eventFailures.push({source:"child", original}); rejectReady?.(original); rejectPending(original); });
  let observeClose;
  writer.closeObserved = new Promise(resolve => { observeClose = resolve; });
  child.once("close", (code, signal) => {
    writer.closedEvent = { code, signal };
    observeClose(writer.closedEvent);
    const error = new Error(`native receipt writer exited (${code ?? "signal"})`);
    if (code !== 0 || signal !== null) { writer.closeFailure = error; writer.eventFailures.push({source:"close", original:error}); writer.closeFailureObserved = true; }
    rejectReady?.(error); rejectPending(error);
  });
  child.stdout.setEncoding("utf8");
  child.stdout.on("error", original => { writer.streamFailures.push(original); writer.eventFailures.push({source:"stdout", original}); rejectReady?.(original); rejectPending(original); });
  child.stdin?.on?.("error", original => { writer.streamFailures.push(original); writer.eventFailures.push({source:"stdin", original}); rejectReady?.(original); rejectPending(original); });
  let readyTimer;
  const ready = new Promise((resolve, reject) => {
    rejectReady = reject;
    readyTimer = setTimeout(() => reject(new Error("native receipt writer did not initialise")), 5_000);
    const onLine = line => {
      let result;
      try { result = JSON.parse(line); } catch (original) { writer.parserFailures.push(original); return; }
      if (!writer.ready && result?.event === "ready" && result.ok === true && typeof result.root === "string" && Object.keys(result).length === 3) {
        clearTimeout(readyTimer); writer.ready = true; resolve(result.root); return;
      }
      if (writer.ready) writer.onLine(result);
    };
    writer.onLine = result => {
      const request = writer.requests.shift();
      if (!request) return;
      if (result?.ok !== true || typeof result.event !== "string") request.reject(new Error("native receipt writer rejected request"));
      else request.resolve(result);
    };
    writer.acceptLine = onLine;
  });
  child.stdout.on("data", chunk => {
    writer.buffer += chunk;
    for (;;) {
      const newline = writer.buffer.indexOf("\n");
      if (newline < 0) break;
      const line = writer.buffer.slice(0, newline); writer.buffer = writer.buffer.slice(newline + 1);
      writer.acceptLine(line);
    }
  });
  try { writer.root = await ready; }
  catch (original) { writer.startupFailures.push(original); throw original; }
  finally { clearTimeout(readyTimer); rejectReady = undefined; }
  return writer;
}

export async function createReceiptSinks(tempRoot, { createDirectory = mkdtemp, openFile = open, platform = process.platform, failures = [], writers = [] } = {}) {
  if (platform === "win32") {
    // The writer creates the attempt directory and both receipts with
    // NtCreateFile relative to a held parent handle. It rejects reparse
    // points while walking and keeps the root non-delete-shareable until
    // cleanup. JavaScript pathname checks are deliberately not used as proof.
    const writer = await startNativeReceiptWriter(path.resolve(tempRoot), { writers });
    return { root: writer.root, helper: { writer, sink: "helper" }, node: { writer, sink: "node" } };
  }
  // Allocate a new unpredictable directory owned by this attempt before a
  // helper starts. Each receipt is then atomically CREATE_NEW-created beneath
  // that directory and only its held handle is used for publication.
  const root = await createDirectory(path.join(path.resolve(tempRoot), "receipts-"));
  const helper = await openFile(path.join(root, helperReceiptName), "wx", 0o600);
  let node;
  try {
    node = await openFile(path.join(root, nodeReceiptName), "wx", 0o600);
  } catch (error) {
    try { await helper.close(); } catch (original) { failures.push(original); }
    throw error;
  }
  return { root, helper, node };
}

export async function publishReceipt(handle, receipt) {
  const finalReceipt = validateReceipt(receipt);
  // A sink is an already-owned CREATE_NEW handle.  There is intentionally no
  // final receipt pathname here: a later root-name substitution cannot change
  // the object receiving these bytes, and cleanup never unlinks a receipt.
  try {
    if (handle?.writer && typeof handle.sink === "string") {
      const result = await startWriterRequest(handle.writer, { op: "write", sink: handle.sink, receipt: finalReceipt });
      if (result.event !== "written" || result.sink !== handle.sink || Object.keys(result).length !== 3) throw new Error("native receipt writer returned invalid acknowledgement");
      return finalReceipt;
    }
    await handle.writeFile(JSON.stringify(finalReceipt));
    await handle.sync();
    return finalReceipt;
  } catch (error) { throw error; }
}

export async function closeReceiptSinks(sinks, { failures = [], closeEvents = [], handoff = {} } = {}) {
  const first = failures.length;
  const throwRecorded = original => { handoff.recorded = true; throw original; };
  const writer = sinks?.helper?.writer ?? sinks?.node?.writer;
  if (writer) {
    writer.closeObserved ??= new Promise(resolve => writer.child.once("close", (code, signal) => resolve({ code, signal })));
    writer.eventFailures ??= [];
    writer.eventHandovers ??= new WeakMap();
    writer.closeEventHandovers ??= new WeakSet();
    if (!writer.closeResult) writer.closeResult = Promise.resolve().then(async () => {
      if (writer.closedEvent === undefined) {
        if (!writer.closeRequest) writer.closeRequest = Promise.resolve().then(() => {
          writer.originalCloseRequest = startWriterRequest(writer, { op: "close" });
          return writer.originalCloseRequest;
        });
        try { await writer.closeRequest; } catch (original) { writer.eventFailures.push({source:"close-request", original}); }
      }
      let closed, timer;
      try { closed = await Promise.race([writer.closeObserved, new Promise(resolve => { timer = setTimeout(() => resolve(false), 5_000); })]); }
      catch (original) { writer.eventFailures.push({source:"close-observation", original}); }
      finally { clearTimeout(timer); }
      if (closed && (closed.code !== 0 || closed.signal !== null) && !writer.closeFailureObserved) {
        writer.closeFailure ??= new Error("cleanup_unconfirmed");
        writer.eventFailures.push({source:"close", original:writer.closeFailure}); writer.closeFailureObserved = true;
      }
      if (!closed && !writer.eventFailures.length) writer.eventFailures.push({source:"close-unconfirmed", original:new Error("cleanup_unconfirmed")});
      return closed;
    });
    const closed = await writer.closeResult;
    const handed = writer.eventHandovers.get(failures) ?? 0;
    for (const event of writer.eventFailures.slice(handed)) failures.push(event.original);
    writer.eventHandovers.set(failures, writer.eventFailures.length);
    if (closed && !writer.closeEventHandovers.has(closeEvents)) { closeEvents.push(closed); writer.closeEventHandovers.add(closeEvents); }
    if (failures.length > first) throwRecorded(failures[first]);
    if (writer.eventFailures?.length) throwRecorded(writer.eventFailures[0].original);
    return;
  }
  const results = await Promise.allSettled([sinks?.helper, sinks?.node].map(sink => Promise.resolve().then(() => sink?.close())));
  for (const result of results) if (result.status === "rejected") failures.push(result.reason);
  if (failures.length > first) throwRecorded(failures[first]);
}
export async function persistNodeExitReceipt(nodeSink, code, signal, spawnFailed = false) {
  // Node owns this terminal observation. Reading the helper's independently
  // published pathname would reintroduce a lstat/read replacement race.
  const closedReason = spawnFailed ? "helper_exit_spawn_error" : signal ? "helper_exit_signal" : code === 0 ? "completed" : "helper_exit_nonzero";
  return publishReceipt(nodeSink, {
    stage: "helper-exit",
    outcome: closedReason === "completed" ? "normal" : "error",
    closedReason,
  });
}

function helperReceiptFromOutput(stdout, parserFailures) {
  try {
    const result = JSON.parse(stdout.trim());
    return result?.receipt ? validateReceipt(result.receipt) : undefined;
  } catch (original) { parserFailures.push(original); return undefined; }
}

export async function finalizeReconnectExit(receiptSinks, code, signal, stdout, { persist = persistNodeExitReceipt, candidateIdentity, observation = { receiptFailures: [], parserFailures: [], primaryFailures: [] } } = {}) {
  const receiptFailures = observation.receiptFailures;
  try {
    const helperReceipt = helperReceiptFromOutput(stdout, observation.parserFailures);
    if (helperReceipt) await publishReceipt(receiptSinks.helper, helperReceipt);
  } catch (error) {
    receiptFailures.push(error);
  }
  // Node owns this independent observation even if the helper sink rejects.
  try { await persist(receiptSinks.node, code, signal); }
  catch (error) { receiptFailures.push(error); }
  // A receipt-sink failure must never replace the helper's primary result.
  let result; let primaryError; let primaryPresent = false;
  try {
    if (code !== 0 || signal) throw new Error("release-asset ConPTY reconnect probe failed");
    result = parseProbe(stdout, "reconnect", candidateIdentity);
  } catch (error) { primaryError = error; primaryPresent = true; observation.primaryFailures.push(error); }
  if (primaryPresent) throw primaryError;
  if (receiptFailures.length) throw new AggregateError(receiptFailures, "reconnect receipt persistence failed");
  return result;
}

export function startReconnectProbe(executable, apiURL, readyPath, reconnectPath, shutdownRequestPath, shutdownAcknowledgementPath, shutdownToken, receiptSinks, candidateIdentity, { spawnProcess = spawn, observation = { receiptFailures: [], parserFailures: [], primaryFailures: [] } } = {}) {
  if (!validCandidateIdentity(candidateIdentity)) throw new Error("reconnect candidate identity invalid");
  const heldIdentity = Object.freeze({ ...candidateIdentity });
  const child = spawnProcess("python", [
    helper,
    "--executable", executable,
    "--source-commit", heldIdentity.sourceCommit,
    "--expected-executable-sha256", heldIdentity.binarySHA256,
    "--mode", "reconnect",
    "--api-url", apiURL,
    "--ready-file", readyPath,
    "--reconnect-file", reconnectPath,
    "--shutdown-request-file", shutdownRequestPath,
    "--shutdown-acknowledgement-file", shutdownAcknowledgementPath,
    "--shutdown-token", shutdownToken,
  ], { windowsHide: true, stdio: ["ignore", "pipe", "pipe"] });
  let stdout = "";
  child.stdout.setEncoding("utf8");
  child.stdout.on("data", chunk => { stdout += chunk; });
  const exited = new Promise(resolve => child.once("close", resolve));
  const completed = new Promise((resolve, reject) => {
    let spawnFailed = false;
    child.on("error", error => {
      observation.primaryFailures.push(error);
      if (spawnFailed) return;
      spawnFailed = true;
      // Preserve the original spawn error.  The independent Node receipt
      // records this terminal state without fabricating a helper result.
      Promise.resolve().then(() => persistNodeExitReceipt(receiptSinks.node, null, null, true)).catch(receiptFailure => {
        observation.receiptFailures.push(receiptFailure);
      }).finally(() => reject(error));
    });
    for (const name of ["stdout", "stderr"]) child[name]?.on?.("error", original => {
      observation.primaryFailures.push(original);
      if (spawnFailed) return;
      spawnFailed = true;
      reject(original);
    });
    child.once("close", async (code, signal) => {
      try {
        if (!spawnFailed) resolve(await finalizeReconnectExit(receiptSinks, code, signal, stdout, { candidateIdentity: heldIdentity, observation }));
      } catch (error) {
        reject(error);
      }
    });
  });
  const probe = { child, completed, exited, shutdownRequestPath, shutdownAcknowledgementPath, shutdownToken, completedSuccessfully: false };
  Object.defineProperty(probe, "observation", { value: observation });
  completed.then(() => { probe.completedSuccessfully = true; }, () => undefined);
  return probe;
}

export async function waitForFile(file, timeoutMs = 20_000, observations = [], inspect = stat) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      await inspect(file);
      return;
    } catch (error) {
      observations.push(error);
      let code;
      try { code = error?.code; } catch (secondary) { observations.push(secondary); throw error; }
      if (code !== "ENOENT") throw error;
    }
    await new Promise(resolve => setTimeout(resolve, 50));
  }
  throw new Error("release-asset ConPTY probe did not reach unavailable state");
}

export async function waitForAcknowledgement(file, token, timeoutMs = 5_000, failures = [], observations = [], read = readFile) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      if ((await read(file, "utf8")) === `${token}\n`) return true;
      return false;
    } catch (error) {
      observations.push(error);
      let code;
      try { code = error?.code; } catch (secondary) { failures.push(error, secondary); return false; }
      if (code !== "ENOENT") { failures.push(error); return false; }
    }
    await new Promise(resolve => setTimeout(resolve, 50));
  }
  return false;
}

async function waitForExit(exited, timeoutMs = 5_000, failures = []) {
  if (!exited) return false;
  return Promise.race([
    Promise.resolve(exited).then(() => true, original => { failures.push(original); return false; }),
    new Promise(resolve => setTimeout(() => resolve(false), timeoutMs)),
  ]);
}

export async function stopProbe(probe, { write = writeFile, waitForAck = waitForAcknowledgement, waitForHelperExit = waitForExit, failures = [], observations = [] } = {}) {
  if (!probe?.child) return true;
  // Never address a process by PID here. The helper owns the PtyProcess and
  // must explicitly confirm that its own close completed before its temporary
  // extraction can be removed.
  if (probe.child.exitCode !== null) return probe.completedSuccessfully === true;
  if (!probe.shutdownRequestPath || !probe.shutdownAcknowledgementPath || !probe.shutdownToken) return false;
  try {
    await write(probe.shutdownRequestPath, `${probe.shutdownToken}\n`, { flag: "wx" });
  } catch (original) {
    failures.push(original);
    return false;
  }
  const acknowledged = waitForAck === waitForAcknowledgement ? await waitForAck(probe.shutdownAcknowledgementPath, probe.shutdownToken, 5_000, failures, observations) : await waitForAck(probe.shutdownAcknowledgementPath, probe.shutdownToken);
  if (!acknowledged) return false;
  return waitForHelperExit === waitForExit ? waitForHelperExit(probe.exited, 5_000, failures) : waitForHelperExit(probe.exited);
}

export async function cleanupResources({ probe, apiServer, unavailable, tempRoot, retainTempRoot = false, failures = [], observations = [] }, { stop = stopProbe, remove = rm } = {}) {
  let confirmed = true;
  try {
    confirmed = (await (stop === stopProbe ? stop(probe, { failures, observations }) : stop(probe))) === true;
  } catch (original) {
    failures.push(original);
    confirmed = false;
  }
  for (const close of [() => apiServer?.stop(), () => unavailable?.close()]) {
    try {
      await close();
    } catch (original) {
      failures.push(original);
      confirmed = false;
    }
  }
  // On Windows the native receipt writer anchors the attempt root. Closing it
  // solely to recurse by pathname would re-open the substitution window. Keep
  // that attempt-owned evidence for explicit inspection instead.
  if (confirmed && !retainTempRoot) {
    try {
      await remove(tempRoot, { recursive: true, force: true });
    } catch (original) {
      failures.push(original);
      confirmed = false;
    }
  }
  return confirmed;
}

export function cleanupOutcome(primaryError, cleanupConfirmed) {
  if (primaryError) return primaryError;
  return cleanupConfirmed ? undefined : new Error("cleanup_unconfirmed");
}

export async function finalizeProbeOutcome(primary, cleanup, close, { primaryPresent = primary !== undefined, observation = { cleanupOriginals: [] } } = {}) {
  const originals = observation.cleanupOriginals;
  let confirmed = false;
  try { confirmed = await cleanup(originals) === true; } catch (original) { originals.push(original); }
  try { await close(originals); } catch (original) { originals.push(original); }
  if (primaryPresent) throw primary;
  if (originals.length) throw originals[0];
  if (!confirmed) throw new Error("cleanup_unconfirmed");
}
async function main() {
  if (process.platform !== "win32") {
    console.log(JSON.stringify({ ok: true, classification: "not_applicable", platform: process.platform }));
    return;
  }
  const { coreRoot: suppliedCoreRoot, coreKind } = parseArgs(process.argv.slice(2));

  let tempRoot; let apiServer; let unavailable; let probe; let receiptSinks; let primaryError; let primaryPresent = false; let successfulResult; const finalizationObservation = { cleanupOriginals: [], closeEvents: [], observations: [], receiptWriters: [] };
  try {
    receiptSinks = await createReceiptSinks(os.tmpdir(), { failures: finalizationObservation.cleanupOriginals, writers: finalizationObservation.receiptWriters });
    tempRoot = receiptSinks.root;
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
    assertExtractedCandidateBuildMetadata((await run("go", ["version", "-m", executable])).stdout);
    // Bind once to the actual extracted bytes. Python independently verifies
    // this digest through its non-write/delete-shared handle and retains that
    // same identity across the direct constructor and ConPTY journey.
    const extractedIdentity = Object.freeze({ sourceCommit: candidate.sourceCommit, binarySHA256: sha256(await readFile(executable)) });
    stage = "unavailable";
    unavailable = await reserveUnavailableLoopbackURL();
    const readyPath = path.join(tempRoot, "probe-unavailable-ready");
    const reconnectPath = path.join(tempRoot, "probe-reconnect");
    const shutdownRequestPath = path.join(tempRoot, "probe-shutdown-request");
    const shutdownAcknowledgementPath = path.join(tempRoot, "probe-shutdown-acknowledgement");
    probe = startReconnectProbe(executable, unavailable.url, readyPath, reconnectPath, shutdownRequestPath, shutdownAcknowledgementPath, randomUUID(), receiptSinks, extractedIdentity);
    await waitForFile(readyPath, 20_000, finalizationObservation.observations);
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
    successfulResult = { ok: true, classification: "direct-release-asset-source-built-core-conpty-read", candidate: { version: candidate.version, sourceCommit: candidate.sourceCommit, manifestSha256: candidate.manifestSha256, archiveSha256: candidate.archiveSha256, executable: candidate.executable }, core: { evidence: coreRuntime.evidence, sourceCommit: coreDevelop }, platform: "win32-amd64", unavailable: "rendered", connectedDashboard: "rendered", reconnect: connected.reconnect, navigation: connected.navigation, terminal: { narrowResize: connected.narrowResize }, exit: connected.exit };
  } catch (error) {
    primaryError = error; primaryPresent = true;
    if (stage === "core-runtime-preflight") failureReason = error instanceof CorePreflightFailure ? error.reason : "preflight_unclassified";
  } finally {
    // Keep the native root and receipt handles anchored while the cooperating
    // probe and loopback resources stop. Windows deliberately retains that
    // root afterwards instead of doing a pathname-recursive delete.
    try {
      await finalizeProbeOutcome(primaryError,
        failures => tempRoot ? cleanupResources({ probe, apiServer, unavailable, tempRoot, retainTempRoot: process.platform === "win32", failures, observations: finalizationObservation.observations }) : false,
        async failures => {
          // closeReceiptSinks records all actual component originals before
          // handing its first failure back; do not count that handoff twice.
          const handoff = {};
          try { await closeReceiptSinks(receiptSinks, { failures, closeEvents: finalizationObservation.closeEvents, handoff }); } catch (original) { if (!handoff.recorded) failures.push(original); }
        }, { primaryPresent, observation: finalizationObservation });
    } catch (original) {
      if (!primaryPresent) { stage = "cleanup-unconfirmed"; failureReason = "cleanup_unconfirmed"; }
      throw original;
    }
  }
  console.log(JSON.stringify(successfulResult));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main().catch(() => { console.log(JSON.stringify({ ok: false, stage, ...(failureReason ? { reason: failureReason } : {}) })); process.exitCode = 1; });
