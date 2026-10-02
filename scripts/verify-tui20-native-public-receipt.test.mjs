import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile } from "node:fs/promises";
import { spawnSync } from "node:child_process";
import { tmpdir } from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const script = fileURLToPath(new URL("./verify-tui20-native-public-receipt.mjs", import.meta.url));
const hash = "a".repeat(64), otherHash = "c".repeat(64), commit = "b".repeat(40), otherCommit = "d".repeat(40);
const records = () => ({
  "binary-digest.json": { schemaVersion: 1, kind: "tui20-native-binary-digest", sha256: hash, size: 1 },
  "input-custody.json": { schemaVersion: 1, kind: "tui20-native-input-custody", source: { tuiCommit: commit, tuiTree: hash, tuiDirtyHash: hash, tuiInventoryHash: hash }, core: { requestedCommit: commit }, ownership: { threeDistinctPaths: true, allParentsNonLink: true, registriesInitiallyAbsent: true }, verification: { freshEnvironment: true, requiredToolsVerified: ["git", "go"] } },
  "core-source-binding.json": { schemaVersion: 1, kind: "tui20-native-core-source-binding", coreCommit: commit, coreTree: hash, coreDirtyHash: hash },
  "build-output.json": { schemaVersion: 1, kind: "tui20-native-build-output", tuiCommit: commit, nativeBinary: { sha256: hash, size: 1 } },
  "native-public-projection.json": { schemaVersion: 1, kind: "tui20-native-public-result", sourceCommit: commit, binarySHA256: hash, result: "succeeded", actionsPassed: true, ownedRuntimeClosed: true }
});
async function writeReceipt(root, mutate = value => value, phaseName = "phase") { const phase = path.join(root, phaseName); await mkdir(phase, { recursive: true }); for (const [name, value] of Object.entries(records())) await writeFile(path.join(phase, name), JSON.stringify(mutate(structuredClone(value), name))); return phase; }
function verify(root) { return spawnSync(process.execPath, [script, root]); }
async function rejects(mutate, expression) { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { await writeReceipt(root, mutate); const result = verify(root); assert.notEqual(result.status, 0); assert.match(result.stderr.toString(), expression); } finally { await rm(root, { recursive: true, force: true }); } }

test("native public receipt allows one fully-bound closed set per phase", async () => { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { await writeReceipt(root); await writeReceipt(root, value => value, "failed-phase"); const result = verify(root); assert.equal(result.status, 0, result.stderr.toString()); } finally { await rm(root, { recursive: true, force: true }); } });
test("native public receipt rejects private paths, PIDs, and birth evidence", () => rejects((value, name) => name === "build-output.json" ? { ...value, privatePath: "/runner/private" } : value, /forbidden public key/));
test("native public receipt rejects mixed TUI commits", () => rejects((value, name) => name === "build-output.json" ? { ...value, tuiCommit: otherCommit } : value, /TUI commit mismatch/));
test("native public receipt rejects mixed Core commits", () => rejects((value, name) => name === "core-source-binding.json" ? { ...value, coreCommit: otherCommit } : value, /Core commit mismatch/));
test("native public receipt rejects mixed executable identities", () => rejects((value, name) => name === "native-public-projection.json" ? { ...value, binarySHA256: otherHash } : value, /native executable identity mismatch/));
test("native public receipt rejects a missing sibling record", async () => { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { const phase = await writeReceipt(root); await rm(path.join(phase, "build-output.json")); const result = verify(root); assert.notEqual(result.status, 0); assert.match(result.stderr.toString(), /set is incomplete/); } finally { await rm(root, { recursive: true, force: true }); } });
test("native public receipt rejects a duplicate record in a separate sibling directory", async () => { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { await writeReceipt(root); await mkdir(path.join(root, "duplicate")); await writeFile(path.join(root, "duplicate", "binary-digest.json"), JSON.stringify(records()["binary-digest.json"])); const result = verify(root); assert.notEqual(result.status, 0); assert.match(result.stderr.toString(), /set is incomplete/); } finally { await rm(root, { recursive: true, force: true }); } });
test("native public receipt rejects a foreign public receipt record", async () => { const root = await mkdtemp(path.join(tmpdir(), "tui20-public-")); try { const phase = await writeReceipt(root); await writeFile(path.join(phase, "foreign.json"), JSON.stringify(records()["binary-digest.json"])); const result = verify(root); assert.notEqual(result.status, 0); assert.match(result.stderr.toString(), /foreign native public receipt/); } finally { await rm(root, { recursive: true, force: true }); } });
