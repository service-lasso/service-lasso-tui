import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";

const SHA256 = /^[a-f0-9]{64}$/u;
const SHA1 = /^[a-f0-9]{40}$/u;
const REQUIRED_CHECKS = Object.freeze([
  "Linux test and build",
  "Windows test and build",
  "macOS test and build",
  "Release asset cross-compilation",
]);
const REQUIRED_PLATFORMS = Object.freeze(["darwin-amd64", "darwin-arm64", "linux-amd64", "win32-amd64"]);
const PUBLIC_DOWNLOAD_HOSTS = new Set(["github.com", "github-releases.githubusercontent.com", "objects.githubusercontent.com"]);

function fail(message) { throw new Error(message); }

function object(value, name) {
  if (!value || typeof value !== "object" || Array.isArray(value)) fail(`${name} must be an object`);
  return value;
}

function exactKeys(value, keys, name) {
  const actual = Object.keys(object(value, name)).sort();
  const expected = [...keys].sort();
  if (actual.length !== expected.length || actual.some((key, index) => key !== expected[index])) fail(`${name} has an unexpected shape`);
}

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
  for (const asset of manifest.assets) {
    if (typeof asset.name !== "string" || !SHA256.test(asset.sha256) || !["service-lasso-tui", "service-lasso-tui.exe"].includes(asset.executable)) fail("candidate manifest asset is invalid");
  }
  return manifest;
}

export function assertReleaseReceipt(release, manifest, localAssets) {
  object(release, "release");
  if (release.tag_name !== manifest.release.tag || release.target_commitish !== manifest.source.commit || release.draft !== false || release.prerelease !== true || release.immutable !== true) fail("release does not prove the immutable exact candidate identity");
  if (!Array.isArray(release.assets)) fail("release assets are missing");
  const requiredNames = [...manifest.assets.map(asset => asset.name), manifest.checksumManifest.name, "candidate-manifest.json"].sort();
  const names = release.assets.map(asset => asset?.name).sort();
  if (names.length !== requiredNames.length || names.some((name, index) => name !== requiredNames[index])) fail("release asset inventory is incomplete or unexpected");
  for (const name of requiredNames) {
    const remote = release.assets.find(asset => asset.name === name);
    const localDigest = localAssets?.[name];
    if (!SHA256.test(localDigest) || remote?.digest !== `sha256:${localDigest}` || !Number.isInteger(remote?.size) || remote.size <= 0) fail(`release asset receipt is invalid for ${name}`);
  }
  return { tag: release.tag_name, commit: release.target_commitish, immutable: release.immutable, assets: requiredNames };
}

export function assertExistingCandidateRecovery(release, manifest, localAssets) {
  return assertReleaseReceipt(release, manifest, localAssets);
}

export function assertTransportPolicy({ uploadURL, downloadURL, authorization }) {
  const upload = new URL(uploadURL);
  const download = new URL(downloadURL);
  if (upload.protocol !== "https:" || upload.hostname !== "uploads.github.com" || authorization !== "Bearer") fail("release upload must use GitHub's Bearer-authenticated upload host");
  if (download.protocol !== "https:" || !PUBLIC_DOWNLOAD_HOSTS.has(download.hostname) || download.username || download.password) fail("release download host is not approved for headerless public retrieval");
  return true;
}

async function readJSON(file) { return JSON.parse(await readFile(file, "utf8")); }

function parseArgs(argv) {
  const values = {};
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    if (!argument.startsWith("--") || !argv[index + 1]) fail("usage");
    values[argument.slice(2)] = argv[index + 1]; index += 1;
  }
  if (!values.mode) fail("usage");
  return values;
}

export async function main(argv) {
  const values = parseArgs(argv);
  const identity = candidateIdentity({ sourceRef: values["source-ref"], sourceCommit: values["source-commit"], version: values.version, tag: values.tag });
  if (values.mode === "preflight") {
    const result = assertPreflight({ immutableReleases: await readJSON(values["immutable-releases"]), environment: await readJSON(values.environment), branchProtection: await readJSON(values["branch-protection"]) });
    process.stdout.write(`${JSON.stringify(result)}\n`); return;
  }
  const manifest = assertManifest(await readJSON(values.manifest), identity);
  const receipt = assertReleaseReceipt(await readJSON(values.release), manifest, await readJSON(values["local-assets"]));
  process.stdout.write(`${JSON.stringify(receipt)}\n`);
}

if (process.argv[1] === fileURLToPath(import.meta.url)) main(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 1; });
