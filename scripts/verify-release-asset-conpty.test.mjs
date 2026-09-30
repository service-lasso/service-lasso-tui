import assert from "node:assert/strict";
import test from "node:test";
import { assertCandidateManifest } from "./verify-release-asset-conpty.mjs";

const manifest = { schemaVersion: 1, kind: "develop-prerelease-candidate", source: { repository: "service-lasso/service-lasso-tui", ref: "refs/heads/develop", commit: "97fafb04c69fce8efdd245eb186e6dfb9915485d" }, release: { tag: "candidate-2026.9.30-97fafb0", prerelease: true }, version: "2026.9.30-97fafb0", checksumManifest: { name: "SHA256SUMS.txt", sha256: "638ad5e54e06dcb894a4579872e788ddc4521cd3ffca46fb06574c8b78de1cf5" }, assets: [{ platform: "win32-amd64", name: "service-lasso-tui-2026.9.30-97fafb0-win32-amd64.zip", sha256: "b8838f245d4b1d39cac0b51ed2ad14ffd0237779f1a5e9d3e61358066370e479", executable: "service-lasso-tui.exe" }] };

test("accepts only the pinned Windows candidate manifest binding", () => assert.doesNotThrow(() => assertCandidateManifest(manifest)));
test("rejects a mutable or mismatched Windows archive binding", () => {
  const changed = structuredClone(manifest); changed.assets[0].sha256 = "0".repeat(64);
  assert.throws(() => assertCandidateManifest(changed), /candidate manifest/);
});
