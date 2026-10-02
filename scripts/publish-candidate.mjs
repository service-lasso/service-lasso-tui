import { mkdtemp } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { assertManifest, assertPreflight, assertPublicationDirectory, candidateIdentity, holdVerifiedLocalAssets, readBoundedRegularLocalAsset, verifyDraftAssetBytes, verifyPublicAssetBytes } from "./verify-candidate-publication.mjs";

const ROOT = "https://api.github.com/repos/service-lasso/service-lasso-tui";
const SHA = /^[a-f0-9]{40}$/u;
const fail = message => { throw new Error(message); };
const canonical = value => JSON.stringify(value && typeof value === "object" ? Array.isArray(value) ? value.map(item => JSON.parse(canonical(item))) : Object.fromEntries(Object.keys(value).sort().map(key => [key, JSON.parse(canonical(value[key]))])) : value);

// One deadline covers response headers AND the entire streamed body. Never log
// provider exception messages: they may contain credential-bearing URLs.
export async function providerJSON(fetchImpl, token, route, { method = "GET", body, optional = false } = {}) {
  if (!/^\/(?:immutable-releases|environments\/development-candidate|branches\/develop\/protection|git\/refs|git\/ref\/tags\/candidate-[a-zA-Z0-9.-]+|git\/tags\/[a-f0-9]{40}|releases(?:\/tags\/candidate-[a-zA-Z0-9.-]+|\/[1-9][0-9]*)?)$/u.test(route)) fail("provider route denied");
  const controller = new AbortController();
  let timer;
  const deadline = new Promise((_, reject) => { timer = setTimeout(() => { controller.abort(); reject(new Error("provider deadline exceeded")); }, 30000); });
  try {
    return await Promise.race([deadline, (async () => {
      const response = await fetchImpl(`${ROOT}${route}`, { method, redirect: "error", signal: controller.signal, headers: { Accept: "application/vnd.github+json", Authorization: `Bearer ${token}`, ...(body ? { "Content-Type": "application/json" } : {}) }, ...(body ? { body: JSON.stringify(body) } : {}) });
      if (optional && response.status === 404) return null;
      if (response.status !== (method === "POST" ? 201 : 200)) fail("provider response denied");
      const declared = response.headers?.get?.("content-length");
      if (declared != null && (!/^\d+$/u.test(declared) || Number(declared) > 1048576)) fail("provider metadata bound exceeded");
      if (!response.body?.[Symbol.asyncIterator]) fail("provider metadata missing");
      const chunks = []; let size = 0;
      for await (const chunk of response.body) { const bytes = Buffer.from(chunk); size += bytes.length; if (size > 1048576) fail("provider metadata bound exceeded"); chunks.push(bytes); }
      return JSON.parse(Buffer.concat(chunks).toString("utf8"));
    })()]);
  } catch { fail("provider request or metadata denied"); } finally { clearTimeout(timer); controller.abort(); }
}

export async function resolveCandidateTag({ identity, read }) {
  const ref = await read(`/git/ref/tags/${identity.tag}`, { optional: true });
  if (ref === null) return null;
  if (ref?.ref !== `refs/tags/${identity.tag}`) fail("candidate tag ref malformed");
  let object = ref.object; const visited = new Set(); const chain = [];
  for (let depth = 0; depth <= 16; depth += 1) {
    if (!object || !SHA.test(object.sha) || !["commit", "tag"].includes(object.type) || visited.has(object.sha)) fail("candidate tag object malformed or cyclic");
    visited.add(object.sha); chain.push({ type: object.type, sha: object.sha });
    if (object.type === "commit") {
      if (object.sha !== identity.sourceCommit) fail("candidate tag source mismatch");
      return { ref: ref.ref, commit: object.sha, chain };
    }
    if (depth === 16) fail("candidate tag depth exceeded");
    const tag = await read(`/git/tags/${object.sha}`);
    if (tag?.sha !== object.sha) fail("candidate annotated tag identity malformed");
    object = tag.object;
  }
  fail("candidate tag unresolved");
}

