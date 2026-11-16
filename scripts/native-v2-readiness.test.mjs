// UNRUN source fixtures only. Synthetic custody and digests are never authority.
import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, writeFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { assessNativeV2Readiness, assessNativeV2ReadinessFromFiles, READINESS_GROUPS } from "./native-v2-readiness.mjs";

const SCALAR = "scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations".split(" ");
const ABI = Object.freeze({ goIntBits: 64, cSizeTBits: 64 });
const CUSTODY_IDS = ["constructors", "windows-inputs", "matching-host-ownership", "admission", "core-integration"];

function nativeFixture() {
  const result = Object.fromEntries(SCALAR.map(key => [key, "1"]));
  result.requests = "2";
  for (const key of ["raw_frames", "public_writes", "forward_waits", "forward_iterations"]) result[key] = ["1", "2", "3"];
  result.custody_attempts = ["1", "2", "3", "4", "5"];
  result.final_io_attempts = ["1", "2"];
  return result;
}
const u64 = value => { const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(value); return bytes; };
const records = rows => {
  const parts = [u64(BigInt(rows.length))];
  for (const row of rows) parts.push(...row.map(value => u64(BigInt(value.length))), ...row);
  return Buffer.concat(parts);
};
function profileFixture() {
  const evidence = records([["source", "author-fixture", "subject", "scope", "conditions", "meaning", "original-statement"].map(value => Buffer.from(value))]);
  const role = records([[u64(0n), u64(0n), u64(8n), u64(1n), u64(1n), u64(1n), Buffer.from("actor fixture"), Buffer.from("preservation fixture")]]);
  const policy = Buffer.alloc(72); policy.writeBigUInt64LE(1n, 32);
  const sections = [Buffer.alloc(32, 1), Buffer.alloc(32, 2), policy, Buffer.concat([u64(1n), u64(1n), u64(1n)]), evidence, role, records([]), records([]), records([])];
  let total = 32; for (const bytes of sections) total += 16 + bytes.length;
  const output = Buffer.alloc(total); Buffer.from([0x53, 0x4c, 0x43, 0x50, 0x52, 0x4f, 0x46, 0x31]).copy(output);
  output.writeUInt32LE(1, 8); output.writeUInt32LE(9, 12); output.writeBigUInt64LE(BigInt(total), 16);
  let at = 32;
  sections.forEach((bytes, index) => { output.writeUInt32LE(index + 1, at); output.writeBigUInt64LE(BigInt(bytes.length), at + 8); bytes.copy(output, at + 16); at += 16 + bytes.length; });
  return output;
}
function custodyFixture() {
  return Object.fromEntries(CUSTODY_IDS.map(id => [id, { owner: `${id}-owner`, evidenceSha256: "a".repeat(64), custodyRef: `custody/${id}` }]));
}
const full = () => ({ profile: profileFixture(), abi: { ...ABI }, nativeGeometry: nativeFixture(), custody: custodyFixture() });
const stateOf = (result, id) => result.groups.find(row => row.id === id).state;

test("an empty input is not admitted and every required group stays unavailable", () => {
  const result = assessNativeV2Readiness();
  assert.equal(result.admitted, false);
  assert.equal(result.disposition, "NOT_READY_NOT_ADMITTED");
  assert.deepEqual(result.groups.map(row => row.id), READINESS_GROUPS.map(row => row.id));
  assert.ok(result.groups.every(row => row.state === "UNAVAILABLE"));
  assert.equal(result.blockers.length, READINESS_GROUPS.length);
  assert.equal(Object.hasOwn(result, "nativePass"), false);
  assert.equal(JSON.stringify(result).includes("success"), false);
});

test("valid profile ABI and geometry are ready but absent custody still refuses admission", () => {
  const result = assessNativeV2Readiness({ profile: profileFixture(), abi: { ...ABI }, nativeGeometry: nativeFixture() });
  assert.equal(result.admitted, false);
  for (const id of ["slcprof1-profile", "go-c-abi", "native-geometry"]) assert.equal(stateOf(result, id), "READY");
  for (const id of CUSTODY_IDS) assert.equal(stateOf(result, id), "UNAVAILABLE");
  assert.equal(result.blockers.length, CUSTODY_IDS.length);
});

test("a complete consistent bundle is admitted for review only", () => {
  const result = assessNativeV2Readiness(full());
  assert.equal(result.admitted, true);
  assert.equal(result.disposition, "ADMITTED_FOR_REVIEW");
  assert.deepEqual(result.blockers, []);
  assert.ok(result.groups.every(row => row.state === "READY"));
});

