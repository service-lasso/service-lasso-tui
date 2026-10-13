// UNRUN source cases. Synthetic image framing is not a compiled/loaded native
// owner, authenticated publisher, bootstrap issuer, or platform qualification.
import test from "node:test";
import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { gzipSync, gunzipSync } from "node:zlib";
import { SCOPE } from "./scoped-policy.mjs";
import { nativeOwnerLayout, createNativeOwnerArchive, verifyNativeOwnerArchive,
  nativeOwnerMemberInventory, assertNativeOwnerAsset, assertNativeOwnerManifest } from "./native-owner-package.mjs";
const hash = bytes => createHash("sha256").update(bytes).digest("hex");
function image(platform, marker) {
  const bytes = Buffer.alloc(128); bytes[127] = marker;
  if (platform === "linux-amd64") { bytes.write("7f454c46", 0, "hex"); bytes[4] = 2; bytes[5] = 1; bytes.writeUInt16LE(62, 18); }
  else { bytes.write("MZ"); bytes.writeUInt32LE(64, 60); bytes.writeUInt32LE(0x00004550, 64); bytes.writeUInt16LE(0x8664, 68); bytes.writeUInt16LE(0x20b, 88); }
  return bytes;
}
function fixture(platform) {
  const members = nativeOwnerLayout(platform).map((item, index) => ({ ...item, bytes: image(platform, index + 1) }));
  const archive = createNativeOwnerArchive(platform, members);
  const asset = { platform, name: `service-lasso-tui-2026.10.11-1111111-${platform}.${platform === "linux-amd64" ? "tar.gz" : "zip"}`,
    sha256: hash(archive), executable: members[0].path, members: nativeOwnerMemberInventory(platform, members) };
  return { members, archive, asset };
}
for (const platform of ["linux-amd64", "win32-amd64"]) {
  test(`${platform}: exact public/private bodies and distinct member authentication`, () => {
    const { members, archive, asset } = fixture(platform);
    const held = assertNativeOwnerAsset(asset, archive);
    assert.deepEqual(held.map(item => item.path), members.map(item => item.path));
    for (const [index, item] of held.entries()) assert.deepEqual(item.bytes, members[index].bytes);
    const replacement = members.map(item => ({ ...item, bytes: Buffer.from(item.bytes) }));
    replacement[1].bytes[127] = 9;
    const changed = createNativeOwnerArchive(platform, replacement);
    assert.throws(() => assertNativeOwnerAsset({ ...asset, sha256: hash(changed) }, changed));
    assert.throws(() => verifyNativeOwnerArchive(platform, Buffer.concat([archive, Buffer.from([0])])));
    assert.throws(() => createNativeOwnerArchive(platform, [...members, members[0]]));
    assert.throws(() => createNativeOwnerArchive(platform, [members[1], members[0]]));
    assert.throws(() => createNativeOwnerArchive(platform, [members[0], { ...members[1], path: "../service-lasso-tui-go" }]));
    assert.throws(() => assertNativeOwnerAsset({ ...asset, members: [asset.members[0], { ...asset.members[1], role: "native-owner" }] }, archive));
  });
}
test("tar links and duplicate path cannot become extracted native members", () => {
  const { archive } = fixture("linux-amd64");
  const linked = gunzipSync(archive); linked[156] = 50;
  assert.throws(() => verifyNativeOwnerArchive("linux-amd64", gzipSync(linked)));
  const duplicate = gunzipSync(archive);
  // First synthetic body takes one block: second header starts at1024.
  duplicate.fill(0, 1024, 1124); duplicate.write("service-lasso-tui", 1024);
  assert.throws(() => verifyNativeOwnerArchive("linux-amd64", gzipSync(duplicate)));
});
test("schema4 requires both member hashes and exact candidate source/layout", () => {
  const identity = { sourceRef: "refs/heads/develop", sourceCommit: "1".repeat(40), version: "2026.10.11-1111111", tag: "candidate-2026.10.11-1111111" };
  const manifest = { schemaVersion: 4, kind: "develop-prerelease-candidate", version: identity.version,
    source: { repository: "service-lasso/service-lasso-tui", ref: identity.sourceRef, commit: identity.sourceCommit },
    release: { tag: identity.tag, draft: false, prerelease: true, immutable: true },
    checksumManifest: { name: "SHA256SUMS.txt", sha256: "2".repeat(64) }, corePackagingIssue: "service-lasso/service-lasso#1461", scope: SCOPE,
    assets: [fixture("linux-amd64").asset, fixture("win32-amd64").asset] };
  assert.equal(assertNativeOwnerManifest(manifest, identity), manifest);
  assert.throws(() => assertNativeOwnerManifest({ ...manifest, schemaVersion: 3 }, identity));
  assert.throws(() => assertNativeOwnerManifest({ ...manifest, assets: [...manifest.assets].reverse() }, identity));
  assert.throws(() => assertNativeOwnerManifest({ ...manifest, source: { ...manifest.source, commit: "3".repeat(40) } }, identity));
  assert.throws(() => assertNativeOwnerManifest({ ...manifest, assets: [{ ...manifest.assets[0], members: [manifest.assets[0].members[0]] }, manifest.assets[1]] }, identity));
});
