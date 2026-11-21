import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, rm, writeFile, readFile, mkdir } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { produceSlcprof1 } from "./produce-slcprof1.mjs";
import { decodeCandidateProfile } from "./candidate-profile-source.mjs";

function integer(value) { const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(value); return bytes; }
function records(rows) { const parts = [integer(BigInt(rows.length))]; for (const row of rows) parts.push(...row.map(value => integer(BigInt(value.length))), ...row); return Buffer.concat(parts); }

function sections() {
  const evidence = records([["source", "author-fixture", "subject", "scope", "conditions", "meaning", "original-statement"].map(value => Buffer.from(value))]);
  const role = records([[integer(0n), integer(0n), integer(8n), integer(1n), integer(1n), integer(1n), Buffer.from("actor fixture"), Buffer.from("preservation fixture")]]);
  const policy = Buffer.alloc(72); policy.writeBigUInt64LE(1n, 32);
  return [
    Buffer.alloc(32, 1),
    Buffer.alloc(32, 2),
    policy,
    Buffer.concat([integer(1n), integer(1n), integer(1n)]),
    evidence,
    role,
    records([]),
    records([]),
    records([]),
  ];
}

async function fixture() {
  const root = await mkdtemp(path.join(os.tmpdir(), "slcprof1-producer-"));
  const source = path.join(root, "sections");
  await mkdir(source);
  const values = sections();
  const spec = { sections: values.map((bytes, index) => {
    const name = `section-${index + 1}.bin`;
    return { id: index + 1, path: `sections/${name}`, bytes };
  }) };
  for (const row of spec.sections) await writeFile(path.join(root, row.path), row.bytes);
  await writeFile(path.join(root, "sections.json"), JSON.stringify({ sections: spec.sections.map(({ id, path: file }) => ({ id, path: file })) }));
  return { root, values, specPath: path.join(root, "sections.json"), output: path.join(root, "profile.slcprof1") };
}

test("produces a canonical SLCPROF1 from durable section files", async () => {
  const value = await fixture();
  try {
    const result = await produceSlcprof1({ specPath: value.specPath, outputPath: value.output });
    const profile = await readFile(value.output);
    assert.equal(result.bytes, profile.length);
    assert.match(result.sha256, /^[a-f0-9]{64}$/);
    const decoded = decodeCandidateProfile(profile);
    assert.deepEqual(decoded.sections.map(section => section.bytes), value.values);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("refuses to overwrite an existing profile", async () => {
  const value = await fixture();
  try {
    await writeFile(value.output, "existing");
    await assert.rejects(() => produceSlcprof1({ specPath: value.specPath, outputPath: value.output }), /EEXIST/);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("refuses traversal, duplicate, and missing sections", async () => {
  const value = await fixture();
  try {
    const spec = JSON.parse(await readFile(value.specPath, "utf8"));
    spec.sections[0].path = "../escape.bin";
    await writeFile(value.specPath, JSON.stringify(spec));
    await assert.rejects(() => produceSlcprof1({ specPath: value.specPath, outputPath: value.output }), /escapes|denied/);

    spec.sections[0].path = "sections/section-1.bin";
    spec.sections[1].path = "sections/section-1.bin";
    await writeFile(value.specPath, JSON.stringify(spec));
    await assert.rejects(() => produceSlcprof1({ specPath: value.specPath, outputPath: value.output }), /duplicated/);

    spec.sections[1].path = "sections/missing.bin";
    await writeFile(value.specPath, JSON.stringify(spec));
    await assert.rejects(() => produceSlcprof1({ specPath: value.specPath, outputPath: value.output }), /ENOENT/);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});
