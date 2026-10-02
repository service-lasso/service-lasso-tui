import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile } from "node:fs/promises";
import { spawnSync } from "node:child_process";
import { tmpdir } from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const script = fileURLToPath(new URL("./verify-tui20-native-public-receipt.mjs", import.meta.url));
const hash = "a".repeat(64), commit = "b".repeat(40);
const records = () => ({
  "binary-digest.json": { schemaVersion: 1, kind: "tui20-native-binary-digest", sha256: hash, size: 1 },
  "input-custody.json": { schemaVersion: 1, kind: "tui20-native-input-custody", source: { tuiCommit: commit }, core: { requestedCommit: commit }, ownership: { threeDistinctPaths: true }, verification: { freshEnvironment: true } },
  "core-source-binding.json": { schemaVersion: 1, kind: "tui20-native-core-source-binding", coreCommit: commit, coreTree: hash, coreDirtyHash: hash },
  "build-output.json": { schemaVersion: 1, kind: "tui20-native-build-output", tuiCommit: commit, nativeBinary: { sha256: hash, size: 1 } },
  "native-public-projection.json": { schemaVersion: 1, kind: "tui20-native-public-result", sourceCommit: commit, binarySHA256: hash, result: "succeeded", actionsPassed: true, ownedRuntimeClosed: true }
});
async function writeReceipt(root, mutate = value => value) { const phase = path.join(root, "phase"); await mkdir(phase); for (const [name, value] of Object.entries(records())) await writeFile(path.join(phase, name), JSON.stringify(mutate(value, name))); }
test("native public receipt allows only its closed metadata schemas", async () => { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { await writeReceipt(root); const result = spawnSync(process.execPath, [script, root]); assert.equal(result.status, 0, result.stderr.toString()); } finally { await rm(root, { recursive: true, force: true }); } });
test("native public receipt rejects private paths, PIDs, and birth evidence", async () => { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { await writeReceipt(root, (value, name) => name === "build-output.json" ? { ...value, privatePath: "/runner/private" } : value); const result = spawnSync(process.execPath, [script, root]); assert.notEqual(result.status, 0); assert.match(result.stderr.toString(), /forbidden public key/); } finally { await rm(root, { recursive: true, force: true }); } });
