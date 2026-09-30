import { createHash } from "node:crypto";
import { lstat, mkdir, open, readFile, readdir, rm } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const SHA256 = /^[a-f0-9]{64}$/u;
const SHA1 = /^[a-f0-9]{40}$/u;
const MAX_REDIRECTS = 5;
const MAX_ASSET_BYTES = 1024 * 1024 * 1024;
const MAX_TOTAL_BYTES = 2 * 1024 * 1024 * 1024;
const MAX_METADATA_BYTES = 1024 * 1024;
const PUBLIC_DOWNLOAD_HOSTS = new Set(["github.com", "github-releases.githubusercontent.com", "objects.githubusercontent.com", "release-assets.githubusercontent.com"]);
const REQUIRED_CHECKS = Object.freeze(["Linux test and build", "Windows test and build", "macOS test and build", "Release asset cross-compilation"]);
const REQUIRED_PLATFORMS = Object.freeze(["darwin-amd64", "darwin-arm64", "linux-amd64", "win32-amd64"]);
const PLATFORM_ARCHIVES = Object.freeze({
  "darwin-amd64": { extension: "tar.gz", executable: "service-lasso-tui" },
  "darwin-arm64": { extension: "tar.gz", executable: "service-lasso-tui" },
  "linux-amd64": { extension: "tar.gz", executable: "service-lasso-tui" },
  "win32-amd64": { extension: "zip", executable: "service-lasso-tui.exe" },
});

function fail(message) { throw new Error(message); }
function object(value, name) { if (!value || typeof value !== "object" || Array.isArray(value)) fail(`${name} must be an object`); return value; }
function exactKeys(value, keys, name) { const actual = Object.keys(object(value, name)).sort(); const expected = [...keys].sort(); if (actual.length !== expected.length || actual.some((key, index) => key !== expected[index])) fail(`${name} has an unexpected shape`); }
function positiveBoundedSize(value, name) { if (!Number.isSafeInteger(value) || value <= 0 || value > MAX_ASSET_BYTES) fail(`${name} size is invalid`); return value; }
function fixedArchiveAssets(version) { return REQUIRED_PLATFORMS.map(platform => ({ platform, name: `service-lasso-tui-${version}-${platform}.${PLATFORM_ARCHIVES[platform].extension}`, executable: PLATFORM_ARCHIVES[platform].executable })); }
function fixedInventoryNames(version) { return [...fixedArchiveAssets(version).map(asset => asset.name), "SHA256SUMS.txt", "candidate-manifest.json"].sort(); }

export function candidateIdentity({ sourceRef, sourceCommit, version, tag }) {
  if (sourceRef !== "refs/heads/develop" || !SHA1.test(sourceCommit)) fail("candidate source must be the full develop commit");
  if (!/^\d{4}\.\d{1,2}\.\d{1,2}-[a-f0-9]{7}$/u.test(version) || version.slice(-7) !== sourceCommit.slice(0, 7)) fail("candidate version must bind the source short SHA");
  if (tag !== `candidate-${version}`) fail("candidate tag must bind the version");
  return { sourceRef, sourceCommit, version, tag };
}

export function assertPreflight({ immutableReleases, environment, branchProtection }) {
  if (immutableReleases?.enabled !== true) fail("immutable GitHub releases are not enabled");
  if (environment?.name !== "development-candidate") fail("development-candidate environment is missing");
  const wait = environment?.protection_rules?.find(rule => rule?.type === "wait_timer");
  if (!Number.isInteger(wait?.wait_timer) || wait.wait_timer < 1 || wait.wait_timer > 30) fail("development-candidate requires a bounded one-to-thirty minute wait timer");
  const policy = environment?.deployment_branch_policy;
  if (policy?.protected_branches !== true || policy?.custom_branch_policies !== false) fail("development-candidate must be restricted to protected branches");
  if (branchProtection?.required_status_checks?.strict !== true) fail("develop branch protection must require up-to-date checks");
  const contexts = new Set(branchProtection.required_status_checks.contexts ?? []);
  if (!REQUIRED_CHECKS.every(check => contexts.has(check))) fail("develop branch protection is missing a current TUI CI check");
  const reviewCount = branchProtection?.required_pull_request_reviews?.required_approving_review_count;
  if (!Number.isSafeInteger(reviewCount) || reviewCount < 0) fail("develop branch protection must report a valid required approving review count");
  if (branchProtection?.allow_force_pushes?.enabled !== false) fail("develop branch protection must explicitly disable force pushes");
  return { environment: environment.name, waitMinutes: wait.wait_timer, requiredChecks: REQUIRED_CHECKS };
}