export async function publishCandidate({ assetDirectory, identity, token, fetchImpl = fetch, beforeHeldAcquisition }) {
  candidateIdentity(identity);
  if (!token) fail("publisher authority missing");
  const admittedManifestBytes = await readBoundedRegularLocalAsset(assetDirectory, "candidate-manifest.json", 1048576);
  const manifest = assertManifest(JSON.parse(admittedManifestBytes.toString("utf8")), identity);
  // Instrument the actual acquisition boundary without replacing any validator.
  if (beforeHeldAcquisition) await beforeHeldAcquisition();
  const inventory = JSON.parse((await readBoundedRegularLocalAsset(assetDirectory, "candidate-local-assets.json", 1048576)).toString("utf8"));
  await assertPublicationDirectory(assetDirectory, manifest);
  const held = await holdVerifiedLocalAssets({ assetDirectory, manifest, localAssets: inventory });
  if (!held.bytes["candidate-manifest.json"].equals(admittedManifestBytes)) fail("held candidate manifest differs from admitted bytes");
  const read = (route, options) => providerJSON(fetchImpl, token, route, options);
  const policy = async () => {
    const value = { immutableReleases: await read("/immutable-releases"), environment: await read("/environments/development-candidate"), branchProtection: await read("/branches/develop/protection") };
    assertPreflight(value); return canonical(value);
  };
  const approved = await policy();
  const beforeWrite = async () => { if (await policy() !== approved) fail("approved provider policy changed"); };
  const tagProof = () => resolveCandidateTag({ identity, read });
  const existing = await read(`/releases/tags/${identity.tag}`, { optional: true });
  const originalTag = await tagProof();
  const publicReceipt = async release => {
    const proof = await tagProof(); if (!proof) fail("candidate tag missing");
    const receipt = await verifyPublicAssetBytes({ release, manifest, localAssets: held.inventory, assetDirectory, downloadDir: await mkdtemp(path.join(os.tmpdir(), "candidate-public-")), fetchImpl });
    // Detect ref movement across the complete byte receipt too.
    const finalProof = await tagProof(); if (canonical(finalProof) !== canonical(proof)) fail("candidate tag changed during receipt");
    return { ...receipt, tagProof: finalProof };
  };
  if (existing) return { recovered: true, release: existing, receipt: await publicReceipt(existing) };
  if (originalTag) fail("orphan candidate tag collision");
  // Recheck both collision surfaces immediately before the sole creation.
  if (await read(`/releases/tags/${identity.tag}`, { optional: true }) || await tagProof()) fail("candidate creation collision");
  await beforeWrite();
  await read("/git/refs", { method: "POST", body: { ref: `refs/tags/${identity.tag}`, sha: identity.sourceCommit } });
  if (!await tagProof()) fail("new candidate tag missing");
  await beforeWrite();
  const created = await read("/releases", { method: "POST", body: { tag_name: identity.tag, target_commitish: identity.sourceCommit, name: `TUI candidate ${identity.version}`, draft: true, prerelease: true, make_latest: "false", generate_release_notes: false } });
  if (!Number.isSafeInteger(created?.id) || created.id <= 0 || created.tag_name !== identity.tag || created.target_commitish !== identity.sourceCommit || created.draft !== true || created.prerelease !== true || !Array.isArray(created.assets) || created.assets.length !== 0) fail("created draft identity denied");
  if (!await tagProof()) fail("created candidate tag missing");
  for (const name of Object.keys(held.inventory).sort()) {
    if (!await tagProof()) fail("candidate tag missing before upload");
    await beforeWrite();
    const controller = new AbortController(); let timer;
    try {
      await Promise.race([(async () => {
        const response = await fetchImpl(`https://uploads.github.com/repos/service-lasso/service-lasso-tui/releases/${created.id}/assets?name=${encodeURIComponent(name)}`, { method: "POST", redirect: "error", signal: controller.signal, headers: { Authorization: `Bearer ${token}`, Accept: "application/vnd.github+json", "Content-Type": "application/octet-stream" }, body: held.bytes[name] });
        if (response.status !== 201) fail("candidate upload denied");
      })(), new Promise((_, reject) => { timer = setTimeout(() => { controller.abort(); reject(new Error("candidate upload deadline")); }, 30000); })]);
    } catch { fail("candidate upload denied"); } finally { clearTimeout(timer); controller.abort(); }
  }
  const draft = await read(`/releases/${created.id}`);
  if (draft?.id !== created.id) fail("draft numeric identity mismatch");
  await verifyDraftAssetBytes({ release: draft, manifest, localAssets: held.inventory, heldBytes: held.bytes, downloadDir: await mkdtemp(path.join(os.tmpdir(), "candidate-draft-")), token, fetchImpl });
  if (!await tagProof()) fail("candidate tag missing before publication");
  await beforeWrite();
  await read(`/releases/${created.id}`, { method: "PATCH", body: { draft: false } });
  const final = await read(`/releases/tags/${identity.tag}`);
  if (final?.id !== created.id) fail("published numeric identity mismatch");
  return { recovered: false, release: final, receipt: await publicReceipt(final) };
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const values = {}; for (let i = 2; i < process.argv.length; i += 2) { const key = process.argv[i]; if (!key?.startsWith("--") || !process.argv[i + 1] || values[key]) fail("publisher usage"); values[key] = process.argv[i + 1]; }
  if (process.env.GITHUB_REPOSITORY !== "service-lasso/service-lasso-tui") fail("publisher repository denied");
  publishCandidate({ assetDirectory: values["--asset-directory"], identity: candidateIdentity({ sourceRef: values["--source-ref"], sourceCommit: values["--source-commit"], version: values["--version"], tag: values["--tag"] }), token: process.env.GH_TOKEN }).then(result => process.stdout.write(`${JSON.stringify(result)}\n`)).catch(() => { process.stderr.write("candidate publication denied\n"); process.exitCode = 1; });
}
