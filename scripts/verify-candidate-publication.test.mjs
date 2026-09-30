import assert from "node:assert/strict";
import test from "node:test";
import { assertExistingCandidateRecovery, assertManifest, assertPreflight, assertReleaseReceipt, assertTransportPolicy, candidateIdentity } from "./verify-candidate-publication.mjs";

const identity = candidateIdentity({ sourceRef: "refs/heads/develop", sourceCommit: "0123456789abcdef0123456789abcdef01234567", version: "2026.10.1-0123456", tag: "candidate-2026.10.1-0123456" });
const digest = "a".repeat(64);
const names = ["service-lasso-tui-2026.10.1-0123456-win32-amd64.zip", "service-lasso-tui-2026.10.1-0123456-linux-amd64.tar.gz", "service-lasso-tui-2026.10.1-0123456-darwin-amd64.tar.gz", "service-lasso-tui-2026.10.1-0123456-darwin-arm64.tar.gz"];
const manifest = { schemaVersion: 2, kind: "develop-prerelease-candidate", source: { repository: "service-lasso/service-lasso-tui", ref: identity.sourceRef, commit: identity.sourceCommit }, release: { tag: identity.tag, prerelease: true, draft: false, immutable: true }, version: identity.version, checksumManifest: { name: "SHA256SUMS.txt", sha256: digest }, corePackagingIssue: "service-lasso/service-lasso#1461", assets: [{ platform: "win32-amd64", name: names[0], sha256: digest, executable: "service-lasso-tui.exe" }, { platform: "linux-amd64", name: names[1], sha256: digest, executable: "service-lasso-tui" }, { platform: "darwin-amd64", name: names[2], sha256: digest, executable: "service-lasso-tui" }, { platform: "darwin-arm64", name: names[3], sha256: digest, executable: "service-lasso-tui" }] };
const localAssets = Object.fromEntries([...names, "SHA256SUMS.txt", "candidate-manifest.json"].map(name => [name, digest]));
const release = { tag_name: identity.tag, target_commitish: identity.sourceCommit, draft: false, prerelease: true, immutable: true, assets: Object.keys(localAssets).map(name => ({ name, digest: `sha256:${digest}`, size: 1 })) };
const preflight = { immutableReleases: { enabled: true }, environment: { name: "development-candidate", protection_rules: [{ type: "wait_timer", wait_timer: 10 }], deployment_branch_policy: { protected_branches: true, custom_branch_policies: false } }, branchProtection: { required_status_checks: { strict: true, contexts: ["Linux test and build", "Windows test and build", "macOS test and build", "Release asset cross-compilation"] }, required_pull_request_reviews: { required_approving_review_count: 1 } } };

test("accepts a complete protected preflight", () => assert.equal(assertPreflight(preflight).environment, "development-candidate"));
test("rejects disabled immutable releases and unbounded environment waits", () => {
  assert.throws(() => assertPreflight({ ...preflight, immutableReleases: { enabled: false } }), /immutable/u);
  assert.throws(() => assertPreflight({ ...preflight, environment: { ...preflight.environment, protection_rules: [{ type: "wait_timer", wait_timer: 31 }] } }), /bounded/u);
});
test("rejects unprotected develop and missing current CI checks", () => {
  assert.throws(() => assertPreflight({ ...preflight, environment: { ...preflight.environment, deployment_branch_policy: { protected_branches: false, custom_branch_policies: false } } }), /restricted/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, required_status_checks: { strict: true, contexts: ["Linux test and build"] } } }), /CI/u);
});
test("requires a full-SHA-bound four-platform candidate manifest", () => {
  assert.doesNotThrow(() => assertManifest(manifest, identity));
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.slice(1) }, identity), /all platform/u);
  assert.throws(() => candidateIdentity({ ...identity, sourceRef: "refs/heads/main" }), /develop/u);
});
test("readback accepts only an immutable complete byte receipt", () => {
  assert.equal(assertReleaseReceipt(release, manifest, localAssets).immutable, true);
  assert.equal(assertExistingCandidateRecovery(release, manifest, localAssets).tag, identity.tag);
  assert.throws(() => assertReleaseReceipt({ ...release, immutable: false }, manifest, localAssets), /immutable/u);
  assert.throws(() => assertReleaseReceipt({ ...release, assets: release.assets.slice(1) }, manifest, localAssets), /inventory/u);
  assert.throws(() => assertReleaseReceipt({ ...release, assets: release.assets.map(asset => asset.name === names[0] ? { ...asset, digest: `sha256:${"b".repeat(64)}` } : asset) }, manifest, localAssets), /receipt/u);
});
test("keeps authenticated uploads distinct from approved headerless downloads", () => {
  assert.equal(assertTransportPolicy({ uploadURL: "https://uploads.github.com/repos/service-lasso/service-lasso-tui/releases/1/assets", downloadURL: "https://github.com/service-lasso/service-lasso-tui/releases/download/tag/file", authorization: "Bearer" }), true);
  assert.throws(() => assertTransportPolicy({ uploadURL: "https://uploads.github.com/x", downloadURL: "https://evil.example/file", authorization: "Bearer" }), /download host/u);
  assert.throws(() => assertTransportPolicy({ uploadURL: "https://uploads.github.com/x", downloadURL: "https://github.com/file", authorization: "Basic" }), /Bearer/u);
});
