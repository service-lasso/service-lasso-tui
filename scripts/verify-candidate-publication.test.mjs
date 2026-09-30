import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { mkdtemp } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { assertExistingCandidateRecovery, assertManifest, assertPreflight, assertReleaseReceipt, assertTransportPolicy, candidateIdentity, verifyPublicAssetBytes } from "./verify-candidate-publication.mjs";

const identity = candidateIdentity({ sourceRef: "refs/heads/develop", sourceCommit: "0123456789abcdef0123456789abcdef01234567", version: "2026.10.1-0123456", tag: "candidate-2026.10.1-0123456" });
const names = ["service-lasso-tui-2026.10.1-0123456-win32-amd64.zip", "service-lasso-tui-2026.10.1-0123456-linux-amd64.tar.gz", "service-lasso-tui-2026.10.1-0123456-darwin-amd64.tar.gz", "service-lasso-tui-2026.10.1-0123456-darwin-arm64.tar.gz", "SHA256SUMS.txt", "candidate-manifest.json"];
const bodyFor = name => Buffer.from(`published-${name}`, "utf8");
const digest = bytes => createHash("sha256").update(bytes).digest("hex");
const manifest = { schemaVersion: 2, kind: "develop-prerelease-candidate", source: { repository: "service-lasso/service-lasso-tui", ref: identity.sourceRef, commit: identity.sourceCommit }, release: { tag: identity.tag, prerelease: true, draft: false, immutable: true }, version: identity.version, checksumManifest: { name: "SHA256SUMS.txt", sha256: digest(bodyFor("SHA256SUMS.txt")) }, corePackagingIssue: "service-lasso/service-lasso#1461", assets: [{ platform: "win32-amd64", name: names[0], sha256: digest(bodyFor(names[0])), executable: "service-lasso-tui.exe" }, { platform: "linux-amd64", name: names[1], sha256: digest(bodyFor(names[1])), executable: "service-lasso-tui" }, { platform: "darwin-amd64", name: names[2], sha256: digest(bodyFor(names[2])), executable: "service-lasso-tui" }, { platform: "darwin-arm64", name: names[3], sha256: digest(bodyFor(names[3])), executable: "service-lasso-tui" }] };
const localAssets = Object.fromEntries(names.map(name => { const body = bodyFor(name); return [name, { sha256: digest(body), size: body.length }]; }));
const release = { tag_name: identity.tag, target_commitish: identity.sourceCommit, draft: false, prerelease: true, immutable: true, assets: names.map(name => ({ name, digest: `sha256:${localAssets[name].sha256}`, size: localAssets[name].size, browser_download_url: `https://github.com/service-lasso/service-lasso-tui/releases/download/${identity.tag}/${name}` })) };
const preflight = { immutableReleases: { enabled: true }, environment: { name: "development-candidate", protection_rules: [{ type: "wait_timer", wait_timer: 10 }], deployment_branch_policy: { protected_branches: true, custom_branch_policies: false } }, branchProtection: { required_status_checks: { strict: true, contexts: ["Linux test and build", "Windows test and build", "macOS test and build", "Release asset cross-compilation"] }, required_pull_request_reviews: { required_approving_review_count: 0 } } };

function response(status, headers = {}, body) { return { status, headers: { get: key => headers[key.toLowerCase()] ?? null }, body: body === undefined ? undefined : (async function* () { yield body; })() }; }
function publicFetch({ bodies = Object.fromEntries(names.map(name => [name, bodyFor(name)])), redirect = true, calls = [] } = {}) {
  return async (url, options) => {
    calls.push({ url: String(url), options }); const parsed = new URL(url);
    if (parsed.hostname === "github.com") {
      const name = decodeURIComponent(parsed.pathname.split("/").at(-1));
      if (!bodies[name]) return response(404);
      if (redirect) return response(302, { location: `https://release-assets.githubusercontent.com/public/${encodeURIComponent(name)}?signature=bounded` });
      return response(200, { "content-length": String(bodies[name].length) }, bodies[name]);
    }
    const name = decodeURIComponent(parsed.pathname.split("/").at(-1));
    return response(200, { "content-length": String(bodies[name].length) }, bodies[name]);
  };
}
async function verify(overrides = {}) { return verifyPublicAssetBytes({ release: overrides.release ?? release, manifest, localAssets: overrides.localAssets ?? localAssets, downloadDir: await mkdtemp(path.join(os.tmpdir(), "candidate-public-bytes-")), fetchImpl: overrides.fetchImpl ?? publicFetch(overrides) }); }

