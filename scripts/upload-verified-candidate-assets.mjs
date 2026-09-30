import { createHash } from "node:crypto";
import { mkdtemp } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { assertManifest, assertPublicationDirectory, candidateIdentity, holdVerifiedLocalAssets, readBoundedRegularLocalAsset, verifyDraftAssetBytes } from "./verify-candidate-publication.mjs";

const REPOSITORY = "service-lasso/service-lasso-tui";
const SHA256 = /^[a-f0-9]{64}$/u;
function fail(message) { throw new Error(message); }
function parseArgs(argv) { const values = {}; for (let index = 0; index < argv.length; index += 1) { const key = argv[index]; const value = argv[index + 1]; if (!key?.startsWith("--") || !value || values[key.slice(2)]) fail("usage"); values[key.slice(2)] = value; index += 1; } return values; }
function fixedNames(version) { return [
  `service-lasso-tui-${version}-win32-amd64.zip`, `service-lasso-tui-${version}-linux-amd64.tar.gz`,
  `service-lasso-tui-${version}-darwin-amd64.tar.gz`, `service-lasso-tui-${version}-darwin-arm64.tar.gz`,
  "SHA256SUMS.txt", "candidate-manifest.json",
]; }
function assetUploadURL(releaseID, name) { return `https://uploads.github.com/repos/${REPOSITORY}/releases/${releaseID}/assets?name=${encodeURIComponent(name)}`; }
function draftReleaseURL(releaseID) { return `https://api.github.com/repos/${REPOSITORY}/releases/${releaseID}`; }
function assertUploadURL(value, releaseID, name) {
  const url = new URL(value);
  if (url.protocol !== "https:" || url.hostname !== "uploads.github.com" || url.port || url.username || url.password || url.hash || url.pathname !== `/repos/${REPOSITORY}/releases/${releaseID}/assets` || url.search !== `?name=${encodeURIComponent(name)}`) fail("candidate upload destination is invalid");
  return url;
}
async function upload(url, bytes, token, fetchImpl) {
  const response = await fetchImpl(url, { method: "POST", redirect: "error", headers: { Accept: "application/vnd.github+json", Authorization: `Bearer ${token}`, "Content-Type": "application/octet-stream" }, body: bytes });
  if (response.status !== 201) fail(`candidate asset upload failed (HTTP ${response.status})`);
}
async function readBoundedMetadata(response) {
  const declared = response.headers?.get?.("content-length");
  if (declared !== null && declared !== undefined && (!/^\d+$/u.test(declared) || Number(declared) > 1024 * 1024)) fail("candidate draft metadata exceeds its bound");
  if (!response.body || typeof response.body[Symbol.asyncIterator] !== "function") fail("candidate draft metadata is missing");
  const chunks = []; let size = 0;
  for await (const chunk of response.body) { const bytes = Buffer.from(chunk); size += bytes.length; if (size > 1024 * 1024) fail("candidate draft metadata exceeds its bound"); chunks.push(bytes); }
  try { return JSON.parse(Buffer.concat(chunks).toString("utf8")); } catch { fail("candidate draft metadata is invalid"); }
}
async function readDraftRelease(releaseID, token, fetchImpl) {
  const response = await fetchImpl(draftReleaseURL(releaseID), { method: "GET", redirect: "error", headers: { Accept: "application/vnd.github+json", Authorization: `Bearer ${token}` } });
  if (response.status !== 200) fail(`candidate draft read failed (HTTP ${response.status})`);
  return readBoundedMetadata(response);
}

export async function uploadVerifiedCandidateAssets({ assetDirectory, localAssets, identity, releaseID, fetchImpl = fetch }) {
  const token = process.env.GH_TOKEN;
  if (process.env.GITHUB_REPOSITORY !== REPOSITORY || !/^[1-9][0-9]*$/u.test(String(releaseID)) || !token) fail("candidate upload authority is invalid");
  if (path.resolve(localAssets) !== path.resolve(assetDirectory, "candidate-local-assets.json")) fail("candidate local inventory path is invalid");
  const manifest = assertManifest(JSON.parse((await readBoundedRegularLocalAsset(assetDirectory, "candidate-manifest.json", 1024 * 1024)).toString("utf8")), identity);
  const inventory = JSON.parse((await readBoundedRegularLocalAsset(assetDirectory, "candidate-local-assets.json", 1024 * 1024)).toString("utf8"));
  await assertPublicationDirectory(assetDirectory, manifest);
  const held = await holdVerifiedLocalAssets({ assetDirectory, manifest, localAssets: inventory });
  for (const name of fixedNames(identity.version)) {
    const bytes = held.bytes[name];
    if (!bytes || createHash("sha256").update(bytes).digest("hex") !== inventory[name].sha256) fail(`candidate asset custody changed for ${name}`);
    await upload(assertUploadURL(assetUploadURL(releaseID, name), releaseID, name), bytes, token, fetchImpl);
  }
  const draft = await readDraftRelease(releaseID, token, fetchImpl);
  const downloadDir = await mkdtemp(path.join(os.tmpdir(), "candidate-draft-receipt-"));
  await verifyDraftAssetBytes({ release: draft, manifest, localAssets: held.inventory, heldBytes: held.bytes, downloadDir, token, fetchImpl });
  return held.inventory;
}

async function main(argv) {
  const values = parseArgs(argv);
  const identity = candidateIdentity({ sourceRef: values["source-ref"], sourceCommit: values["source-commit"], version: values.version, tag: values.tag });
  if (!values["asset-directory"] || !values["local-assets"] || !values["release-id"]) fail("usage");
  await uploadVerifiedCandidateAssets({ assetDirectory: values["asset-directory"], localAssets: values["local-assets"], identity, releaseID: values["release-id"] });
}
if (process.argv[1] === new URL(import.meta.url).pathname) main(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 1; });
