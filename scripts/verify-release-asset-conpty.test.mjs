import assert from "node:assert/strict";
import test from "node:test";
import { mkdir, readFile, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { assertCandidateManifest, parseArgs, prepareSourceBuiltCore, verifyPackagedCore } from "./verify-release-asset-conpty.mjs";

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

test("defaults to isolated source-built Core and accepts packaged Core only when explicitly selected", () => {
  assert.deepEqual(parseArgs(["--core-root", "core"]), { coreRoot: path.resolve("core"), coreKind: "source-built" });
  assert.deepEqual(parseArgs(["--core-kind", "packaged", "--core-root", "core"]), { coreRoot: path.resolve("core"), coreKind: "packaged" });
  assert.throws(() => parseArgs(["--core-kind", "source"]), /usage/);
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
    const prepared = await prepareSourceBuiltCore({ coreRoot: "supplied-core", tempRoot, command });
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

test("packaged Core mode rejects a missing runtime dist instead of falling back to source", async () => {
  const tempRoot = path.join(os.tmpdir(), `tui-packaged-core-${process.pid}-${Date.now()}`);
  await mkdir(tempRoot, { recursive: true });
  try {
    await mkdir(path.join(tempRoot, "packages", "core"), { recursive: true });
    await Promise.all([
      writeFile(path.join(tempRoot, "package.json"), "{}"),
      writeFile(path.join(tempRoot, "packages", "core", "index.js"), ""),
    ]);
    await assert.rejects(() => verifyPackagedCore({ coreRoot: tempRoot }), /ENOENT/);
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