export function assertManifest(manifest, identity) {
  exactKeys(manifest, ["assets", "checksumManifest", "corePackagingIssue", "kind", "release", "schemaVersion", "source", "version"], "candidate manifest");
  if (manifest.schemaVersion !== 2 || manifest.kind !== "develop-prerelease-candidate" || manifest.version !== identity.version) fail("candidate manifest identity is invalid");
  exactKeys(manifest.source, ["commit", "ref", "repository"], "candidate manifest source");
  exactKeys(manifest.release, ["draft", "immutable", "prerelease", "tag"], "candidate manifest release");
  exactKeys(manifest.checksumManifest, ["name", "sha256"], "candidate checksum manifest");
  if (manifest.source?.repository !== "service-lasso/service-lasso-tui" || manifest.source?.ref !== identity.sourceRef || manifest.source?.commit !== identity.sourceCommit) fail("candidate manifest source is invalid");
  if (manifest.corePackagingIssue !== "service-lasso/service-lasso#1461") fail("candidate manifest Core handoff is invalid");
  if (manifest.release?.tag !== identity.tag || manifest.release?.prerelease !== true || manifest.release?.draft !== false || manifest.release?.immutable !== true) fail("candidate manifest release contract is invalid");
  if (manifest.checksumManifest?.name !== "SHA256SUMS.txt" || !SHA256.test(manifest.checksumManifest?.sha256)) fail("candidate checksum manifest is invalid");
  const expectedAssets = fixedArchiveAssets(identity.version);
  if (!Array.isArray(manifest.assets) || manifest.assets.length !== expectedAssets.length) fail("candidate manifest must describe all platform assets");
  for (const expected of expectedAssets) {
    const asset = manifest.assets.find(candidate => candidate?.platform === expected.platform);
    exactKeys(asset, ["executable", "name", "platform", "sha256"], "candidate manifest asset");
    if (!asset || asset.name !== expected.name || asset.executable !== expected.executable || !SHA256.test(asset.sha256)) fail("candidate manifest platform inventory is invalid");
  }
  return manifest;
}

function assertLocalAssets(localAssets, requiredNames) {
  exactKeys(localAssets, requiredNames, "local candidate asset inventory");
  for (const name of requiredNames) {
    const asset = localAssets[name];
    if (!SHA256.test(asset?.sha256) || !positiveBoundedSize(asset?.size, `local ${name}`)) fail(`local candidate asset inventory is invalid for ${name}`);
  }
  return localAssets;
}

function localAssetPath(assetDirectory, name) {
  const root = path.resolve(assetDirectory);
  const file = path.resolve(root, name);
  if (path.dirname(file) !== root) fail("local candidate asset path is invalid");
  return { root, file };
}

function fileIdentity(stat) {
  const valid = value => (typeof value === "bigint" && value >= 0n) || (typeof value === "number" && Number.isFinite(value) && value >= 0);
  if (!valid(stat?.dev) || !valid(stat?.ino)) fail("local candidate asset identity is unavailable");
  return { dev: stat.dev, ino: stat.ino, size: stat.size, ctimeMs: stat.ctimeMs, birthtimeMs: stat.birthtimeMs };
}
function sameFileIdentity(left, right) { return left.dev === right.dev && left.ino === right.ino && left.size === right.size && left.ctimeMs === right.ctimeMs && left.birthtimeMs === right.birthtimeMs; }

