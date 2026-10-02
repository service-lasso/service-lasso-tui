import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { mkdtemp, readFile, rename, rm, symlink, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { assertDraftReleaseReceipt, assertExistingCandidateRecovery, assertManifest, assertPreflight, assertPublicationDirectory, assertReleaseReceipt, assertTransportPolicy, candidateIdentity, readBoundedRegularLocalAsset, verifyPublicAssetBytes } from "./verify-candidate-publication.mjs";


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

function response(status, headers = {}, body) { return { status, headers: { get: key => headers[key.toLowerCase()] ?? null }, body: body === undefined ? undefined : (async function* () { yield body; })(), text: async () => body === undefined ? "" : Buffer.from(body).toString("utf8") }; }
function signedRedirect(name) { return `https://release-assets.githubusercontent.com/github-production-release-asset-2e65be/123/${encodeURIComponent(name)}?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ABCDEFGHIJKLMNOP%2F20261001%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20261001T000000Z&X-Amz-Expires=300&X-Amz-SignedHeaders=host&X-Amz-Signature=${"a".repeat(64)}`; }
function azureRedirect() { return "https://release-assets.githubusercontent.com/github-production-release-asset/123/01234567-89ab-cdef-0123-456789abcdef?sp=r&sv=2024-11-04&sr=b&spr=https&se=2030-01-01T00%3A00%3A00Z&rscd=attachment&rsct=application%2Foctet-stream&skoid=01234567-89ab-cdef-0123-456789abcdef&sktid=89abcdef-0123-4567-89ab-cdef01234567&skt=2029-12-31T00%3A00%3A00Z&ske=2030-01-01T00%3A00%3A00Z&sks=b&skv=2024-11-04&sig=sanitized-signature&jwt=sanitized.jwt.value&response-content-disposition=attachment&response-content-type=application%2Foctet-stream"; }
function publicFetch({ bodies = localBodies, redirect = true, calls = [] } = {}) {
  return async (url, options) => {
    calls.push({ url: String(url), options }); const parsed = new URL(url);
    if (parsed.hostname === "github.com") {
      const name = decodeURIComponent(parsed.pathname.split("/").at(-1));
      if (!bodies[name]) return response(404);
      if (redirect) return response(302, { location: signedRedirect(name) });
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
  assert.throws(() => assertDraftReleaseReceipt(release, manifest, localAssets), /draft candidate/u);
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
  await assert.rejects(() => verify({ fetchImpl: async () => response(302, { location: "https://release-assets.githubusercontent.com/github-production-release-asset-2e65be/123/opaque-value-long-enough?signature=unbounded" }) }), /signed release asset/u);
  await assert.rejects(() => verify({ fetchImpl: async () => response(302, { location: `https://objects.githubusercontent.com/github-production-release-asset-2e65be/123/opaque-value-long-enough?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ABCDEFGHIJKLMNOP%2F20261001%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20261001T000000Z&X-Amz-Expires=301&X-Amz-SignedHeaders=host&X-Amz-Signature=${"a".repeat(64)}` }) }), /signed release asset/u);
  await assert.rejects(() => verify({ fetchImpl: async url => response(302, { location: String(url) }) }), /redirect limit/u);
});
test("accepts the observed bounded GitHub Azure redirect shape", async () => {
  const calls = []; let assetIndex = 0;
  await verify({ fetchImpl: async (url, options) => {
    calls.push({ url: String(url), options });
    const parsed = new URL(url);
    if (parsed.hostname === "github.com") return response(302, { location: azureRedirect() });
    const name = [...names].sort()[assetIndex++];
    return response(200, { "content-length": String(localBodies[name].length) }, localBodies[name]);
  } });
  assert.equal(calls.filter(call => new URL(call.url).hostname === "release-assets.githubusercontent.com").length, names.length);
  assert.ok(calls.every(call => Object.keys(call.options.headers).length === 0));
});
test("rejects Azure signed redirects with missing, extra, malformed, credentialed, or oversized values", () => {
  const redirect = new URL(azureRedirect());
  const assertion = value => assert.throws(() => assertTransportPolicy({ uploadURL: "https://uploads.github.com/", downloadURL: value, authorization: "Bearer", redirect: true }), /signed release asset|release download host/u);
  const missing = new URL(redirect); missing.searchParams.delete("jwt"); assertion(missing);
  const extra = new URL(redirect); extra.searchParams.set("unexpected", "value"); assertion(extra);
  const malformedPath = new URL(redirect); malformedPath.pathname = "/github-production-release-asset/123/not-a-uuid"; assertion(malformedPath);
  const credentialed = new URL(redirect); credentialed.username = "user"; assertion(credentialed);
  const oversized = new URL(redirect); oversized.searchParams.set("jwt", "a".repeat(4097)); assertion(oversized);
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

import { publishCandidate } from "./publish-candidate.mjs";

function publisherProvider({ existing = false, orphan = false, annotated = false, malformed = false, cycle = false, mismatch = false, changeAt = 0, policyUnavailable = false, draftMismatch = false, publicMismatch = false, createCollision = false, draftRedirect = false, replaceLocal = false } = {}) {
  const calls = []; const state = { directory: null }; let writes = 0; let policyReads = 0; let releaseReads = 0; let created = existing; let tagExists = existing || orphan;
  const tagSHA = "a".repeat(40);
  const record = draft => ({ ...release, id: 123, draft, immutable: !draft, assets: draft && writes === 2 ? [] : release.assets });
  const fetchImpl = async (url, options) => {
    const parsed = new URL(url); calls.push({ url: String(url), method: options.method, headers: options.headers });
    const json = value => response(200, {}, Buffer.from(JSON.stringify(value)));
    if (parsed.hostname === "uploads.github.com") { if (replaceLocal && writes === 2) await Promise.all(names.map(name => writeFile(path.join(state.directory, name), Buffer.from(`replacement-${name}`)))); writes++; assert.deepEqual(options.body, localBodies[parsed.searchParams.get("name")]); return response(201); }
    if (options.method === "POST" && parsed.pathname.endsWith("/git/refs")) { writes++; tagExists = true; return response(201, {}, Buffer.from("{}")); }
    if (options.method === "POST") { writes++; created = true; return response(201, {}, Buffer.from(JSON.stringify(record(true)))); }
    if (options.method === "PATCH") { writes++; return json(record(false)); }
    if (parsed.pathname.endsWith("/immutable-releases")) { policyReads++; if (policyUnavailable && policyReads > 1) return response(403); return json({ enabled: true }); }
    if (parsed.pathname.endsWith("/environments/development-candidate")) return json({ ...preflight.environment, ...(changeAt && policyReads >= changeAt ? { protection_rules: [{ type: "wait_timer", wait_timer: 11 }] } : {}) });
    if (parsed.pathname.endsWith("/branches/develop/protection")) return json(preflight.branchProtection);
    if (parsed.pathname.includes("/git/ref/tags/")) {
      if (!tagExists) return response(404);
      return json({ ref: `refs/tags/${identity.tag}`, object: malformed ? { type: "blob", sha: tagSHA } : { type: annotated || cycle ? "tag" : "commit", sha: annotated || cycle ? tagSHA : mismatch ? "b".repeat(40) : identity.sourceCommit } });
    }
    if (parsed.pathname.includes("/git/tags/")) return json({ sha: tagSHA, object: { type: cycle ? "tag" : "commit", sha: cycle ? tagSHA : mismatch ? "b".repeat(40) : identity.sourceCommit } });
    if (parsed.pathname.includes("/releases/tags/")) { releaseReads++; if (createCollision && releaseReads === 2) return json(record(false)); return created ? json(record(false)) : response(404); }
    if (parsed.pathname.endsWith("/releases/123")) return json({ ...record(true), ...(draftMismatch ? { assets: release.assets.slice(1) } : {}) });
    if (parsed.pathname.includes("/releases/assets/")) { const name = release.assets.find(asset => asset.url === String(url)).name; return draftRedirect ? response(302, { location: signedRedirect(name) }) : response(200, {}, localBodies[name]); }
    if (parsed.hostname === "release-assets.githubusercontent.com") { assert.deepEqual(options.headers, {}); const name = decodeURIComponent(parsed.pathname.split("/").at(-1)); return response(200, {}, localBodies[name]); }
    if (parsed.hostname === "github.com") { assert.deepEqual(options.headers, {}); const name = decodeURIComponent(parsed.pathname.split("/").at(-1)); return response(200, {}, publicMismatch ? Buffer.alloc(localBodies[name].length) : localBodies[name]); }
    throw new Error("unexpected fixture endpoint");
  };
  return { fetchImpl, calls, state, writes: () => writes, policyReads: () => policyReads };
}
async function runPublisher(provider) {
  const directory = await mkdtemp(path.join(os.tmpdir(), "complete-publisher-"));
  try {
    await Promise.all(names.map(name => writeFile(path.join(directory, name), localBodies[name])));
    await writeFile(path.join(directory, "candidate-local-assets.json"), JSON.stringify(localAssets));
    provider.state.directory = directory;
    return await publishCandidate({ assetDirectory: directory, identity, token: "fixture-token", fetchImpl: provider.fetchImpl });
  } finally { await rm(directory, { recursive: true, force: true }); }
}
test("complete publisher creates once, verifies private inventory, publishes once, and resolves annotated source", async () => {
  const provider = publisherProvider({ annotated: true }); const result = await runPublisher(provider);
  assert.equal(result.recovered, false); assert.equal(result.receipt.verified.length, 6); assert.equal(result.receipt.tagProof.chain.length, 2);
  assert.equal(provider.writes(), 9); assert.equal(provider.policyReads(), 10);
  assert.equal(provider.calls.filter(call => call.method === "PATCH").length, 1);
});
test("complete publisher exact immutable recovery is read-only for lightweight and annotated tags", async () => {
  for (const annotated of [false, true]) { const provider = publisherProvider({ existing: true, annotated }); assert.equal((await runPublisher(provider)).recovered, true); assert.equal(provider.writes(), 0); }
});
test("complete publisher refuses orphan, mismatched, malformed, cyclic and read-only release collisions before writes", async () => {
  for (const options of [{ orphan: true }, { orphan: true, annotated: true }, { existing: true, mismatch: true }, { existing: true, annotated: true, mismatch: true }, { orphan: true, malformed: true }, { orphan: true, cycle: true }, { createCollision: true }]) {
    const provider = publisherProvider(options); await assert.rejects(() => runPublisher(provider)); assert.equal(provider.writes(), 0);
  }
});
test("policy changes or denial midphase stop before the next mutation", async () => {
  for (const [options, writes] of [[{ changeAt: 2 }, 0], [{ changeAt: 3 }, 1], [{ changeAt: 10 }, 8], [{ policyUnavailable: true }, 0]]) {
    const provider = publisherProvider(options); await assert.rejects(() => runPublisher(provider), /policy|provider/u); assert.equal(provider.writes(), writes);
  }
});
test("private complete-byte gate denies publish and final public mismatch denies acceptance", async () => {
  const draft = publisherProvider({ draftMismatch: true }); await assert.rejects(() => runPublisher(draft)); assert.equal(draft.writes(), 8);
  const final = publisherProvider({ publicMismatch: true }); await assert.rejects(() => runPublisher(final)); assert.equal(final.writes(), 9);
});




test("complete publisher uploads held bytes after path replacement and private redirect drops all auth", async () => {
  const provider = publisherProvider({ replaceLocal: true, draftRedirect: true });
  await assert.rejects(() => runPublisher(provider), /actual local/u);
  assert.equal(provider.writes(), 9);
  assert.equal(provider.calls.filter(call => new URL(call.url).hostname === "release-assets.githubusercontent.com").length, 6);
});
