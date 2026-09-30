import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { mkdtemp, rename, rm, symlink, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { assertExistingCandidateRecovery, assertManifest, assertPreflight, assertPublicationDirectory, assertReleaseReceipt, assertTransportPolicy, candidateIdentity, readBoundedRegularLocalAsset, verifyPublicAssetBytes } from "./verify-candidate-publication.mjs";
import { uploadVerifiedCandidateAssets } from "./upload-verified-candidate-assets.mjs";

const identity = candidateIdentity({ sourceRef: "refs/heads/develop", sourceCommit: "0123456789abcdef0123456789abcdef01234567", version: "2026.10.1-0123456", tag: "candidate-2026.10.1-0123456" });
const names = ["service-lasso-tui-2026.10.1-0123456-win32-amd64.zip", "service-lasso-tui-2026.10.1-0123456-linux-amd64.tar.gz", "service-lasso-tui-2026.10.1-0123456-darwin-amd64.tar.gz", "service-lasso-tui-2026.10.1-0123456-darwin-arm64.tar.gz", "SHA256SUMS.txt", "candidate-manifest.json"];
const digest = bytes => createHash("sha256").update(bytes).digest("hex");
const bodyFor = name => name === "SHA256SUMS.txt"
  ? Buffer.from(names.slice(0, 4).map(asset => `${digest(bodyFor(asset))}  ${asset}`).join("\n") + "\n", "utf8")
  : Buffer.from(`published-${name}`, "utf8");
const manifest = { schemaVersion: 2, kind: "develop-prerelease-candidate", source: { repository: "service-lasso/service-lasso-tui", ref: identity.sourceRef, commit: identity.sourceCommit }, release: { tag: identity.tag, prerelease: true, draft: false, immutable: true }, version: identity.version, checksumManifest: { name: "SHA256SUMS.txt", sha256: digest(bodyFor("SHA256SUMS.txt")) }, corePackagingIssue: "service-lasso/service-lasso#1461", assets: [{ platform: "win32-amd64", name: names[0], sha256: digest(bodyFor(names[0])), executable: "service-lasso-tui.exe" }, { platform: "linux-amd64", name: names[1], sha256: digest(bodyFor(names[1])), executable: "service-lasso-tui" }, { platform: "darwin-amd64", name: names[2], sha256: digest(bodyFor(names[2])), executable: "service-lasso-tui" }, { platform: "darwin-arm64", name: names[3], sha256: digest(bodyFor(names[3])), executable: "service-lasso-tui" }] };
const localBodies = Object.fromEntries(names.map(name => [name, name === "candidate-manifest.json" ? Buffer.from(JSON.stringify(manifest), "utf8") : bodyFor(name)]));
const localAssets = Object.fromEntries(names.map(name => { const body = localBodies[name]; return [name, { sha256: digest(body), size: body.length }]; }));
const release = { tag_name: identity.tag, target_commitish: identity.sourceCommit, draft: false, prerelease: true, immutable: true, assets: names.map((name, index) => ({ id: index + 101, url: `https://api.github.com/repos/service-lasso/service-lasso-tui/releases/assets/${index + 101}`, name, digest: `sha256:${localAssets[name].sha256}`, size: localAssets[name].size, browser_download_url: `https://github.com/service-lasso/service-lasso-tui/releases/download/${identity.tag}/${name}` })) };
const preflight = { immutableReleases: { enabled: true }, environment: { name: "development-candidate", protection_rules: [{ type: "wait_timer", wait_timer: 10 }], deployment_branch_policy: { protected_branches: true, custom_branch_policies: false } }, branchProtection: { required_status_checks: { strict: true, contexts: ["Linux test and build", "Windows test and build", "macOS test and build", "Release asset cross-compilation"] }, required_pull_request_reviews: { required_approving_review_count: 0 }, allow_force_pushes: { enabled: false } } };

function response(status, headers = {}, body) { return { status, headers: { get: key => headers[key.toLowerCase()] ?? null }, body: body === undefined ? undefined : (async function* () { yield body; })() }; }
function publicFetch({ bodies = localBodies, redirect = true, calls = [] } = {}) {
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
async function verify(overrides = {}) {
  const assetDirectory = await mkdtemp(path.join(os.tmpdir(), "candidate-local-bytes-"));
  try {
    await Promise.all(names.map(name => writeFile(path.join(assetDirectory, name), localBodies[name])));
    await writeFile(path.join(assetDirectory, "candidate-local-assets.json"), JSON.stringify(localAssets));
    return await verifyPublicAssetBytes({ release: overrides.release ?? release, manifest, localAssets: overrides.localAssets ?? localAssets, assetDirectory, downloadDir: await mkdtemp(path.join(os.tmpdir(), "candidate-public-bytes-")), fetchImpl: overrides.fetchImpl ?? publicFetch(overrides) });
  } finally { await rm(assetDirectory, { recursive: true, force: true }); }
}

test("accepts a complete protected preflight without inventing a review count", () => assert.equal(assertPreflight(preflight).environment, "development-candidate"));
test("rejects disabled immutable releases and unbounded environment waits", () => {
  assert.throws(() => assertPreflight({ ...preflight, immutableReleases: { enabled: false } }), /immutable/u);
  assert.throws(() => assertPreflight({ ...preflight, environment: { ...preflight.environment, protection_rules: [{ type: "wait_timer", wait_timer: 31 }] } }), /bounded/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, allow_force_pushes: undefined } }), /force pushes/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, allow_force_pushes: { enabled: true } } }), /force pushes/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, allow_force_pushes: {} } }), /force pushes/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, required_pull_request_reviews: undefined } }), /review count/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, required_pull_request_reviews: {} } }), /review count/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, required_pull_request_reviews: { required_approving_review_count: "0" } } }), /review count/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, required_pull_request_reviews: { required_approving_review_count: 0.5 } } }), /review count/u);
  assert.throws(() => assertPreflight({ ...preflight, branchProtection: { ...preflight.branchProtection, required_pull_request_reviews: { required_approving_review_count: -1 } } }), /review count/u);
});
test("requires a full-SHA-bound four-platform candidate manifest", () => {
  assert.doesNotThrow(() => assertManifest(manifest, identity));
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.slice(1) }, identity), /all platform/u);
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.map((asset, index) => index === 0 ? { ...asset, name: "service-lasso-tui-anything.zip" } : asset) }, identity), /platform inventory/u);
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.map((asset, index) => index === 0 ? { ...asset, platform: "arbitrary-platform" } : asset) }, identity), /manifest asset|platform inventory/u);
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.map((asset, index) => index === 1 ? { ...asset, platform: "win32-amd64" } : asset) }, identity), /manifest asset|platform inventory/u);
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.map((asset, index) => index === 0 ? { ...asset, executable: "service-lasso-tui" } : asset) }, identity), /manifest asset|platform inventory/u);
  assert.throws(() => assertManifest({ ...manifest, source: { ...manifest.source, unexpected: true } }, identity), /source has an unexpected shape/u);
  assert.throws(() => assertManifest({ ...manifest, release: { ...manifest.release, unexpected: true } }, identity), /release has an unexpected shape/u);
  assert.throws(() => assertManifest({ ...manifest, checksumManifest: { ...manifest.checksumManifest, unexpected: true } }, identity), /checksum manifest has an unexpected shape/u);
  assert.throws(() => assertManifest({ ...manifest, assets: manifest.assets.map((asset, index) => index === 0 ? { ...asset, unexpected: true } : asset) }, identity), /asset has an unexpected shape/u);
  assert.throws(() => candidateIdentity({ ...identity, sourceRef: "refs/heads/main" }), /develop/u);
});
test("validates complete immutable metadata inventory before reading public bytes", () => {
  assert.equal(assertReleaseReceipt(release, manifest, localAssets).immutable, true);
  assert.equal(assertExistingCandidateRecovery(release, manifest, localAssets).tag, identity.tag);
  assert.throws(() => assertReleaseReceipt({ ...release, immutable: false }, manifest, localAssets), /immutable/u);
  assert.throws(() => assertReleaseReceipt({ ...release, assets: release.assets.slice(1) }, manifest, localAssets), /inventory/u);
  assert.throws(() => assertReleaseReceipt(release, { ...manifest, assets: manifest.assets.map((asset, index) => index === 0 ? { ...asset, sha256: "f".repeat(64) } : asset) }, localAssets), /does not match the manifest/u);
  assert.throws(() => assertReleaseReceipt(release, { ...manifest, checksumManifest: { ...manifest.checksumManifest, sha256: "e".repeat(64) } }, localAssets), /checksum manifest does not match/u);
});
test("recovers a complete existing candidate only after all six public bodies match", async () => {
  const calls = []; const receipt = await verify({ calls });
  assert.equal(receipt.verified.length, 6); assert.equal(receipt.totalBytes, names.reduce((sum, name) => sum + localAssets[name].size, 0));
  assert.ok(calls.every(call => call.options.method === "GET" && call.options.redirect === "manual" && Object.keys(call.options.headers).length === 0));
  assert.ok(calls.some(call => new URL(call.url).hostname === "release-assets.githubusercontent.com"));
});
test("rejects public body mismatch, truncation, missing inventory, and over-bound bodies", async () => {
  const mismatched = { ...localBodies, [names[0]]: Buffer.from("x".repeat(localBodies[names[0]].length)) };
  await assert.rejects(() => verify({ bodies: mismatched }), /byte receipt/u);
  const truncated = { ...localBodies, [names[1]]: localBodies[names[1]].subarray(0, -1) };
  await assert.rejects(() => verify({ bodies: truncated }), /body size|truncated/u);
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.slice(1) } }), /inventory/u);
  const oversized = { ...localBodies, [names[2]]: Buffer.concat([localBodies[names[2]], Buffer.from("more")]) };
  await assert.rejects(() => verify({ bodies: oversized }), /body size|exceeds/u);
});
test("rejects missing, duplicate, and malformed asset IDs in the complete receipt pipeline", async () => {
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.map((asset, index) => index === 0 ? { ...asset, id: undefined } : asset) } }), /asset ID/u);
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.map((asset, index) => index === 1 ? { ...asset, id: release.assets[0].id, url: release.assets[0].url } : asset) } }), /asset ID/u);
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.map((asset, index) => index === 2 ? { ...asset, id: 0, url: "https://api.github.com/repos/service-lasso/service-lasso-tui/releases/assets/0" } : asset) } }), /asset ID/u);
  await assert.rejects(() => verify({ release: { ...release, assets: release.assets.map((asset, index) => index === 3 ? { ...asset, id: 999, url: "https://api.github.com/repos/service-lasso/service-lasso-tui/releases/assets/not-an-id" } : asset) } }), /record URL/u);
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

