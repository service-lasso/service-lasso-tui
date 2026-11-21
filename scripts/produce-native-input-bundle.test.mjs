import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, rm, writeFile, mkdir, readFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { produceNativeInputBundle } from "./produce-native-input-bundle.mjs";
import { digest, NATIVE_INPUT_FILE, readNativeInputBundle } from "./native-input-bundle.mjs";
import { encodeCandidateProfileSections } from "./candidate-profile-source.mjs";
import { SCOPE } from "./scoped-policy.mjs";

const source = Object.freeze({ repository: "service-lasso/service-lasso-tui", commit: "a".repeat(40), ref: "refs/heads/develop" });
const version = `2026.11.18-${source.commit.slice(0, 7)}`;
const integer = value => { const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(value); return bytes; };
const records = rows => { const parts = [integer(BigInt(rows.length))]; for (const row of rows) parts.push(...row.map(value => integer(BigInt(value.length))), ...row); return Buffer.concat(parts); };
function profileFixture() {
  const evidence = records([["source", "author-fixture", "subject", "scope", "conditions", "meaning", "original-statement"].map(value => Buffer.from(value))]);
  const role = records([[integer(0n), integer(0n), integer(8n), integer(1n), integer(1n), integer(1n), Buffer.from("actor fixture"), Buffer.from("preservation fixture")]]);
  const policy = Buffer.alloc(72); policy.writeBigUInt64LE(1n, 32);
  return encodeCandidateProfileSections([Buffer.alloc(32, 1), Buffer.alloc(32, 2), policy, Buffer.concat([integer(1n), integer(1n), integer(1n)]), evidence, role, records([]), records([]), records([])].map((bytes, index) => ({ id: index + 1, bytes })));
}
function nativeFixture() {
  const scalar = "scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations".split(" ");
  const result = Object.fromEntries(scalar.map(key => [key, "1"]));
  result.requests = "2";
  for (const key of ["raw_frames", "public_writes", "forward_waits", "forward_iterations"]) result[key] = ["1", "2", "3"];
  result.custody_attempts = ["1", "2", "3", "4", "5"];
  result.final_io_attempts = ["1", "2"];
  return result;
}

async function fixture(classification = "authorization") {
  const root = await mkdtemp(path.join(os.tmpdir(), "native-bundle-producer-"));
  const originals = path.join(root, "originals");
  await mkdir(originals);
  const bytes = {
    "profile.slcprof1": profileFixture(),
    "abi.json": Buffer.from(JSON.stringify({ goIntBits: 64, cSizeTBits: 64 })),
    "geometry.json": Buffer.from(JSON.stringify(nativeFixture())),
    ...Object.fromEntries(["entry.json", "provider.json", "family.json", "finance.json", "go", "node", "python"].map(name => [name, Buffer.from(`original:${name}\n`)])),
  };
  for (const [name, value] of Object.entries(bytes)) await writeFile(path.join(originals, name), value);
  const ref = name => ({ name, sha256: digest(bytes[name]), size: bytes[name].length });
  const record = {
    schema: "service-lasso.tui-native-input-bundle.v1",
    scope: SCOPE,
    classification,
    source,
    version,
    platform: "linux",
    host: { platform: "linux", arch: "amd64" },
    profile: { ...ref("profile.slcprof1"), representationBytes: 1, bodyBytes: 1 },
    abi: { ...ref("abi.json"), cSizeTBits: 64, goIntBits: 64 },
    geometry: { ...ref("geometry.json"), representationBytes: 1, bodyBytes: 1, terminalCalls: 1, requests: 2, securityLabelBytes: 1 },
    custody: { entry: ref("entry.json"), provider: ref("provider.json"), family: ref("family.json"), finance: ref("finance.json") },
    toolchain: { go: ref("go"), node: ref("node"), python: ref("python") },
    windowsQueryProfile: null,
  };
  const recordPath = path.join(root, "native-inputs.json");
  await writeFile(recordPath, JSON.stringify(record));
  return { root, originals, recordPath, output: path.join(root, "bundle") };
}

test("produces and reloads an authorization bundle", async () => {
  const value = await fixture();
  try {
    const result = await produceNativeInputBundle({ recordPath: value.recordPath, originalsDirectory: value.originals, outputDirectory: value.output, requireAuthorization: true });
    assert.equal(result.classification, "authorization");
    assert.equal(result.platform, "linux");
    const loaded = await readNativeInputBundle({ directory: value.output, expected: { source, version, platform: "linux" }, requireAuthorization: true });
    assert.equal(loaded.record.classification, "authorization");
    assert.equal(loaded.record.profile.name, "profile.slcprof1");
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("refuses development-synthetic when authorization is required", async () => {
  const value = await fixture("development-synthetic");
  try {
    await assert.rejects(() => produceNativeInputBundle({ recordPath: value.recordPath, originalsDirectory: value.originals, outputDirectory: value.output, requireAuthorization: true }), /authorization record required/);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("refuses an existing output directory and removes a failed output", async () => {
  const value = await fixture();
  try {
    await mkdir(value.output);
    await assert.rejects(() => produceNativeInputBundle({ recordPath: value.recordPath, originalsDirectory: value.originals, outputDirectory: value.output, requireAuthorization: true }), /EEXIST/);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
  const missing = await fixture();
  try {
    await rm(path.join(missing.originals, "finance.json"));
    await assert.rejects(() => produceNativeInputBundle({ recordPath: missing.recordPath, originalsDirectory: missing.originals, outputDirectory: missing.output, requireAuthorization: true }), /ENOENT/);
    await assert.rejects(() => readNativeInputBundle({ directory: missing.output }), /denied/);
  } finally {
    await rm(missing.root, { recursive: true, force: true });
  }
});

test("bundle file name is stable", async () => {
  const value = await fixture();
  try {
    await produceNativeInputBundle({ recordPath: value.recordPath, originalsDirectory: value.originals, outputDirectory: value.output, requireAuthorization: true });
    await readFile(path.join(value.output, NATIVE_INPUT_FILE));
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});