test("a malformed profile or ABI is invalid and blocks geometry", () => {
  const truncated = profileFixture().subarray(0, 20);
  const profile = assessNativeV2Readiness({ ...full(), profile: truncated });
  assert.equal(stateOf(profile, "slcprof1-profile"), "INVALID");
  assert.equal(profile.admitted, false);

  for (const abi of [{ goIntBits: 16, cSizeTBits: 64 }, { goIntBits: 64, cSizeTBits: 64, extra: true }, [], { goIntBits: "64", cSizeTBits: 64 }]) {
    const result = assessNativeV2Readiness({ ...full(), abi });
    assert.equal(stateOf(result, "go-c-abi"), "INVALID");
    assert.equal(stateOf(result, "native-geometry"), "UNAVAILABLE");
    assert.equal(result.admitted, false);
  }
});

test("geometry must be closed and joined to the profile quantities", () => {
  const cases = [];
  const missing = nativeFixture(); delete missing.services; cases.push(missing);
  cases.push({ ...nativeFixture(), unknown: "1" });
  cases.push({ ...nativeFixture(), services: "01" });
  cases.push({ ...nativeFixture(), body_bytes: "2" });
  cases.push({ ...nativeFixture(), requests: "1" });
  cases.push({ ...nativeFixture(), terminal_calls: "2" });
  const short = nativeFixture(); short.raw_frames = ["1", "2"]; cases.push(short);
  const sparse = nativeFixture(); delete sparse.raw_frames[1]; cases.push(sparse);
  for (const nativeGeometry of cases) {
    const result = assessNativeV2Readiness({ ...full(), nativeGeometry });
    assert.equal(stateOf(result, "native-geometry"), "INVALID");
    assert.equal(result.admitted, false);
  }
  const orphan = assessNativeV2Readiness({ abi: { ...ABI }, nativeGeometry: nativeFixture(), custody: custodyFixture() });
  assert.equal(stateOf(orphan, "native-geometry"), "UNAVAILABLE");
});

test("custody records must be closed, owned, referenced and digest-bound", () => {
  const mutations = [
    entry => ({ ...entry, extra: "1" }),
    entry => { const next = { ...entry }; delete next.owner; return next; },
    entry => ({ ...entry, owner: "" }),
    entry => ({ ...entry, custodyRef: "" }),
    entry => ({ ...entry, evidenceSha256: "A".repeat(64) }),
    entry => ({ ...entry, evidenceSha256: "a".repeat(63) }),
  ];
  for (const id of CUSTODY_IDS) {
    for (const mutate of mutations) {
      const custody = custodyFixture(); custody[id] = mutate(custody[id]);
      const result = assessNativeV2Readiness({ ...full(), custody });
      assert.equal(stateOf(result, id), "INVALID", `${id} accepted a mutated custody record`);
      assert.equal(result.admitted, false);
    }
  }
});

test("file assessment and CLI exit codes fail closed", async () => {
  const root = await mkdtemp(path.join(tmpdir(), "tui37-readiness-"));
  try {
    const bundle = full();
    const files = {
      profile: path.join(root, "profile.bin"), abi: path.join(root, "abi.json"),
      geometry: path.join(root, "geometry.json"), custody: path.join(root, "custody.json"),
    };
    await writeFile(files.profile, bundle.profile);
    await writeFile(files.abi, JSON.stringify(bundle.abi));
    await writeFile(files.geometry, JSON.stringify(bundle.nativeGeometry));
    await writeFile(files.custody, JSON.stringify(bundle.custody));
    const admitted = await assessNativeV2ReadinessFromFiles(files);
    assert.equal(admitted.admitted, true);

    const cli = fileURLToPath(new URL("./native-v2-readiness.mjs", import.meta.url));
    const run = args => spawnSync(process.execPath, [cli, ...args], { encoding: "utf8" });
    const green = run(["--profile", files.profile, "--abi", files.abi, "--geometry", files.geometry, "--custody", files.custody]);
    assert.equal(green.status, 0); assert.equal(JSON.parse(green.stdout).admitted, true);
    const empty = run([]);
    assert.equal(empty.status, 1); assert.equal(JSON.parse(empty.stdout).disposition, "NOT_READY_NOT_ADMITTED");
    assert.equal(run(["--bogus", "x"]).status, 2);
  } finally {
    await rm(root, { recursive: true, force: true });
  }
});
