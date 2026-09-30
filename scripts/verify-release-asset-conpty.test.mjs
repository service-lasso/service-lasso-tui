import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import test from "node:test";
import { access, mkdir, readFile, rm, symlink, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { assertCandidateManifest, assertExtractedCandidateBuildMetadata, cleanupOutcome, cleanupResources, closeReceiptSinks, createReceiptSinks, finalizeReconnectExit, npmCommand, parseArgs, parseProbe, persistNodeExitReceipt, prepareCoreRuntime, prepareSourceBuiltCore, publishReceipt, stopProbe, validateReceipt } from "./verify-release-asset-conpty.mjs";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

const manifest = { schemaVersion: 1, kind: "develop-prerelease-candidate", source: { repository: "service-lasso/service-lasso-tui", ref: "refs/heads/develop", commit: "97fafb04c69fce8efdd245eb186e6dfb9915485d" }, release: { tag: "candidate-2026.9.30-97fafb0", prerelease: true }, version: "2026.9.30-97fafb0", checksumManifest: { name: "SHA256SUMS.txt", sha256: "638ad5e54e06dcb894a4579872e788ddc4521cd3ffca46fb06574c8b78de1cf5" }, assets: [{ platform: "darwin-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-darwin-amd64.tar.gz", sha256: "5d6df8cfa18771e159b7af34c1c5c00d70ae5eef1ddf727896ae2afb30b63638", executable: "service-lasso-tui" }, { platform: "darwin-arm64", name: "service-lasso-tui-2026.9.30-97fafb0-darwin-arm64.tar.gz", sha256: "a9523555416a1b332107a9a92cfecc2aa3b7cdd23caab1063540496e4e20b33c", executable: "service-lasso-tui" }, { platform: "linux-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-linux-amd64.tar.gz", sha256: "238a8e3f92ae5f9cf28c5cd29af91b698addb9bfa546a7b31c7f7a71b7c33e70", executable: "service-lasso-tui" }, { platform: "win32-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-win32-amd64.zip", sha256: "b8838f245d4b1d39cac0b51ed2ad14ffd0237779f1a5e9d3e61358066370e479", executable: "service-lasso-tui.exe" }] };

test("accepts only the pinned Windows candidate manifest binding", () => assert.doesNotThrow(() => assertCandidateManifest(manifest)));
test("requires the extracted candidate executable to retain its clean candidate VCS identity", () => {
  const metadata = `build\tvcs.revision=${manifest.source.commit}\nbuild\tvcs.modified=false\n`;
  assert.doesNotThrow(() => assertExtractedCandidateBuildMetadata(metadata));
  assert.throws(() => assertExtractedCandidateBuildMetadata(`build\tvcs.revision=${manifest.source.commit}\nbuild\tvcs.modified=true\n`), /clean candidate VCS identity/u);
  assert.throws(() => assertExtractedCandidateBuildMetadata("build\tvcs.revision=foreign\nbuild\tvcs.modified=false\n"), /clean candidate VCS identity/u);
});
test("rejects a mutable or mismatched Windows archive binding", () => {
  const changed = structuredClone(manifest); changed.assets[0].sha256 = "0".repeat(64);
  assert.throws(() => assertCandidateManifest(changed), /candidate manifest/);
});

test("rejects a candidate that changes its source, release identity, or checksum manifest", () => {
  for (const mutate of [
    value => { value.source.commit = "0".repeat(40); },
    value => { value.release.tag = "candidate-2026.9.30-latest"; },
    value => { value.checksumManifest.sha256 = "0".repeat(64); },
  ]) {
    const changed = structuredClone(manifest);
    mutate(changed);
    assert.throws(() => assertCandidateManifest(changed), /candidate manifest/);
  }
});

test("rejects an incomplete candidate asset inventory", () => {
  const changed = structuredClone(manifest);
  changed.assets.pop();
  assert.throws(() => assertCandidateManifest(changed), /candidate manifest/);
});

test("requires the isolated source-built Core preflight and retains packaged selection as a closed failure", async () => {
  assert.deepEqual(parseArgs(["--core-root", "core"]), { coreRoot: path.resolve("core"), coreKind: "source-built" });
  assert.deepEqual(parseArgs(["--core-kind", "packaged", "--core-root", "core"]), { coreRoot: path.resolve("core"), coreKind: "packaged" });
  await assert.rejects(
    () => prepareCoreRuntime({ coreKind: "packaged", coreRoot: "source-shaped-checkout", tempRoot: os.tmpdir() }),
    error => error?.reason === "packaged_runtime_invalid" && error.message === "packaged_runtime_invalid",
  );
});

test("source Core preparation admits only the exact pinned commit and never trusts a local develop tracking ref", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-core-preflight-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  const commands = [];
  const isolatedRoot = path.join(tempRoot, "core-source");
  const command = async (program, args, options = {}) => {
    commands.push({ program, args, options });
    if (program === "git" && args.includes("rev-parse")) return { stdout: "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" };
    if (program === "npm" && args.join(" ") === "run build") {
      await Promise.all([
        mkdir(path.join(isolatedRoot, "packages", "core"), { recursive: true }),
        mkdir(path.join(isolatedRoot, "dist", "server"), { recursive: true }),
      ]);
      await Promise.all([
        writeFile(path.join(isolatedRoot, "package.json"), "{}"),
        writeFile(path.join(isolatedRoot, "packages", "core", "index.js"), ""),
        writeFile(path.join(isolatedRoot, "dist", "server", "index.js"), ""),
      ]);
    }
    return { stdout: "" };
  };
  try {
    const prepared = await prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command, platform: "linux" });
    assert.deepEqual(prepared, { coreRoot: isolatedRoot, evidence: "source-built" });
    assert.deepEqual(commands.map(({ program, args }) => [program, args]), [
      ["git", ["-C", "supplied-core", "rev-parse", "HEAD"]],
      ["git", ["-C", "supplied-core", "status", "--porcelain=v1", "--untracked-files=all"]],
      ["git", ["-C", "supplied-core", "branch", "--show-current"]],
      ["git", ["init", isolatedRoot]],
      ["git", ["-C", isolatedRoot, "fetch", "--no-tags", "--depth=1", "supplied-core", "10e4d72b75c66977ad1dd629991a27443ffc0fd3"]],
      ["git", ["-C", isolatedRoot, "rev-parse", "FETCH_HEAD"]],
      ["git", ["-C", isolatedRoot, "checkout", "--detach", "10e4d72b75c66977ad1dd629991a27443ffc0fd3"]],
      ["git", ["-C", isolatedRoot, "rev-parse", "HEAD"]],
      ["npm", ["ci"]],
      ["npm", ["run", "build"]],
    ]);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("source Core preparation never inspects a supplied origin/develop tracking ref", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-core-preflight-ancestry-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  try {
    const commands = [];
    await prepareSourceBuiltCore({
      coreRoot: "supplied-core", tempRoot, platform: "linux",
      command: async (_program, args) => {
        commands.push(args);
        if (args.join(" ") === "run build") {
          await Promise.all([mkdir(path.join(tempRoot, "core-source", "packages", "core"), { recursive: true }), mkdir(path.join(tempRoot, "core-source", "dist", "server"), { recursive: true })]);
          await Promise.all([writeFile(path.join(tempRoot, "core-source", "package.json"), "{}"), writeFile(path.join(tempRoot, "core-source", "packages", "core", "index.js"), ""), writeFile(path.join(tempRoot, "core-source", "dist", "server", "index.js"), "")]);
        }
        return { stdout: args.includes("rev-parse") ? "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" : "" };
      },
    });
    assert.equal(commands.flat().some(argument => argument.includes("origin/develop")), false);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("source Core preparation requires the supplied pinned object to be clean and detached", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-core-preflight-identity-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  const command = async (_program, args) => {
    if (args.includes("rev-parse")) return { stdout: "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" };
    if (args.includes("status")) return { stdout: "?? unintended\n" };
    return { stdout: "" };
  };
  try {
    await assert.rejects(() => prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command }), error => error?.reason === "source_identity_dirty");
    await assert.rejects(() => prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command: async (_program, args) => {
      if (args.includes("rev-parse")) return { stdout: "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" };
      if (args.includes("branch")) return { stdout: "some-branch\n" };
      return { stdout: "" };
    } }), error => error?.reason === "source_identity_attached");
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("Windows receipt construction rejects a reparse-point base before creating an attempt root", { skip: process.platform !== "win32" }, async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-receipt-reparse-${process.pid}-${Date.now()}`);
  const target = path.join(tempRoot, "target"); const alias = path.join(tempRoot, "alias");
  await mkdir(target, { recursive: true }); await symlink(target, alias, "junction");
  try {
    await assert.rejects(() => createReceiptSinks(alias), /native receipt writer/u);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("Windows native receipt ownership denies a concurrent writer and reads the exact published bytes", { skip: process.platform !== "win32" }, async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-receipt-ownership-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  try {
    const { stdout } = await new Promise((resolve, reject) => {
      execFile("python", [path.join(repoRoot, "scripts", "verify-release-asset-receipts.py"), "--base-root", tempRoot, "--ownership-self-test"], { windowsHide: true }, (error, stdout, stderr) => {
        if (error) reject(error); else resolve({ stdout, stderr });
      });
    });
    assert.deepEqual(JSON.parse(stdout), { ok: true, event: "ownership-self-test" });
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("Windows native receipt writer accepts only the closed startup-boundary extension", { skip: process.platform !== "win32" }, async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-receipt-startup-boundary-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  const sinks = await createReceiptSinks(tempRoot);
  try {
    const receipt = { stage: "startup", outcome: "error", closedReason: "terminal_exit_code_1", startupBoundary: "program_run_error" };
    assert.deepEqual(await publishReceipt(sinks.helper, receipt), receipt);
    const apiReceipt = { stage: "startup", outcome: "error", closedReason: "terminal_exit_code_2", startupBoundary: "api_url_invalid" };
    assert.deepEqual(await publishReceipt(sinks.node, apiReceipt), apiReceipt);
    await assert.rejects(() => publishReceipt(sinks.node, { ...receipt, startupBoundary: "SENTINEL_SECRET" }), /invalid reconnect receipt/u);
  } finally {
    await closeReceiptSinks(sinks);
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("Windows native receipt acquisition denies replacement of the live root and its ancestors", { skip: process.platform !== "win32" }, async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-receipt-ancestor-replacement-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  try {
    const { stdout } = await new Promise((resolve, reject) => {
      execFile("python", [path.join(repoRoot, "scripts", "verify-release-asset-receipts.py"), "--base-root", tempRoot, "--replacement-self-test"], { windowsHide: true }, (error, stdout, stderr) => {
        if (error) reject(error); else resolve({ stdout, stderr });
      });
    });
    assert.deepEqual(JSON.parse(stdout), { ok: true, event: "replacement-self-test" });
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("npm preflight commands use fixed Windows cmd.exe npm.cmd boundaries", () => {
  assert.deepEqual(npmCommand("ci", "win32"), { program: "cmd.exe", args: ["/d", "/s", "/c", "npm.cmd ci"] });
  assert.deepEqual(npmCommand("build", "win32"), { program: "cmd.exe", args: ["/d", "/s", "/c", "npm.cmd run build"] });
  assert.deepEqual(npmCommand("ci", "linux"), { program: "npm", args: ["ci"] });
  assert.deepEqual(npmCommand("build", "linux"), { program: "npm", args: ["run", "build"] });
  assert.throws(() => npmCommand("test", "win32"), /unsupported npm preflight operation/u);
});

test("Windows source Core preparation sends both npm ci and npm run build through fixed cmd.exe boundaries", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-core-preflight-windows-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  const isolatedRoot = path.join(tempRoot, "core-source");
  const commands = [];
  const command = async (program, args) => {
    commands.push([program, args]);
    if (program === "git" && args.includes("rev-parse")) return { stdout: "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" };
    if (program === "cmd.exe" && args.join(" ") === "/d /s /c npm.cmd run build") {
      await Promise.all([
        mkdir(path.join(isolatedRoot, "packages", "core"), { recursive: true }),
        mkdir(path.join(isolatedRoot, "dist", "server"), { recursive: true }),
      ]);
      await Promise.all([
        writeFile(path.join(isolatedRoot, "package.json"), "{}"),
        writeFile(path.join(isolatedRoot, "packages", "core", "index.js"), ""),
        writeFile(path.join(isolatedRoot, "dist", "server", "index.js"), ""),
      ]);
    }
    return { stdout: "" };
  };
  try {
    await prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command, platform: "win32" });
    assert.deepEqual(commands.slice(-2), [
      ["cmd.exe", ["/d", "/s", "/c", "npm.cmd ci"]],
      ["cmd.exe", ["/d", "/s", "/c", "npm.cmd run build"]],
    ]);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("Windows can actually spawn npm.cmd through the fixed cmd.exe boundary", { skip: process.platform !== "win32" }, async () => {
  const { stdout } = await new Promise((resolve, reject) => {
    execFile("cmd.exe", ["/d", "/s", "/c", "npm.cmd --version"], { windowsHide: true }, (error, stdout, stderr) => {
      if (error) reject(error); else resolve({ stdout, stderr });
    });
  });
  assert.match(stdout.trim(), /^\d+\.\d+\.\d+$/u);
});

test("Windows npm ENOENT remains a closed dependency-install preflight failure", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-core-preflight-npm-enoent-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  const isolatedRoot = path.join(tempRoot, "core-source");
  const command = async (program, args) => {
    if (program === "git" && args.includes("rev-parse")) return { stdout: "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" };
    if (program === "cmd.exe" && args.join(" ") === "/d /s /c npm.cmd ci") {
      const error = new Error("spawn cmd.exe ENOENT");
      error.code = "ENOENT";
      throw error;
    }
    return { stdout: "" };
  };
  try {
    await assert.rejects(
      () => prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command, platform: "win32" }),
      error => error?.reason === "dependency_install_failed" && error.message === "dependency_install_failed",
    );
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("Core preflight returns closed reason categories without exposing command output", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-core-preflight-reason-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  const command = async (program, args) => {
    if (program === "git" && args.includes("rev-parse")) return { stdout: "10e4d72b75c66977ad1dd629991a27443ffc0fd3\n" };
    if (program === "git" && args.includes("fetch")) throw new Error("sensitive command output must not escape");
    return { stdout: "" };
  };
  try {
    await assert.rejects(
      () => prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command }),
      error => error?.reason === "source_clone_failed" && error.message === "source_clone_failed",
    );
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("fault cleanup requires the helper's explicit close acknowledgement and never addresses a PID", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-cooperative-cleanup-${process.pid}-${Date.now()}`);
  const request = path.join(tempRoot, "request");
  const acknowledgement = path.join(tempRoot, "acknowledgement");
  await mkdir(tempRoot, { recursive: true });
  const child = { pid: 4242, exitCode: null, kill: () => assert.fail("cleanup must not address a helper PID") };
  try {
    assert.equal(await stopProbe({
      child,
      shutdownRequestPath: request,
      shutdownAcknowledgementPath: acknowledgement,
      shutdownToken: "per-probe-capability",
      exited: Promise.resolve(),
      completedSuccessfully: false,
    }, {
      waitForAck: async file => {
        assert.equal(file, acknowledgement);
        assert.equal(await readFile(request, "utf8"), "per-probe-capability\n");
        await writeFile(acknowledgement, "per-probe-capability\n", { flag: "wx" });
        child.exitCode = 1;
        return true;
      },
    }), true);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("helper exit or PID reuse without acknowledgement fails cleanup closed", async () => {
  assert.equal(await stopProbe({
    child: { pid: 4242, exitCode: 1 },
    completedSuccessfully: false,
  }), false);
});

test("a mismatched acknowledgement is not cleanup proof", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-cooperative-cleanup-mismatch-${process.pid}-${Date.now()}`);
  const request = path.join(tempRoot, "request");
  const acknowledgement = path.join(tempRoot, "acknowledgement");
  await mkdir(tempRoot, { recursive: true });
  try {
    await writeFile(acknowledgement, "unexpected\n", { flag: "wx" });
    assert.equal(await stopProbe({
      child: { exitCode: null }, shutdownRequestPath: request, shutdownAcknowledgementPath: acknowledgement, shutdownToken: "expected", exited: Promise.resolve(),
    }), false);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("cleanup unconfirmed retains the extracted root and preserves the primary failure", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-cleanup-retained-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  try {
    assert.equal(await cleanupResources({ probe: { child: { exitCode: null } }, tempRoot }, { stop: async () => false }), false);
    await access(tempRoot);
    const primaryFailure = new Error("core-start");
    assert.equal(cleanupOutcome(primaryFailure, false), primaryFailure);
    assert.match(cleanupOutcome(undefined, false)?.message ?? "", /^cleanup_unconfirmed$/u);
  } finally {
    await rm(tempRoot, { recursive: true, force: true });
  }
});

test("native-owned receipt roots are retained instead of path-recursive deletion", async () => {
  let removed = false;
  assert.equal(await cleanupResources({ tempRoot: "native-root", retainTempRoot: true }, { remove: async () => { removed = true; } }), true);
  assert.equal(removed, false);
});

test("reconnect receipts are closed schema and reject terminal sentinel text", () => {
  assert.deepEqual(validateReceipt({ stage: "reconnect", outcome: "timeout", closedReason: "timed_out" }), { stage: "reconnect", outcome: "timeout", closedReason: "timed_out" });
  assert.deepEqual(validateReceipt({ stage: "startup", outcome: "error", closedReason: "terminal_exit_code_1", startupBoundary: "program_run_error" }), { stage: "startup", outcome: "error", closedReason: "terminal_exit_code_1", startupBoundary: "program_run_error" });
  assert.deepEqual(validateReceipt({ stage: "startup", outcome: "error", closedReason: "terminal_exit_code_2", startupBoundary: "api_url_invalid" }), { stage: "startup", outcome: "error", closedReason: "terminal_exit_code_2", startupBoundary: "api_url_invalid" });
  assert.throws(() => validateReceipt({ stage: "startup", outcome: "error", closedReason: "terminal_closed" }), /invalid reconnect receipt/u);
  assert.throws(() => validateReceipt({ stage: "startup", outcome: "error", closedReason: "terminal_exit_code_1", startupBoundary: "SENTINEL_SECRET" }), /invalid reconnect receipt/u);
  assert.throws(() => validateReceipt({ stage: "reconnect", outcome: "error", closedReason: "terminal_exit_code_1", startupBoundary: "program_run_error" }), /invalid reconnect receipt/u);
  assert.throws(() => validateReceipt({ stage: "reconnect", outcome: "timeout", closedReason: "timed_out", terminal: "SENTINEL_SECRET" }), /invalid reconnect receipt/u);
  assert.throws(() => validateReceipt({ stage: "reconnect", outcome: "SENTINEL_SECRET", closedReason: "timed_out" }), /invalid reconnect receipt/u);
});

test("probe success rejects extra or incomplete terminal metadata", async () => {
  const receipt = { stage: "exit", outcome: "normal", closedReason: "completed" };
  const success = JSON.stringify({ ok: true, mode: "reconnect", reconnect: "r", navigation: ["d", "?"], narrowResize: "help-screen-rendered-after-50-columns", exit: "q", receipt });
  assert.deepEqual(await finalizeReconnectExit({ helper: { writeFile: async () => undefined, sync: async () => undefined }, node: { writeFile: async () => undefined, sync: async () => undefined } }, 0, null, success), JSON.parse(success));
  const foreign = JSON.stringify({ ok: true, mode: "reconnect", reconnect: "r", navigation: ["d", "?"], narrowResize: "help-screen-rendered-after-50-columns", exit: "q", receipt, terminal: "SENTINEL_SECRET" });
  await assert.rejects(() => finalizeReconnectExit({ helper: { writeFile: async () => undefined, sync: async () => undefined }, node: { writeFile: async () => undefined, sync: async () => undefined } }, 0, null, foreign), /bounded assertions/u);
});

test("probe parser closes unavailable and failure schemas as well as reconnect success", () => {
  const candidateIdentity = { sourceCommit: "0123456789abcdef0123456789abcdef01234567", binarySHA256: "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789" };
  const unavailable = { ok: true, mode: "unavailable", exit: "q", receipt: { stage: "exit", outcome: "normal", closedReason: "completed", candidateIdentity } };
  assert.deepEqual(parseProbe(JSON.stringify(unavailable), "unavailable"), unavailable);
  assert.throws(() => parseProbe(JSON.stringify({ ok: true, mode: "unavailable", exit: "q", terminal: "SENTINEL_SECRET" }), "unavailable"), /bounded assertions/u);
  assert.throws(() => parseProbe(JSON.stringify({ ok: false, stage: "startup", receipt: { stage: "startup", outcome: "timeout", closedReason: "timed_out" }, terminal: "SENTINEL_SECRET" }), "reconnect"), /bounded assertions/u);
  assert.throws(() => parseProbe(JSON.stringify({ ok: false, stage: "startup", receipt: { stage: "exit", outcome: "timeout", closedReason: "timed_out" } }), "reconnect"), /bounded assertions/u);
  assert.throws(() => parseProbe(JSON.stringify({ ok: false, stage: "startup", receipt: { stage: "startup", outcome: "error", closedReason: "terminal_exit_code_1", startupBoundary: "program_run_error", terminal: "SENTINEL_SECRET" } }), "reconnect"), /bounded assertions/u);
});

test("held receipt handles bind publication despite attempt-root substitution", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-reconnect-receipt-path-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  try {
    const sinks = await createReceiptSinks(tempRoot);
    const displacedRoot = `${tempRoot}-owned`;
    await rm(displacedRoot, { recursive: true, force: true });
    const { rename } = await import("node:fs/promises");
    if (process.platform === "win32") {
      await assert.rejects(() => rename(tempRoot, displacedRoot), /EPERM|EACCES|EBUSY/u);
    } else {
      await rename(tempRoot, displacedRoot);
    }
    assert.deepEqual(await persistNodeExitReceipt(sinks.node, 1, null), { stage: "helper-exit", outcome: "error", closedReason: "helper_exit_nonzero" });
    await closeReceiptSinks(sinks);
    const receiptDirectory = process.platform === "win32" ? sinks.root : path.join(displacedRoot, path.basename(sinks.root));
    assert.deepEqual(JSON.parse(await readFile(path.join(receiptDirectory, "node-exit-outcome.json"), "utf8")), { stage: "helper-exit", outcome: "error", closedReason: "helper_exit_nonzero" });
  } finally {
    if (process.platform !== "win32") {
      await rm(tempRoot, { recursive: true, force: true });
      await rm(`${tempRoot}-owned`, { recursive: true, force: true });
    }
  }
});

test("Windows CI executes the pinned Python ConPTY helper against a safe unavailable endpoint", async () => {
  const workflow = await readFile(path.join(repoRoot, ".github", "workflows", "ci.yml"), "utf8");
  assert.match(workflow, /actions\/setup-python@v6[\s\S]*?python-version: '3\.14'/u);
  assert.match(workflow, /GOFLAGS: ""[\s\S]*?GOWORK: "off"[\s\S]*?node scripts\/assert-go-source-provenance\.mjs/u);
  assert.match(workflow, /go build -mod=readonly -buildvcs=true -o \$binary/u);
  assert.match(workflow, /\$metadata = \(go version -m \$binary\) -join "`n"/u);
  assert.match(workflow, /python -m pip install --require-hashes --only-binary=:all: --no-deps -r scripts\/requirements-conpty\.txt/u);
  assert.match(workflow, /verify-release-asset-conpty\.py --executable \$binary --mode unavailable --api-url http:\/\/127\.0\.0\.1:1/u);
});

test("candidate packaging re-verifies every extracted executable VCS identity", async () => {
  const workflow = await readFile(path.join(repoRoot, ".github", "workflows", "release.yml"), "utf8");
  assert.match(workflow, /"win32-amd64 zip service-lasso-tui\.exe"[\s\S]*?"linux-amd64 tar service-lasso-tui"[\s\S]*?"darwin-amd64 tar service-lasso-tui"[\s\S]*?"darwin-arm64 tar service-lasso-tui"/u);
  assert.match(workflow, /go version -m "\$extraction_root\/\$executable" \| grep -F "vcs\.revision=\$CANDIDATE_SHA"/u);
  assert.match(workflow, /go version -m "\$extraction_root\/\$executable" \| grep -F 'vcs\.modified=false'/u);
});