export async function readBoundedRegularLocalAsset(assetDirectory, name, maxBytes = MAX_ASSET_BYTES, beforeOpen) {
  const { root, file } = localAssetPath(assetDirectory, name);
  const directory = await lstat(root);
  if (!directory.isDirectory() || directory.isSymbolicLink()) fail("local candidate asset directory is invalid");
  const entry = await lstat(file);
  if (!entry.isFile() || entry.isSymbolicLink() || entry.size <= 0 || entry.size > maxBytes) fail(`local candidate asset is invalid for ${name}`);
  const entryIdentity = fileIdentity(entry);
  if (beforeOpen) await beforeOpen(file);
  const handle = await open(file, "r");
  try {
    const opened = await handle.stat();
    if (!opened.isFile() || !sameFileIdentity(entryIdentity, fileIdentity(opened)) || opened.size <= 0 || opened.size > maxBytes) fail(`local candidate asset changed while opening for ${name}`);
    const bytes = await handle.readFile();
    const completed = await handle.stat();
    if (bytes.length !== entry.size || !sameFileIdentity(entryIdentity, fileIdentity(completed))) fail(`local candidate asset changed while reading for ${name}`);
    return bytes;
  } finally {
    await handle.close();
  }
}

function assertChecksumManifestBytes(bytes, manifest) {
  const expected = fixedArchiveAssets(manifest.version);
  const lines = bytes.toString("utf8").split("\n");
  if (lines.at(-1) === "") lines.pop();
  if (lines.length !== expected.length) fail("candidate checksum manifest inventory is invalid");
  const checksums = new Map();
  for (const line of lines) {
    const match = /^([a-f0-9]{64})  ([^/\\\r\n]+)$/u.exec(line);
    if (!match || checksums.has(match[2])) fail("candidate checksum manifest inventory is invalid");
    checksums.set(match[2], match[1]);
  }
  for (const asset of expected) {
    const recorded = manifest.assets.find(candidate => candidate?.platform === asset.platform);
    if (checksums.get(asset.name) !== recorded.sha256) fail(`candidate checksum manifest does not match the manifest for ${asset.name}`);
  }
}

export async function assertActualLocalAssets({ assetDirectory, manifest, localAssets }) {
  const requiredNames = fixedInventoryNames(manifest?.version);
  assertLocalAssets(localAssets, requiredNames);
  const actual = {};
  for (const name of requiredNames) {
    const bytes = await readBoundedRegularLocalAsset(assetDirectory, name, (name.endsWith(".json") || name === "SHA256SUMS.txt") ? MAX_METADATA_BYTES : MAX_ASSET_BYTES);
    actual[name] = { sha256: createHash("sha256").update(bytes).digest("hex"), size: bytes.length, bytes };
    if (actual[name].sha256 !== localAssets[name].sha256 || actual[name].size !== localAssets[name].size) fail(`actual local candidate asset does not match the inventory for ${name}`);
  }
  for (const expected of fixedArchiveAssets(manifest.version)) {
    const asset = manifest.assets.find(candidate => candidate?.platform === expected.platform);
    if (asset.sha256 !== actual[expected.name].sha256) fail(`actual local candidate asset does not match the manifest for ${expected.name}`);
  }
  if (manifest.checksumManifest.sha256 !== actual["SHA256SUMS.txt"].sha256) fail("actual local checksum manifest does not match the manifest");
  assertChecksumManifestBytes(actual["SHA256SUMS.txt"].bytes, manifest);
  return Object.fromEntries(requiredNames.map(name => [name, { sha256: actual[name].sha256, size: actual[name].size }]));
}

export async function assertPublicationDirectory(assetDirectory, manifest) {
  const expected = new Set([...fixedInventoryNames(manifest?.version), "candidate-local-assets.json"]);
  const entries = await readdir(assetDirectory);
  if (entries.length !== expected.size || entries.some(entry => !expected.has(entry))) fail("candidate publication directory has unexpected files");
  return [...expected].sort();
}