test("rejects unexpected candidate files before a provider boundary", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "candidate-unexpected-file-"));
  try {
    await Promise.all(names.map(name => writeFile(path.join(directory, name), localBodies[name])));
    await writeFile(path.join(directory, "candidate-local-assets.json"), JSON.stringify(localAssets));
    await writeFile(path.join(directory, "service-lasso-tui-unexpected.zip"), "not adopted");
    await assert.rejects(() => assertPublicationDirectory(directory, manifest), /unexpected files/u);
  } finally { await rm(directory, { recursive: true, force: true }); }
});

test("rejects an identity swap between lstat and open", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "candidate-identity-swap-")); const name = names[0]; const file = path.join(directory, name);
  try {
    await writeFile(file, localBodies[name]); await writeFile(`${file}.replacement`, localBodies[name]);
    await assert.rejects(() => readBoundedRegularLocalAsset(directory, name, 1024, async () => { await rename(`${file}.replacement`, file); }), /changed while opening/u);
  } finally { await rm(directory, { recursive: true, force: true }); }
});

test("rejects an external same-size file swapped in through a symlink", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "candidate-symlink-swap-")); const external = await mkdtemp(path.join(os.tmpdir(), "candidate-external-file-")); const name = names[0]; const file = path.join(directory, name);
  try {
    await writeFile(file, localBodies[name]); const externalFile = path.join(external, "same-size.bin"); await writeFile(externalFile, localBodies[name]);
    await assert.rejects(() => readBoundedRegularLocalAsset(directory, name, 1024, async () => { await rename(file, `${file}.original`); await symlink(externalFile, file, "file"); }), /changed while opening/u);
  } finally { await rm(directory, { recursive: true, force: true }); await rm(external, { recursive: true, force: true }); }
});

