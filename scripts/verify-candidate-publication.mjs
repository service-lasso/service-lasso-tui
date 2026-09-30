import { createHash } from "node:crypto";
import { mkdir, open, readFile, rm } from "node:fs/promises";
import { fileURLToPath } from "node:url";

const SHA256 = /^[a-f0-9]{64}$/u;
const SHA1 = /^[a-f0-9]{40}$/u;
const MAX_REDIRECTS = 5;
const MAX_ASSET_BYTES = 1024 * 1024 * 1024;
const MAX_TOTAL_BYTES = 2 * 1024 * 1024 * 1024;
const PUBLIC_DOWNLOAD_HOSTS = new Set(["github.com", "github-releases.githubusercontent.com", "objects.githubusercontent.com", "release-assets.githubusercontent.com"]);
const REQUIRED_CHECKS = Object.freeze(["Linux test and build", "Windows test and build", "macOS test and build", "Release asset cross-compilation"]);
const REQUIRED_PLATFORMS = Object.freeze(["darwin-amd64", "darwin-arm64", "linux-amd64", "win32-amd64"]);

function fail(message) { throw new Error(message); }
function object(value, name) { if (!value || typeof value !== "object" || Array.isArray(value)) fail(`${name} must be an object`); return value; }
function exactKeys(value, keys, name) { const actual = Object.keys(object(value, name)).sort(); const expected = [...keys].sort(); if (actual.length !== expected.length || actual.some((key, index) => key !== expected[index])) fail(`${name} has an unexpected shape`); }
function positiveBoundedSize(value, name) { if (!Number.isSafeInteger(value) || value <= 0 || value > MAX_ASSET_BYTES) fail(`${name} size is invalid`); return value; }

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
  if (!branchProtection?.required_pull_request_reviews) fail("develop branch protection must require pull-request review");
  if (branchProtection?.allow_force_pushes?.enabled !== false) fail("develop branch protection must explicitly disable force pushes");
  return { environment: environment.name, waitMinutes: wait.wait_timer, requiredChecks: REQUIRED_CHECKS };
}