test("accepts a complete protected preflight without inventing a review count", () => assert.equal(assertPreflight(preflight).environment, "development-candidate"));
test("rejects disabled immutable releases and unbounded environment waits", () => {
  assert.throws(() => assertPreflight({ ...preflight, immutableReleases: { enabled: false } }), /immutable/u);
  assert.throws(() => assertPreflight({ ...preflight, environment: { ...preflight.environment, protection_rules: [{ type: "wait_timer", wait_timer: 31 }] } }), /bounded/u);
});
test("requires a full-SHA-bound four-platform candidate manifest", () => {
  assert.doesNotThrow(() => assertManifest(manifest, identity));
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.slice(1) }, identity), /all platform/u);
  assert.throws(() => candidateIdentity({ ...identity, sourceRef: "refs/heads/main" }), /develop/u);
});
test("validates complete immutable metadata inventory before reading public bytes", () => {
  assert.equal(assertReleaseReceipt(release, manifest, localAssets).immutable, true);
  assert.equal(assertExistingCandidateRecovery(release, manifest, localAssets).tag, identity.tag);
  assert.throws(() => assertReleaseReceipt({ ...release, immutable: false }, manifest, localAssets), /immutable/u);
  assert.throws(() => assertReleaseReceipt({ ...release, assets: release.assets.slice(1) }, manifest, localAssets), /inventory/u);
});
test("recovers a complete existing candidate only after all six public bodies match", async () => {
  const calls = []; const receipt = await verify({ calls });
  assert.equal(receipt.verified.length, 6); assert.equal(receipt.totalBytes, names.reduce((sum, name) => sum + localAssets[name].size, 0));
  assert.ok(calls.every(call => call.options.method === "GET" && call.options.redirect === "manual" && Object.keys(call.options.headers).length === 0));
  assert.ok(calls.some(call => new URL(call.url).hostname === "release-assets.githubusercontent.com"));
});
test("rejects public body mismatch, truncation, missing inventory, and over-bound bodies", async () => {
  const mismatched = { ...Object.fromEntries(names.map(name => [name, bodyFor(name)])), [names[0]]: Buffer.from("x".repeat(bodyFor(names[0]).length)) };
  await assert.rejects(() => verify({ bodies: mismatched }), /byte receipt/u);
  const truncated = { ...Object.fromEntries(names.map(name => [name, bodyFor(name)])), [names[1]]: bodyFor(names[1]).subarray(0, -1) };
  await assert.rejects(() => verify({ bodies: truncated }), /body size|truncated/u);
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.slice(1) } }), /inventory/u);
  const oversized = { ...Object.fromEntries(names.map(name => [name, bodyFor(name)])), [names[2]]: Buffer.concat([bodyFor(names[2]), Buffer.from("more")]) };
  await assert.rejects(() => verify({ bodies: oversized }), /body size|exceeds/u);
});
test("rejects malformed public URLs and redirect escapes or loops", async () => {
  for (const url of ["http://github.com/x", "https://user@github.com/x", "https://github.com:443/x", "https://github.com/x#fragment", "https://github.com/x?query=1"]) assert.throws(() => assertTransportPolicy({ uploadURL: "https://uploads.github.com/x", downloadURL: url, authorization: "Bearer" }));
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.map(asset => asset.name === names[0] ? { ...asset, browser_download_url: "https://evil.example/file" } : asset) } }), /download host/u);
  await assert.rejects(() => verify({ fetchImpl: async () => response(302, { location: "https://evil.example/file" }) }), /download host/u);
  await assert.rejects(() => verify({ fetchImpl: async () => response(302, { location: "https://release-assets.githubusercontent.com:443/file" }) }), /download host/u);
  await assert.rejects(() => verify({ fetchImpl: async url => response(302, { location: String(url) }) }), /redirect limit/u);
});
test("keeps authenticated uploads separate from headerless public downloads", () => {
  assert.equal(assertTransportPolicy({ uploadURL: "https://uploads.github.com/repos/service-lasso/service-lasso-tui/releases/1/assets", downloadURL: "https://github.com/service-lasso/service-lasso-tui/releases/download/tag/file", authorization: "Bearer" }), true);
  assert.throws(() => assertTransportPolicy({ uploadURL: "https://uploads.github.com/x", downloadURL: "https://github.com/file", authorization: "Basic" }), /Bearer/u);
});
