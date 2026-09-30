import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import test from "node:test";
import { access, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { assertCandidateManifest, cleanupOutcome, cleanupResources, npmCommand, parseArgs, prepareCoreRuntime, prepareSourceBuiltCore, stopProbe } from "./verify-release-asset-conpty.mjs";

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");

const manifest = { schemaVersion: 1, kind: "develop-prerelease-candidate", source: { repository: "service-lasso/service-lasso-tui", ref: "refs/heads/develop", commit: "97fafb04c69fce8efdd245eb186e6dfb9915485d" }, release: { tag: "candidate-2026.9.30-97fafb0", prerelease: true }, version: "2026.9.30-97fafb0", checksumManifest: { name: "SHA256SUMS.txt", sha256: "638ad5e54e06dcb894a4579872e788ddc4521cd3ffca46fb06574c8b78de1cf5" }, assets: [{ platform: "darwin-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-darwin-amd64.tar.gz", sha256: "5d6df8cfa18771e159b7af34c1c5c00d70ae5eef1ddf727896ae2afb30b63638", executable: "service-lasso-tui" }, { platform: "darwin-arm64", name: "service-lasso-tui-2026.9.30-97fafb0-darwin-arm64.tar.gz", sha256: "a9523555416a1b332107a9a92cfecc2aa3b7cdd23caab1063540496e4e20b33c", executable: "service-lasso-tui" }, { platform: "linux-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-linux-amd64.tar.gz", sha256: "238a8e3f92ae5f9cf28c5cd29af91b698addb9bfa546a7b31c7f7a71b7c33e70", executable: "service-lasso-tui" }, { platform: "win32-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-win32-amd64.zip", sha256: "b8838f245d4b1d39cac0b51ed2ad14ffd0237779f1a5e9d3e61358066370e479", executable: "service-lasso-tui.exe" }] };

test("accepts only the pinned Windows candidate manifest binding", () => assert.doesNotThrow(() => assertCandidateManifest(manifest)));
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

test("source Core preparation clones, pins, installs, and builds an isolated runtime before use", async () => {
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
      ["git", ["clone", "--no-local", "--no-checkout", "supplied-core", isolatedRoot]],
      ["git", ["-C", isolatedRoot, "checkout", "--detach", "10e4d72b75c66977ad1dd629991a27443ffc0fd3"]],
      ["git", ["-C", isolatedRoot, "rev-parse", "HEAD"]],
      ["npm", ["ci"]],
      ["npm", ["run", "build"]],
    ]);
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
    if (program === "git" && args[0] === "clone") throw new Error("sensitive command output must not escape");
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

test("Windows CI executes the pinned Python ConPTY helper against a safe unavailable endpoint", async () => {
  const workflow = await readFile(path.join(repoRoot, ".github", "workflows", "ci.yml"), "utf8");
  assert.match(workflow, /actions\/setup-python@v6[\s\S]*?python-version: '3\.14'/u);
  assert.match(workflow, /python -m pip install --require-hashes --only-binary=:all: --no-deps -r scripts\/requirements-conpty\.txt/u);
  assert.match(workflow, /verify-release-asset-conpty\.py --executable service-lasso-tui\.exe --mode unavailable --api-url http:\/\/127\.0\.0\.1:1/u);
});