export function assertManifest(manifest, identity) {
  exactKeys(manifest, ["assets", "checksumManifest", "corePackagingIssue", "kind", "release", "schemaVersion", "source", "version"], "candidate manifest");
  if (manifest.schemaVersion !== 2 || manifest.kind !== "develop-prerelease-candidate" || manifest.version !== identity.version) fail("candidate manifest identity is invalid");
  if (manifest.source?.repository !== "service-lasso/service-lasso-tui" || manifest.source?.ref !== identity.sourceRef || manifest.source?.commit !== identity.sourceCommit) fail("candidate manifest source is invalid");
  if (manifest.corePackagingIssue !== "service-lasso/service-lasso#1461") fail("candidate manifest Core handoff is invalid");
  if (manifest.release?.tag !== identity.tag || manifest.release?.prerelease !== true || manifest.release?.draft !== false || manifest.release?.immutable !== true) fail("candidate manifest release contract is invalid");
  if (manifest.checksumManifest?.name !== "SHA256SUMS.txt" || !SHA256.test(manifest.checksumManifest?.sha256)) fail("candidate checksum manifest is invalid");
  if (!Array.isArray(manifest.assets) || manifest.assets.length !== REQUIRED_PLATFORMS.length) fail("candidate manifest must describe all platform assets");
  const platforms = manifest.assets.map(asset => asset?.platform).sort();
  if (platforms.some((platform, index) => platform !== REQUIRED_PLATFORMS[index])) fail("candidate manifest platform inventory is invalid");
  for (const asset of manifest.assets) if (typeof asset.name !== "string" || !SHA256.test(asset.sha256) || !["service-lasso-tui", "service-lasso-tui.exe"].includes(asset.executable)) fail("candidate manifest asset is invalid");
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

export function assertReleaseReceipt(release, manifest, localAssets) {
  object(release, "release");
  if (release.tag_name !== manifest.release.tag || release.target_commitish !== manifest.source.commit || release.draft !== false || release.prerelease !== true || release.immutable !== true) fail("release does not prove the immutable exact candidate identity");
  if (!Array.isArray(release.assets)) fail("release assets are missing");
  const requiredNames = [...manifest.assets.map(asset => asset.name), manifest.checksumManifest.name, "candidate-manifest.json"].sort();
  const names = release.assets.map(asset => asset?.name).sort();
  if (names.length !== requiredNames.length || names.some((name, index) => name !== requiredNames[index])) fail("release asset inventory is incomplete or unexpected");
  assertLocalAssets(localAssets, requiredNames);
  const assetIDs = new Set();
  for (const name of requiredNames) {
    const remote = release.assets.find(asset => asset.name === name); const local = localAssets[name];
    if (!Number.isSafeInteger(remote?.id) || remote.id <= 0 || assetIDs.has(remote.id)) fail(`release asset ID is invalid for ${name}`);
    assetIDs.add(remote.id);
    if (typeof remote?.url !== "string") fail(`release asset record URL is invalid for ${name}`);
    const assetURL = new URL(remote.url);
    if (assetURL.protocol !== "https:" || assetURL.hostname !== "api.github.com" || assetURL.port || assetURL.username || assetURL.password || assetURL.hash || assetURL.search || assetURL.pathname !== `/repos/service-lasso/service-lasso-tui/releases/assets/${remote.id}`) fail(`release asset record URL is invalid for ${name}`);
    if (remote?.digest !== `sha256:${local.sha256}` || positiveBoundedSize(remote?.size, `release ${name}`) !== local.size || typeof remote.browser_download_url !== "string") fail(`release asset receipt is invalid for ${name}`);
    assertInitialAssetURL(remote.browser_download_url, release, name);
  }
  return { tag: release.tag_name, commit: release.target_commitish, immutable: release.immutable, assets: requiredNames };
}

export function assertExistingCandidateRecovery(release, manifest, localAssets) { return assertReleaseReceipt(release, manifest, localAssets); }

export function assertTransportPolicy({ uploadURL, downloadURL, authorization, redirect = false }) {
  const upload = new URL(uploadURL); const download = new URL(downloadURL);
  const explicitPort = value => typeof value === "string" && /^https:\/\/[^/?#@]+:\d+(?:[/?#]|$)/iu.test(value);
  if (upload.protocol !== "https:" || upload.hostname !== "uploads.github.com" || upload.port || explicitPort(uploadURL) || upload.username || upload.password || upload.hash || authorization !== "Bearer") fail("release upload must use GitHub's Bearer-authenticated upload host");
  if (download.protocol !== "https:" || !PUBLIC_DOWNLOAD_HOSTS.has(download.hostname) || download.port || explicitPort(downloadURL) || download.username || download.password || download.hash) fail("release download host is not approved for headerless public retrieval");
  if (["github.com", "github-releases.githubusercontent.com"].includes(download.hostname) && (download.search || !/^\/[^/]+\/[^/]+\/releases\/download\/[^/]+\/[^/]+$/u.test(download.pathname))) fail("GitHub release download URL is malformed");
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

export async function verifyPublicAssetBytes({ release, manifest, localAssets, downloadDir, fetchImpl = fetch }) {
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
function parseArgs(argv) { const values = {}; for (let index = 0; index < argv.length; index += 1) { const argument = argv[index]; if (!argument.startsWith("--") || !argv[index + 1] || values[argument.slice(2)]) fail("usage"); values[argument.slice(2)] = argv[index + 1]; index += 1; } if (!values.mode) fail("usage"); return values; }
export async function main(argv) {
  const values = parseArgs(argv); const identity = candidateIdentity({ sourceRef: values["source-ref"], sourceCommit: values["source-commit"], version: values.version, tag: values.tag });
  if (values.mode === "preflight") { const result = assertPreflight({ immutableReleases: await readJSON(values["immutable-releases"]), environment: await readJSON(values.environment), branchProtection: await readJSON(values["branch-protection"]) }); process.stdout.write(`${JSON.stringify(result)}\n`); return; }
  if (values.mode !== "receipt" || !values.manifest || !values.release || !values["local-assets"] || !values["download-dir"]) fail("usage");
  const result = await verifyPublicAssetBytes({ release: await readJSON(values.release), manifest: assertManifest(await readJSON(values.manifest), identity), localAssets: await readJSON(values["local-assets"]), downloadDir: values["download-dir"] });
  process.stdout.write(`${JSON.stringify(result)}\n`);
}
if (process.argv[1] === fileURLToPath(import.meta.url)) main(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 1; });
