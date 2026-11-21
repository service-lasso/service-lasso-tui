import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, rm, writeFile, readFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { produceNativeGeometry } from "./produce-native-geometry.mjs";
import { encodeCandidateProfileSections } from "./candidate-profile-source.mjs";

const integer = value => { const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(value); return bytes; };
const records = rows => { const parts = [integer(BigInt(rows.length))]; for (const row of rows) parts.push(...row.map(value => integer(BigInt(value.length))), ...row); return Buffer.concat(parts); };
function profileFixture() {
  const evidence = records([["source", "author-fixture", "subject", "scope", "conditions", "meaning", "original-statement"].map(value => Buffer.from(value))]);
  const role = records([[integer(0n), integer(0n), integer(8n), integer(1n), integer(1n), integer(1n), Buffer.from("actor fixture"), Buffer.from("preservation fixture")]]);
  const policy = Buffer.alloc(72); policy.writeBigUInt64LE(1n, 32);
  return encodeCandidateProfileSections([Buffer.alloc(32, 1), Buffer.alloc(32, 2), policy, Buffer.concat([integer(1n), integer(1n), integer(1n)]), evidence, role, records([]), records([]), records([])].map((bytes, index) => ({ id: index + 1, bytes })));
}
function geometryFixture() {
  const scalar = "scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations".split(" ");
  const result = Object.fromEntries(scalar.map(key => [key, "1"]));
  result.requests = "2";
  for (const key of ["raw_frames", "public_writes", "forward_waits", "forward_iterations"]) result[key] = ["1", "2", "3"];
  result.custody_attempts = ["1", "2", "3", "4", "5"];
  result.final_io_attempts = ["1", "2"];
  return result;
}

async function fixture() {
  const root = await mkdtemp(path.join(os.tmpdir(), "native-geometry-producer-"));
  const profile = path.join(root, "profile.slcprof1");
  const abi = path.join(root, "abi.json");
  const geometry = path.join(root, "geometry.json");
  await writeFile(profile, profileFixture());
  await writeFile(abi, JSON.stringify({ goIntBits: 64, cSizeTBits: 64 }));
  await writeFile(geometry, JSON.stringify(geometryFixture()));
  return { root, profile, abi, geometry, output: path.join(root, "geometry.output.json") };
}

test("produces canonical geometry joined to profile and ABI", async () => {
  const value = await fixture();
  try {
    const result = await produceNativeGeometry({ profilePath: value.profile, abiPath: value.abi, geometryPath: value.geometry, outputPath: value.output });
    assert.match(result.bodyBytes, /^[0-9]+$/);
    const written = await readFile(value.output, "utf8");
    assert.deepEqual(JSON.parse(written), geometryFixture());
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("refuses invalid geometry and existing output", async () => {
  const value = await fixture();
  try {
    const invalid = geometryFixture(); invalid.body_bytes = "2";
    await writeFile(value.geometry, JSON.stringify(invalid));
    await assert.rejects(() => produceNativeGeometry({ profilePath: value.profile, abiPath: value.abi, geometryPath: value.geometry, outputPath: value.output }), /mismatch|denied/);
    await writeFile(value.geometry, JSON.stringify(geometryFixture()));
    await writeFile(value.output, "existing");
    await assert.rejects(() => produceNativeGeometry({ profilePath: value.profile, abiPath: value.abi, geometryPath: value.geometry, outputPath: value.output }), /EEXIST/);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});