test("uploads the original held bytes if every source path is coherently replaced after verification", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "candidate-held-upload-")); const previousRepository = process.env.GITHUB_REPOSITORY; const previousToken = process.env.GH_TOKEN; const uploaded = [];
  try {
    await Promise.all(names.map(name => writeFile(path.join(directory, name), localBodies[name])));
    await writeFile(path.join(directory, "candidate-local-assets.json"), JSON.stringify(localAssets));
    process.env.GITHUB_REPOSITORY = "service-lasso/service-lasso-tui"; process.env.GH_TOKEN = "test-token";
    await uploadVerifiedCandidateAssets({ assetDirectory: directory, localAssets: path.join(directory, "candidate-local-assets.json"), identity, releaseID: "123", upload: async (_url, bytes) => {
      if (uploaded.length === 0) await Promise.all(names.map(name => writeFile(path.join(directory, name), Buffer.from(`replacement-${name}`, "utf8"))));
      uploaded.push(Buffer.from(bytes));
    } });
    assert.deepEqual(uploaded, names.map(name => localBodies[name]));
  } finally { previousRepository === undefined ? delete process.env.GITHUB_REPOSITORY : process.env.GITHUB_REPOSITORY = previousRepository; previousToken === undefined ? delete process.env.GH_TOKEN : process.env.GH_TOKEN = previousToken; await rm(directory, { recursive: true, force: true }); }
});