// The returned buffers are the custody boundary for a new publication.  Callers
// upload these held bytes, never paths that can be reopened after verification.
export async function holdVerifiedLocalAssets({ assetDirectory, manifest, localAssets, beforeOpen }) {
  const requiredNames = fixedInventoryNames(manifest?.version);
  assertLocalAssets(localAssets, requiredNames);
  const held = {}; let totalBytes = 0;
  for (const name of requiredNames) {
    const bytes = await readBoundedRegularLocalAsset(assetDirectory, name, (name.endsWith(".json") || name === "SHA256SUMS.txt") ? MAX_METADATA_BYTES : MAX_ASSET_BYTES, beforeOpen);
    totalBytes += bytes.length;
    if (totalBytes > MAX_TOTAL_BYTES) fail("candidate upload inventory exceeds held-byte bound");
    const sha256 = createHash("sha256").update(bytes).digest("hex");
    if (sha256 !== localAssets[name].sha256 || bytes.length !== localAssets[name].size) fail(`actual local candidate asset does not match the inventory for ${name}`);
    held[name] = bytes;
  }
  const heldInventory = Object.fromEntries(requiredNames.map(name => [name, { sha256: createHash("sha256").update(held[name]).digest("hex"), size: held[name].length, bytes: held[name] }]));
  const actual = await assertActualLocalAssets({ assetDirectory, manifest, localAssets: Object.fromEntries(requiredNames.map(name => [name, heldInventory[name]])) });
  // Verify the manifest/checksum relation against the held immutable bytes too.
  assertChecksumManifestBytes(held["SHA256SUMS.txt"], manifest);
  return { inventory: actual, bytes: held, totalBytes };
}

function assertReleaseAssetMetadata(release, manifest, localAssets, draft) {
  const requiredNames = fixedInventoryNames(manifest?.version);
  assertLocalAssets(localAssets, requiredNames);
  for (const expected of fixedArchiveAssets(manifest?.version)) {
    const asset = manifest?.assets?.find(candidate => candidate?.platform === expected.platform);
    if (!asset || asset.name !== expected.name || asset.executable !== expected.executable || localAssets[expected.name].sha256 !== asset.sha256) fail(`local candidate asset inventory does not match the manifest for ${expected.name}`);
  }
  if (manifest?.checksumManifest?.name !== "SHA256SUMS.txt" || localAssets["SHA256SUMS.txt"].sha256 !== manifest.checksumManifest.sha256) fail("local candidate checksum manifest does not match the manifest");
  object(release, "release");
  if (release.tag_name !== manifest.release.tag || release.target_commitish !== manifest.source.commit || release.draft !== draft || release.prerelease !== true || (!draft && release.immutable !== true)) fail(draft ? "release does not prove the exact draft candidate identity" : "release does not prove the immutable exact candidate identity");
  if (!Array.isArray(release.assets)) fail("release assets are missing");
  const names = release.assets.map(asset => asset?.name).sort();
  if (names.length !== requiredNames.length || names.some((name, index) => name !== requiredNames[index])) fail("release asset inventory is incomplete or unexpected");
  const assetIDs = new Set();
  for (const name of requiredNames) {
    const remote = release.assets.find(asset => asset.name === name); const local = localAssets[name];
    if (!Number.isSafeInteger(remote?.id) || remote.id <= 0 || assetIDs.has(remote.id)) fail(`release asset ID is invalid for ${name}`);
    assetIDs.add(remote.id);
    if (typeof remote?.url !== "string") fail(`release asset record URL is invalid for ${name}`);
    const assetURL = new URL(remote.url);
    if (assetURL.protocol !== "https:" || assetURL.hostname !== "api.github.com" || assetURL.port || assetURL.username || assetURL.password || assetURL.hash || assetURL.search || assetURL.pathname !== `/repos/service-lasso/service-lasso-tui/releases/assets/${remote.id}`) fail(`release asset record URL is invalid for ${name}`);
    if (remote?.digest !== `sha256:${local.sha256}` || positiveBoundedSize(remote?.size, `release ${name}`) !== local.size || typeof remote.browser_download_url !== "string") fail(`release asset receipt is invalid for ${name}`);
    if (!draft) assertInitialAssetURL(remote.browser_download_url, release, name);
  }
  return { tag: release.tag_name, commit: release.target_commitish, immutable: release.immutable === true, assets: requiredNames };
}

export function assertReleaseReceipt(release, manifest, localAssets) { return assertReleaseAssetMetadata(release, manifest, localAssets, false); }
export function assertDraftReleaseReceipt(release, manifest, localAssets) { return assertReleaseAssetMetadata(release, manifest, localAssets, true); }

