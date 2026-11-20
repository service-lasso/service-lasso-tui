import assert from "node:assert/strict";
import { mkdtemp, rm, symlink, writeFile, unlink } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { assertNativeInputBundle, digest, NATIVE_INPUT_FILE, nativeInputRef, readNativeInputBundle, writeNativeInputBundle } from "./native-input-bundle.mjs";
import { encodeCandidateProfileSections } from "./candidate-profile-source.mjs";
import { SCOPE } from "./scoped-policy.mjs";

const source = Object.freeze({ repository: "service-lasso/service-lasso-tui", commit: "a".repeat(40), ref: "refs/heads/develop" });
const version = `2026.11.18-${source.commit.slice(0, 7)}`;

function integer(value) { const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(value); return bytes; }
function records(rows) { const parts = [integer(BigInt(rows.length))]; for (const row of rows) parts.push(...row.map(value => integer(BigInt(value.length))), ...row); return Buffer.concat(parts); }
function profileFixture() {
  const evidence = records([["source", "author-fixture", "subject", "scope", "conditions", "meaning", "original-statement"].map(value => Buffer.from(value))]);
  const role = records([[integer(0n), integer(0n), integer(8n), integer(1n), integer(1n), integer(1n), Buffer.from("actor fixture"), Buffer.from("preservation fixture")]]);
  const policy = Buffer.alloc(72); policy.writeBigUInt64LE(1n, 32);
  return encodeCandidateProfileSections([Buffer.alloc(32, 1), Buffer.alloc(32, 2), policy, Buffer.concat([integer(1n), integer(1n), integer(1n)]), evidence, role, records([]), records([]), records([])].map((bytes, index) => ({ id: index + 1, bytes })));
}
function nativeFixture() {
  const scalar = "scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations".split(" ");
  const result = Object.fromEntries(scalar.map(key => [key, "1"])); result.requests = "2"; for (const key of ["raw_frames", "public_writes", "forward_waits", "forward_iterations"]) result[key] = ["1", "2", "3"]; result.custody_attempts = ["1", "2", "3", "4", "5"]; result.final_io_attempts = ["1", "2"]; return result;
}
const abiFixture = () => ({ goIntBits: 64, cSizeTBits: 64 });

async function fixture({ platform = "linux", query = false } = {}) {
  const directory = await mkdtemp(path.join(os.tmpdir(), "tui28-native-inputs-"));
  const bytes = {
    "profile.slcprof1": profileFixture(),
    "abi.json": Buffer.from(JSON.stringify(abiFixture())),
    "geometry.json": Buffer.from(JSON.stringify(nativeFixture())),
    ...Object.fromEntries([
      "entry.json", "provider.json", "family.json", "finance.json", "go", "node", "python", ...(query ? ["windows-query.json"] : []),
    ].map(name => [name, Buffer.from(`original:${name}\n`)])),
  };
  for (const [name, value] of Object.entries(bytes)) await writeFile(path.join(directory, name), value);
  const ref = name => ({ name, sha256: digest(bytes[name]), size: bytes[name].length });
  const record = {
    schema: "service-lasso.tui-native-input-bundle.v1",
    scope: SCOPE,
    classification: "development-synthetic",
    source,
    version,
    platform,
    host: { platform, arch: "amd64" },
    profile: { ...ref("profile.slcprof1"), representationBytes: 1, bodyBytes: 1 },
    abi: { ...ref("abi.json"), cSizeTBits: 64, goIntBits: 64 },
    geometry: { ...ref("geometry.json"), representationBytes: 1, bodyBytes: 1, terminalCalls: 1, requests: 2, securityLabelBytes: 1 },
    custody: { entry: ref("entry.json"), provider: ref("provider.json"), family: ref("family.json"), finance: ref("finance.json") },
    toolchain: { go: ref("go"), node: ref("node"), python: ref("python") },
    windowsQueryProfile: query ? ref("windows-query.json") : null,
  };
  await writeFile(path.join(directory, NATIVE_INPUT_FILE), JSON.stringify(record));
  return { directory, record, bytes };
}

async function read(directory, expected = {}) {
  return readNativeInputBundle({ directory, expected: { source, version, platform: "linux", ...expected } });
}

