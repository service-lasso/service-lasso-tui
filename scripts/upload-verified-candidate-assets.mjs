import { spawn } from "node:child_process";
import { createHash } from "node:crypto";
import path from "node:path";
import { assertManifest, assertPublicationDirectory, candidateIdentity, holdVerifiedLocalAssets, readBoundedRegularLocalAsset } from "./verify-candidate-publication.mjs";

const REPOSITORY = "service-lasso/service-lasso-tui";
const SHA256 = /^[a-f0-9]{64}$/u;
function fail(message) { throw new Error(message); }
function parseArgs(argv) { const values = {}; for (let index = 0; index < argv.length; index += 1) { const key = argv[index]; const value = argv[index + 1]; if (!key?.startsWith("--") || !value || values[key.slice(2)]) fail("usage"); values[key.slice(2)] = value; index += 1; } return values; }
function fixedNames(version) { return [
  `service-lasso-tui-${version}-win32-amd64.zip`, `service-lasso-tui-${version}-linux-amd64.tar.gz`,
  `service-lasso-tui-${version}-darwin-amd64.tar.gz`, `service-lasso-tui-${version}-darwin-arm64.tar.gz`,
  "SHA256SUMS.txt", "candidate-manifest.json",
]; }
async function upload(url, bytes) {
  await new Promise((resolve, reject) => {
    const child = spawn("curl", ["--fail", "--silent", "--show-error", "--request", "POST", "--header", "Accept: application/vnd.github+json", "--header", `Authorization: Bearer ${process.env.GH_TOKEN ?? ""}`, "--header", "Content-Type: application/octet-stream", "--data-binary", "@-", url], { stdio: ["pipe", "ignore", "pipe"], windowsHide: true });
    let stderr = ""; child.stderr.on("data", chunk => { stderr += String(chunk); });
    child.once("error", reject); child.once("close", code => code === 0 ? resolve() : reject(new Error(`candidate asset upload failed (${code}): ${stderr.slice(0, 512)}`)));
    child.stdin.end(bytes);
  });
}

export async function uploadVerifiedCandidateAssets({ assetDirectory, localAssets, identity, releaseID, upload = upload }) {
  if (process.env.GITHUB_REPOSITORY !== REPOSITORY || !/^[1-9][0-9]*$/u.test(String(releaseID)) || !process.env.GH_TOKEN) fail("candidate upload authority is invalid");
  if (path.resolve(localAssets) !== path.resolve(assetDirectory, "candidate-local-assets.json")) fail("candidate local inventory path is invalid");
  const manifest = assertManifest(JSON.parse((await readBoundedRegularLocalAsset(assetDirectory, "candidate-manifest.json", 1024 * 1024)).toString("utf8")), identity);
  const inventory = JSON.parse((await readBoundedRegularLocalAsset(assetDirectory, "candidate-local-assets.json", 1024 * 1024)).toString("utf8"));
  await assertPublicationDirectory(assetDirectory, manifest);
  const held = await holdVerifiedLocalAssets({ assetDirectory, manifest, localAssets: inventory });
  for (const name of fixedNames(identity.version)) {
    const bytes = held.bytes[name];
    if (!bytes || createHash("sha256").update(bytes).digest("hex") !== inventory[name].sha256) fail(`candidate asset custody changed for ${name}`);
    await upload(`https://uploads.github.com/repos/${REPOSITORY}/releases/${releaseID}/assets?name=${encodeURIComponent(name)}`, bytes);
  }
  return held.inventory;
}

async function main(argv) {
  const values = parseArgs(argv);
  const identity = candidateIdentity({ sourceRef: values["source-ref"], sourceCommit: values["source-commit"], version: values.version, tag: values.tag });
  if (!values["asset-directory"] || !values["local-assets"] || !values["release-id"]) fail("usage");
  await uploadVerifiedCandidateAssets({ assetDirectory: values["asset-directory"], localAssets: values["local-assets"], identity, releaseID: values["release-id"] });
}
if (process.argv[1] === new URL(import.meta.url).pathname) main(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.message}\n`); process.exitCode = 1; });
