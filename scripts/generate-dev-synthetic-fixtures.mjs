import { createHash } from "node:crypto";
import { mkdir, mkdtemp, readFile, rename, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { decodeCandidateProfile } from "./candidate-profile-source.mjs";
import { produceNativeGeometry } from "./produce-native-geometry.mjs";
import { produceSlcprof1 } from "./produce-slcprof1.mjs";
import { writeNativeInputBundle } from "./native-input-bundle.mjs";
import { SCOPE } from "./scoped-policy.mjs";

const root = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const target = path.join(root, "fixtures/dev-synthetic");
const source = Object.freeze({ repository: "service-lasso/service-lasso-tui", commit: "a".repeat(40), ref: "refs/heads/develop" });
const version = `2026.11.18-${source.commit.slice(0, 7)}`;
const integer = value => { const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(value); return bytes; };
const records = rows => { const parts = [integer(BigInt(rows.length))]; for (const row of rows) parts.push(...row.map(value => integer(BigInt(value.length))), ...row); return Buffer.concat(parts); };

function profileSections() {
  const evidence = records([["source", "author-fixture", "subject", "scope", "conditions", "meaning", "original-statement"].map(value => Buffer.from(value))]);
  const role = records([[integer(0n), integer(0n), integer(8n), integer(1n), integer(1n), integer(1n), Buffer.from("actor fixture"), Buffer.from("preservation fixture")]]);
  const policy = Buffer.alloc(72); policy.writeBigUInt64LE(1n, 32);
  return [Buffer.alloc(32, 1), Buffer.alloc(32, 2), policy, Buffer.concat([integer(1n), integer(1n), integer(1n)]), evidence, role, records([]), records([]), records([])];
}

function geometry() {
  const scalar = "scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations".split(" ");
  const result = Object.fromEntries(scalar.map(key => [key, "1"]));
  result.requests = "2";
  for (const key of ["raw_frames", "public_writes", "forward_waits", "forward_iterations"]) result[key] = ["1", "2", "3"];
  result.custody_attempts = ["1", "2", "3", "4", "5"];
  result.final_io_attempts = ["1", "2"];
  return result;
}

function digest(bytes) { return createHash("sha256").update(bytes).digest("hex"); }

async function generate() {
  const staging = await mkdtemp(path.join(root, "fixtures", ".dev-synthetic-"));
  try {
    const sectionsDir = path.join(staging, "sections");
    await mkdir(sectionsDir);
    const sections = profileSections();
    const spec = { sections: [] };
    for (let index = 0; index < sections.length; index += 1) {
      const relative = `sections/${String(index + 1).padStart(2, "0")}.bin`;
      await writeFile(path.join(staging, relative), sections[index]);
      spec.sections.push({ id: index + 1, path: relative });
    }
    await writeFile(path.join(staging, "sections.json"), `${JSON.stringify(spec, null, 2)}\n`);
    await writeFile(path.join(staging, "abi.json"), `${JSON.stringify({ goIntBits: 64, cSizeTBits: 64 }, null, 2)}\n`);
    await writeFile(path.join(staging, "geometry-source.json"), `${JSON.stringify(geometry(), null, 2)}\n`);

    await produceSlcprof1({ specPath: path.join(staging, "sections.json"), outputPath: path.join(staging, "profile.slcprof1") });
    await produceNativeGeometry({
      profilePath: path.join(staging, "profile.slcprof1"),
      abiPath: path.join(staging, "abi.json"),
      geometryPath: path.join(staging, "geometry-source.json"),
      outputPath: path.join(staging, "geometry.json"),
    });

    const originalsDir = path.join(staging, "originals");
    await mkdir(originalsDir);
    const dummy = ["entry.json", "provider.json", "family.json", "finance.json", "go", "node", "python"];
    for (const name of dummy) await writeFile(path.join(originalsDir, name), `development-synthetic ${name}\n`);

    const profile = await readFile(path.join(staging, "profile.slcprof1"));
    const abi = await readFile(path.join(staging, "abi.json"));
    const geometryBytes = await readFile(path.join(staging, "geometry.json"));
    const quantities = decodeCandidateProfile(profile).quantities;
    const ref = async (name, directory = staging) => {
      const bytes = await readFile(path.join(directory, name));
      return { name, sha256: digest(bytes), size: bytes.length };
    };
    const record = {
      schema: "service-lasso.tui-native-input-bundle.v1",
      scope: SCOPE,
      classification: "development-synthetic",
      source,
      version,
      platform: "linux",
      host: { platform: "linux", arch: "amd64" },
      profile: { ...(await ref("profile.slcprof1")), representationBytes: Number(quantities.representationBytes), bodyBytes: Number(quantities.representationBytes) },
      abi: { ...(await ref("abi.json")), cSizeTBits: 64, goIntBits: 64 },
      geometry: { ...(await ref("geometry.json")), representationBytes: Number(quantities.representationBytes), bodyBytes: Number(quantities.representationBytes), terminalCalls: Number(quantities.terminalCalls), requests: Number(quantities.terminalCalls) + 1, securityLabelBytes: Number(quantities.securityLabelBytes) },
      custody: {
        entry: await ref("entry.json", originalsDir),
        provider: await ref("provider.json", originalsDir),
        family: await ref("family.json", originalsDir),
        finance: await ref("finance.json", originalsDir),
      },
      toolchain: {
        go: await ref("go", originalsDir),
        node: await ref("node", originalsDir),
        python: await ref("python", originalsDir),
      },
      windowsQueryProfile: null,
    };
    const originals = {
      profile,
      abi,
      geometry: geometryBytes,
      custody: {
        entry: await readFile(path.join(originalsDir, "entry.json")),
        provider: await readFile(path.join(originalsDir, "provider.json")),
        family: await readFile(path.join(originalsDir, "family.json")),
        finance: await readFile(path.join(originalsDir, "finance.json")),
      },
      toolchain: {
        go: await readFile(path.join(originalsDir, "go")),
        node: await readFile(path.join(originalsDir, "node")),
        python: await readFile(path.join(originalsDir, "python")),
      },
    };
    await writeFile(path.join(staging, "README.md"), [
      "# Development-synthetic fixtures",
      "",
      "This directory is for local development and tests only.",
      "It is not authorization, issuer custody, native proof, or production input.",
      "Production bundle readers and producers must reject this classification.",
      "",
      "Regenerate with: node scripts/generate-dev-synthetic-fixtures.mjs --force",
      "",
    ].join("\n"));
    await mkdir(path.join(staging, "bundle"), { mode: 0o700 });
    await writeNativeInputBundle({ directory: path.join(staging, "bundle"), record, originals });
    if (process.argv.includes("--force")) await rm(target, { recursive: true, force: true });
    await mkdir(path.dirname(target), { recursive: true });
    await rename(staging, target);
    return target;
  } catch (error) {
    await rm(staging, { recursive: true, force: true }).catch(() => {});
    throw error;
  }
}

const output = await generate();
process.stdout.write(`${JSON.stringify({ directory: output, classification: "development-synthetic" })}\n`);