export function assertExistingCandidateRecovery(release, manifest, localAssets) { return assertReleaseReceipt(release, manifest, localAssets); }

export function assertTransportPolicy({ uploadURL, downloadURL, authorization, redirect = false }) {
  const upload = new URL(uploadURL); const download = new URL(downloadURL);
  const explicitPort = value => typeof value === "string" && /^https:\/\/[^/?#@]+:\d+(?:[/?#]|$)/iu.test(value);
  if (upload.protocol !== "https:" || upload.hostname !== "uploads.github.com" || upload.port || explicitPort(uploadURL) || upload.username || upload.password || upload.hash || authorization !== "Bearer") fail("release upload must use GitHub's Bearer-authenticated upload host");
  if (download.protocol !== "https:" || !PUBLIC_DOWNLOAD_HOSTS.has(download.hostname) || download.port || explicitPort(downloadURL) || download.username || download.password || download.hash) fail("release download host is not approved for headerless public retrieval");
  if (["github.com", "github-releases.githubusercontent.com"].includes(download.hostname) && (download.search || !/^\/[^/]+\/[^/]+\/releases\/download\/[^/]+\/[^/]+$/u.test(download.pathname))) fail("GitHub release download URL is malformed");
  if (["objects.githubusercontent.com", "release-assets.githubusercontent.com"].includes(download.hostname)) {
    if (!/^\/github-production-release-asset-2e65be\/[1-9][0-9]{0,18}\/[A-Za-z0-9._~-]{1,256}$/u.test(download.pathname)) fail("signed release asset path is malformed");
    const permitted = new Set(["X-Amz-Algorithm", "X-Amz-Credential", "X-Amz-Date", "X-Amz-Expires", "X-Amz-SignedHeaders", "X-Amz-Signature"]);
    const entries = [...download.searchParams.entries()];
    if (entries.length !== permitted.size || new Set(entries.map(([key]) => key)).size !== permitted.size || entries.some(([key]) => !permitted.has(key))) fail("signed release asset query is malformed");
    const query = Object.fromEntries(entries);
    if (query["X-Amz-Algorithm"] !== "AWS4-HMAC-SHA256" || !/^[A-Z0-9]{16,32}\/20[0-9]{6}\/us-(east|west)-[12]\/s3\/aws4_request$/u.test(query["X-Amz-Credential"]) || !/^20[0-9]{6}T[0-9]{6}Z$/u.test(query["X-Amz-Date"]) || !/^[1-9][0-9]{0,2}$/u.test(query["X-Amz-Expires"]) || Number(query["X-Amz-Expires"]) > 300 || query["X-Amz-SignedHeaders"] !== "host" || !/^[a-f0-9]{64}$/u.test(query["X-Amz-Signature"])) fail("signed release asset query is malformed");
  }
  return true;
}

function assertInitialAssetURL(value, release, name) {
  const url = new URL(value);
  assertTransportPolicy({ uploadURL: "https://uploads.github.com/", downloadURL: url, authorization: "Bearer" });
  if (url.hostname !== "github.com" || url.search || url.pathname !== `/service-lasso/service-lasso-tui/releases/download/${encodeURIComponent(release.tag_name)}/${encodeURIComponent(name)}`) fail(`release download URL is malformed for ${name}`);
  return url;
}

async function streamResponse(response, destination, expectedSize) {
  const contentLength = response.headers?.get?.("content-length");
  if (contentLength !== null && contentLength !== undefined && (!/^\d+$/u.test(contentLength) || Number(contentLength) !== expectedSize)) fail("public asset declared body size mismatches provider size");
  if (!response.body || typeof response.body[Symbol.asyncIterator] !== "function") fail("public asset response body is missing");
  const output = await open(destination, "wx"); const hash = createHash("sha256"); let size = 0;
  try { for await (const chunk of response.body) { const bytes = Buffer.from(chunk); size += bytes.length; if (size > expectedSize || size > MAX_ASSET_BYTES) fail("public asset body exceeds its bound"); hash.update(bytes); await output.write(bytes); } } finally { await output.close(); }
  if (size !== expectedSize) fail("public asset body is truncated or size-mismatched");
  return { size, sha256: hash.digest("hex") };
}

async function fetchPublicAsset(initialURL, expectedSize, destination, fetchImpl) {
  let url = initialURL;
  for (let redirects = 0; redirects <= MAX_REDIRECTS; redirects += 1) {
    assertTransportPolicy({ uploadURL: "https://uploads.github.com/", downloadURL: url, authorization: "Bearer", redirect: redirects > 0 });
    const response = await fetchImpl(url, { method: "GET", redirect: "manual", headers: {} });
    if (response.status === 200) return streamResponse(response, destination, expectedSize);
    if (![301, 302, 303, 307, 308].includes(response.status)) fail(`public asset download failed with HTTP ${response.status}`);
    if (redirects === MAX_REDIRECTS) fail("public asset download redirect limit exceeded");
    const location = response.headers?.get?.("location");
    if (!location || !/^https:\/\//iu.test(location)) fail("public asset download redirect is malformed");
    assertTransportPolicy({ uploadURL: "https://uploads.github.com/", downloadURL: location, authorization: "Bearer", redirect: true });
    url = new URL(location);
  }
  fail("public asset download redirect limit exceeded");
}

function assertDraftAssetAPIURL(value, id) {
  const url = new URL(value);
  if (url.protocol !== "https:" || url.hostname !== "api.github.com" || url.port || url.username || url.password || url.hash || url.search || url.pathname !== `/repos/service-lasso/service-lasso-tui/releases/assets/${id}`) fail("draft release asset record URL is invalid");
  return url;
}

async function fetchDraftAsset(asset, expectedSize, destination, token, fetchImpl) {
  let url = assertDraftAssetAPIURL(asset.url, asset.id); let authenticated = true;
  for (let redirects = 0; redirects <= MAX_REDIRECTS; redirects += 1) {
    const headers = authenticated ? { Accept: "application/octet-stream", Authorization: `Bearer ${token}` } : {};
    const response = await fetchImpl(url, { method: "GET", redirect: "manual", headers });
    if (response.status === 200) return streamResponse(response, destination, expectedSize);
    if (![301, 302, 303, 307, 308].includes(response.status)) fail(`draft asset download failed with HTTP ${response.status}`);
    if (redirects === MAX_REDIRECTS) fail("draft asset download redirect limit exceeded");
    const location = response.headers?.get?.("location");
    if (!location || !/^https:\/\//iu.test(location)) fail("draft asset download redirect is malformed");
    assertTransportPolicy({ uploadURL: "https://uploads.github.com/", downloadURL: location, authorization: "Bearer", redirect: true });
    url = new URL(location); authenticated = false;
  }
  fail("draft asset download redirect limit exceeded");
}

export async function verifyDraftAssetBytes({ release, manifest, localAssets, heldBytes, downloadDir, token, fetchImpl = fetch }) {
  if (typeof token !== "string" || token.length < 1) fail("draft release read authority is invalid");
  const receipt = assertDraftReleaseReceipt(release, manifest, localAssets);
  if (!heldBytes || typeof heldBytes !== "object") fail("draft held candidate bytes are missing");
  await mkdir(downloadDir, { recursive: true }); let total = 0; const verified = [];
  try {
    for (let index = 0; index < receipt.assets.length; index += 1) {
      const name = receipt.assets[index]; const remote = release.assets.find(asset => asset.name === name); const expected = heldBytes[name];
      if (!Buffer.isBuffer(expected) || expected.length !== localAssets[name].size || createHash("sha256").update(expected).digest("hex") !== localAssets[name].sha256) fail(`draft held candidate bytes are invalid for ${name}`);
      if (total + remote.size > MAX_TOTAL_BYTES) fail("draft asset inventory exceeds temporary storage bound");
      const result = await fetchDraftAsset(remote, remote.size, `${downloadDir}/draft-asset-${index}`, token, fetchImpl);
      if (result.sha256 !== localAssets[name].sha256 || result.sha256 !== remote.digest.slice("sha256:".length) || result.size !== expected.length) fail(`draft asset byte receipt is invalid for ${name}`);
      total += result.size; verified.push({ name, ...result });
    }
  } finally { await rm(downloadDir, { recursive: true, force: true }); }
  return { ...receipt, totalBytes: total, verified };
}

export async function verifyPublicAssetBytes({ release, manifest, localAssets, assetDirectory, downloadDir, fetchImpl = fetch }) {
  await assertActualLocalAssets({ assetDirectory, manifest, localAssets });
  const receipt = assertReleaseReceipt(release, manifest, localAssets); await mkdir(downloadDir, { recursive: true }); let total = 0; const verified = [];
  try {
    for (let index = 0; index < receipt.assets.length; index += 1) {
      const name = receipt.assets[index]; const remote = release.assets.find(asset => asset.name === name); const local = localAssets[name];
      if (total + remote.size > MAX_TOTAL_BYTES) fail("public asset inventory exceeds temporary storage bound");
      const result = await fetchPublicAsset(assertInitialAssetURL(remote.browser_download_url, release, name), remote.size, `${downloadDir}/asset-${index}`, fetchImpl);
      if (result.sha256 !== local.sha256 || result.sha256 !== remote.digest.slice("sha256:".length) || result.size !== local.size || result.size !== remote.size) fail(`public asset byte receipt is invalid for ${name}`);
      total += result.size; verified.push({ name, ...result });
    }
  } finally { await rm(downloadDir, { recursive: true, force: true }); }
  return { ...receipt, totalBytes: total, verified };
}

async function readJSON(file) { return JSON.parse(await readFile(file, "utf8")); }
function assertLocalMetadataPath(assetDirectory, supplied, name) {
  if (path.resolve(supplied) !== path.resolve(assetDirectory, name)) fail(`local candidate ${name} path is invalid`);
}
async function readLocalMetadataJSON(assetDirectory, name) {
  return JSON.parse((await readBoundedRegularLocalAsset(assetDirectory, name, MAX_METADATA_BYTES)).toString("utf8"));
}
function parseArgs(argv) { const values = {}; for (let index = 0; index < argv.length; index += 1) { const argument = argv[index]; if (!argument.startsWith("--") || !argv[index + 1] || values[argument.slice(2)]) fail("usage"); values[argument.slice(2)] = argv[index + 1]; index += 1; } if (!values.mode) fail("usage"); return values; }
export async function main(argv) {
  const values = parseArgs(argv); const identity = candidateIdentity({ sourceRef: values["source-ref"], sourceCommit: values["source-commit"], version: values.version, tag: values.tag });
  if (values.mode === "preflight") { const result = assertPreflight({ immutableReleases: await readJSON(values["immutable-releases"]), environment: await readJSON(values.environment), branchProtection: await readJSON(values["branch-protection"]) }); process.stdout.write(`${JSON.stringify(result)}\n`); return; }
  if (!values.manifest || !values["local-assets"] || !values["asset-directory"]) fail("usage");
  assertLocalMetadataPath(values["asset-directory"], values.manifest, "candidate-manifest.json"); assertLocalMetadataPath(values["asset-directory"], values["local-assets"], "candidate-local-assets.json");
  const manifest = assertManifest(await readLocalMetadataJSON(values["asset-directory"], "candidate-manifest.json"), identity); const localAssets = await readLocalMetadataJSON(values["asset-directory"], "candidate-local-assets.json");
  if (values.mode === "local-assets") { const result = await assertActualLocalAssets({ assetDirectory: values["asset-directory"], manifest, localAssets }); process.stdout.write(`${JSON.stringify(result)}\n`); return; }
  if (values.mode === "publication-assets") { const result = await assertPublicationDirectory(values["asset-directory"], manifest); await assertActualLocalAssets({ assetDirectory: values["asset-directory"], manifest, localAssets }); process.stdout.write(`${JSON.stringify(result)}\n`); return; }
  if (values.mode !== "receipt" || !values.release || !values["download-dir"]) fail("usage");
  const result = await verifyPublicAssetBytes({ release: await readJSON(values.release), manifest, localAssets, assetDirectory: values["asset-directory"], downloadDir: values["download-dir"] });
  process.stdout.write(`${JSON.stringify(result)}\n`);
}
if (process.argv[1] === fileURLToPath(import.meta.url)) main(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 1; });
