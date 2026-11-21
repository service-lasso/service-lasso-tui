import { constants } from "node:fs";
import { open, lstat, readFile } from "node:fs/promises";
import { createHash } from "node:crypto";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { decodeCandidateProfile, encodeCandidateProfileSections } from "./candidate-profile-source.mjs";

const MAX_SECTION_BYTES = 16 * 1024 * 1024;

function fail(message) {
  throw new Error(`SLCPROF1 production denied: ${message}`);
}

function inside(parent, file) {
  const relative = path.relative(parent, file);
  return relative === "" || (!path.isAbsolute(relative) && relative !== ".." && !relative.startsWith(`..${path.sep}`));
}

function parseSpec(value) {
  if (!value || typeof value !== "object" || Array.isArray(value) || Object.keys(value).join(",") !== "sections") {
    fail("spec must contain only a sections array");
  }
  if (!Array.isArray(value.sections) || value.sections.length !== 9) fail("exactly nine sections are required");
  const seen = new Set();
  return value.sections.map((section, index) => {
    if (!section || typeof section !== "object" || Array.isArray(section) || Object.keys(section).sort().join(",") !== "id,path") {
      fail(`section ${index + 1} must contain only id and path`);
    }
    if (section.id !== index + 1) fail(`section ${index + 1} id must be ${index + 1}`);
    if (typeof section.path !== "string" || !section.path || section.path.includes("\0") || path.isAbsolute(section.path)) {
      fail(`section ${index + 1} path must be relative`);
    }
    if (seen.has(section.path)) fail("section path is duplicated");
    seen.add(section.path);
    return { id: section.id, path: section.path };
  });
}

async function readSection(root, relative) {
  const file = path.resolve(root, relative);
  if (!inside(root, file) || file === root) fail("section path escapes the spec directory");
  const before = await lstat(file);
  if (before.isSymbolicLink() || !before.isFile() || before.size <= 0 || before.size > MAX_SECTION_BYTES) {
    fail("section path must be a bounded regular non-symlink file");
  }
  const handle = await open(file, constants.O_RDONLY | constants.O_NOFOLLOW);
  try {
    const stat = await handle.stat();
    if (!stat.isFile() || stat.size !== before.size) fail("section file changed during production");
    const bytes = await handle.readFile();
    if (bytes.length !== stat.size) fail("section file readback changed");
    return bytes;
  } finally {
    await handle.close();
  }
}

export async function produceSlcprof1({ specPath, outputPath }) {
  if (typeof specPath !== "string" || typeof outputPath !== "string" || !path.isAbsolute(specPath) || !path.isAbsolute(outputPath)) {
    fail("absolute spec and output paths are required");
  }
  const specBytes = await readFile(specPath);
  let spec;
  try {
    spec = JSON.parse(specBytes.toString("utf8"));
  } catch {
    fail("spec is not valid JSON");
  }
  const sections = parseSpec(spec);
  const root = path.dirname(specPath);
  const originals = [];
  for (const section of sections) {
    originals.push({ id: section.id, bytes: await readSection(root, section.path) });
  }
  const profile = encodeCandidateProfileSections(originals);
  decodeCandidateProfile(profile);
  const handle = await open(outputPath, "wx", 0o600);
  try {
    await handle.writeFile(profile);
    await handle.sync();
  } finally {
    await handle.close();
  }
  return { bytes: profile.length, sha256: createHash("sha256").update(profile).digest("hex") };
}

async function main() {
  const [specPath, outputPath] = process.argv.slice(2);
  if (!specPath || !outputPath || process.argv.length !== 4) {
    process.stderr.write("usage: produce-slcprof1 <sections.json> <profile.slcprof1>\n");
    process.exitCode = 2;
    return;
  }
  try {
    const result = await produceSlcprof1({ specPath: path.resolve(specPath), outputPath: path.resolve(outputPath) });
    process.stdout.write(`${JSON.stringify(result)}\n`);
  } catch (error) {
    process.stderr.write(`${error.message}\n`);
    process.exitCode = 1;
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) {
  await main();
}