async function rejectsMutation(mutate, expected = {}) {
  const fixtureValue = await fixture();
  try {
    const record = JSON.parse(JSON.stringify(fixtureValue.record));
    mutate(record, fixtureValue.directory);
    await writeFile(path.join(fixtureValue.directory, NATIVE_INPUT_FILE), JSON.stringify(record));
    await assert.rejects(() => read(fixtureValue.directory, expected), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
}

test("loads a closed Linux bundle and its originals", async () => {
  const fixtureValue = await fixture();
  try {
    const loaded = await read(fixtureValue.directory);
    assert.deepEqual(loaded.record, fixtureValue.record);
    assert.equal(loaded.files.profile.toString(), fixtureValue.bytes["profile.slcprof1"].toString());
    assert.equal(loaded.files.abi.toString(), fixtureValue.bytes["abi.json"].toString());
    assert.equal(loaded.files.geometry.toString(), fixtureValue.bytes["geometry.json"].toString());
    assert.equal(loaded.files.custody.entry.toString(), fixtureValue.bytes["entry.json"].toString());
    assert.equal(loaded.files.windowsQueryProfile, null);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("accepts a closed Windows bundle shape", async () => {
  const fixtureValue = await fixture({ platform: "win32", query: true });
  try {
    assert.equal(assertNativeInputBundle(fixtureValue.record, { source, version, platform: "win32" }).platform, "win32");
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("loads a Windows bundle with a query profile", { skip: process.platform !== "win32" }, async () => {
  const fixtureValue = await fixture({ platform: "win32", query: true });
  try {
    const loaded = await readNativeInputBundle({ directory: fixtureValue.directory, expected: { source, version, platform: "win32" } });
    assert.equal(loaded.record.platform, "win32");
    assert.equal(loaded.files.windowsQueryProfile.toString(), fixtureValue.bytes["windows-query.json"].toString());
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a bundle for another host", { skip: process.platform === "win32" }, async () => {
  const fixtureValue = await fixture({ platform: "win32", query: true });
  try {
    await assert.rejects(() => readNativeInputBundle({ directory: fixtureValue.directory, expected: { source, version, platform: "win32" } }), /actual host/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a missing bundle", async () => {
  const directory = await mkdtemp(path.join(os.tmpdir(), "tui28-native-inputs-"));
  try {
    await assert.rejects(() => read(directory), /native input bundle denied/);
  } finally {
    await rm(directory, { recursive: true, force: true });
  }
});

test("denies a missing original", async () => {
  const fixtureValue = await fixture();
  try {
    await unlink(path.join(fixtureValue.directory, "abi.json"));
    await assert.rejects(() => read(fixtureValue.directory), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a digest mismatch", async () => {
  await rejectsMutation(record => { record.profile.sha256 = "b".repeat(64); });
});

test("denies a size mismatch", async () => {
  await rejectsMutation(record => { record.profile.size += 1; });
});

test("denies a profile body/representation mismatch", async () => {
  await rejectsMutation(record => { record.profile.bodyBytes = 2; });
});

test("denies a geometry request mismatch", async () => {
  await rejectsMutation(record => { record.geometry.requests = 3; });
});

test("denies a profile/geometry byte join mismatch", async () => {
  await rejectsMutation(record => { record.geometry.bodyBytes = 2; record.geometry.representationBytes = 2; });
});

test("denies coherent manifest quantities that disagree with the original profile", async () => {
  const fixtureValue = await fixture();
  try {
    const record = JSON.parse(JSON.stringify(fixtureValue.record));
    record.profile.representationBytes = 2;
    record.profile.bodyBytes = 2;
    record.geometry.representationBytes = 2;
    record.geometry.bodyBytes = 2;
    await writeFile(path.join(fixtureValue.directory, NATIVE_INPUT_FILE), JSON.stringify(record));
    await assert.rejects(() => read(fixtureValue.directory), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a non-amd64 ABI", async () => {
  await rejectsMutation(record => { record.abi.cSizeTBits = 32; });
});

test("denies a host/platform mismatch", async () => {
  await rejectsMutation(record => { record.host.platform = "win32"; });
});

test("denies a Windows bundle without a query profile", async () => {
  const fixtureValue = await fixture({ platform: "win32", query: true });
  try {
    const record = JSON.parse(JSON.stringify(fixtureValue.record));
    record.windowsQueryProfile = null;
    await writeFile(path.join(fixtureValue.directory, NATIVE_INPUT_FILE), JSON.stringify(record));
    await assert.rejects(() => readNativeInputBundle({ directory: fixtureValue.directory }), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a Linux bundle with a Windows query profile", async () => {
  const fixtureValue = await fixture({ platform: "linux", query: true });
  try {
    const record = JSON.parse(JSON.stringify(fixtureValue.record));
    record.windowsQueryProfile = record.profile;
    await writeFile(path.join(fixtureValue.directory, NATIVE_INPUT_FILE), JSON.stringify(record));
    await assert.rejects(() => readNativeInputBundle({ directory: fixtureValue.directory }), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies aliased originals", async () => {
  await rejectsMutation(record => { record.profile.name = record.abi.name; });
});

test("denies path traversal", async () => {
  await rejectsMutation(record => { record.profile.name = "../profile.slcprof1"; });
});

test("denies unknown bundle keys", async () => {
  await rejectsMutation(record => { record.extra = true; });
});

test("denies duplicate JSON keys", async () => {
  const fixtureValue = await fixture();
  try {
    const text = JSON.stringify(fixtureValue.record).replace('"schema":', '"schema":"service-lasso.tui-native-input-bundle.v1","schema":');
    await writeFile(path.join(fixtureValue.directory, NATIVE_INPUT_FILE), text);
    await assert.rejects(() => read(fixtureValue.directory), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a source that does not match the expected authority", async () => {
  await rejectsMutation(() => {}, { source: { ...source, commit: "b".repeat(40) } });
});

test("denies a foreign source repository", async () => {
  await rejectsMutation(record => { record.source.repository = "other/repo"; });
});

test("denies a symlinked original", async () => {
  const fixtureValue = await fixture();
  try {
    await symlink(path.join(fixtureValue.directory, "profile.slcprof1"), path.join(fixtureValue.directory, "profile-link"));
    const record = JSON.parse(JSON.stringify(fixtureValue.record));
    record.profile.name = "profile-link";
    await writeFile(path.join(fixtureValue.directory, NATIVE_INPUT_FILE), JSON.stringify(record));
    await assert.rejects(() => read(fixtureValue.directory), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a development bundle when authorization is required", async () => {
  const fixtureValue = await fixture();
  try {
    await assert.rejects(() => readNativeInputBundle({ directory: fixtureValue.directory, expected: { source, version, platform: "linux" }, requireAuthorization: true }), /authorization input bundle required/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
  }
});

test("denies a missing classification", async () => {
  await rejectsMutation(record => { delete record.classification; });
});

test("denies an unknown classification", async () => {
  await rejectsMutation(record => { record.classification = "native"; });
});

test("writes a verified authorization bundle and reloads it", async () => {
  const fixtureValue = await fixture();
  const directory = await mkdtemp(path.join(os.tmpdir(), "tui28-native-written-"));
  try {
    const record = { ...fixtureValue.record, classification: "authorization" };
    const originals = {
      profile: fixtureValue.bytes["profile.slcprof1"],
      abi: fixtureValue.bytes["abi.json"],
      geometry: fixtureValue.bytes["geometry.json"],
      custody: {
        entry: fixtureValue.bytes["entry.json"],
        provider: fixtureValue.bytes["provider.json"],
        family: fixtureValue.bytes["family.json"],
        finance: fixtureValue.bytes["finance.json"],
      },
      toolchain: {
        go: fixtureValue.bytes.go,
        node: fixtureValue.bytes.node,
        python: fixtureValue.bytes.python,
      },
    };
    await writeNativeInputBundle({ directory, record, originals });
    const loaded = await readNativeInputBundle({ directory, expected: { source, version, platform: "linux" }, requireAuthorization: true });
    assert.equal(loaded.record.classification, "authorization");
    assert.equal(loaded.files.profile.toString(), originals.profile.toString());
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
    await rm(directory, { recursive: true, force: true });
  }
});

test("writer derives a reference from original bytes", () => {
  const bytes = Buffer.from("original");
  assert.deepEqual(nativeInputRef("original.bin", bytes), { name: "original.bin", sha256: digest(bytes), size: bytes.length });
});

test("writer denies original bytes that do not match the record", async () => {
  const fixtureValue = await fixture();
  const directory = await mkdtemp(path.join(os.tmpdir(), "tui28-native-written-"));
  try {
    const originals = {
      profile: Buffer.from("wrong"),
      abi: fixtureValue.bytes["abi.json"],
      geometry: fixtureValue.bytes["geometry.json"],
      custody: {
        entry: fixtureValue.bytes["entry.json"],
        provider: fixtureValue.bytes["provider.json"],
        family: fixtureValue.bytes["family.json"],
        finance: fixtureValue.bytes["finance.json"],
      },
      toolchain: {
        go: fixtureValue.bytes.go,
        node: fixtureValue.bytes.node,
        python: fixtureValue.bytes.python,
      },
    };
    await assert.rejects(() => writeNativeInputBundle({ directory, record: fixtureValue.record, originals }), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
    await rm(directory, { recursive: true, force: true });
  }
});

test("writer refuses to overwrite an existing original", async () => {
  const fixtureValue = await fixture();
  const directory = await mkdtemp(path.join(os.tmpdir(), "tui28-native-written-"));
  try {
    const originals = {
      profile: fixtureValue.bytes["profile.slcprof1"],
      abi: fixtureValue.bytes["abi.json"],
      geometry: fixtureValue.bytes["geometry.json"],
      custody: {
        entry: fixtureValue.bytes["entry.json"],
        provider: fixtureValue.bytes["provider.json"],
        family: fixtureValue.bytes["family.json"],
        finance: fixtureValue.bytes["finance.json"],
      },
      toolchain: {
        go: fixtureValue.bytes.go,
        node: fixtureValue.bytes.node,
        python: fixtureValue.bytes.python,
      },
    };
    await writeFile(path.join(directory, "profile.slcprof1"), "existing");
    await assert.rejects(() => writeNativeInputBundle({ directory, record: fixtureValue.record, originals }), /native input bundle denied/);
  } finally {
    await rm(fixtureValue.directory, { recursive: true, force: true });
    await rm(directory, { recursive: true, force: true });
  }
});
