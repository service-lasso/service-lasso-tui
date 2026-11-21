import { constants } from "node:fs";
import { mkdir, open, readFile, rmdir } from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import {
  assertNativeInputBundle,
  NATIVE_INPUT_FILE,
  readNativeInputBundle,
  writeNativeInputBundle,
} from "./native-input-bundle.mjs";

const MAX_ORIGINAL_BYTES = 1024 * 1024 * 1024;

function fail(message) {
  throw new Error(`native input bundle production denied: ${message}`);
}

function requiredNames(record) {
  const names = [
    record.profile.name,
    record.abi.name,
    record.geometry.name,
    record.custody.entry.name,
    record.custody.provider.name,
    record.custody.family.name,
    record.custody.finance.name,
    record.toolchain.go.name,
    record.toolchain.node.name,
    record.toolchain.python.name,
  ];
  if (record.platform === "win32") names.push(record.windowsQueryProfile.name);
  return names;
}

async function readOriginal(directory, name) {
  if (path.basename(name) !== name || name === "." || name === "..") fail("original name is not a leaf");
  const file = path.resolve(directory, name);
  const relative = path.relative(directory, file);
  if (path.isAbsolute(relative) || relative === ".." || relative.startsWith(`..${path.sep}`)) {
    fail("original path escapes the originals directory");
  }
  const handle = await open(file, constants.O_RDONLY | constants.O_NOFOLLOW);
  try {
    const stat = await handle.stat();
    if (!stat.isFile() || stat.size <= 0 || stat.size > MAX_ORIGINAL_BYTES) {
      fail("original must be a bounded regular non-symlink file");
    }
    const bytes = await handle.readFile();
    if (bytes.length !== stat.size) fail("original readback changed");
    return bytes;
  } finally {
    await handle.close();
  }
}

export async function produceNativeInputBundle({
  recordPath,
  originalsDirectory,
  outputDirectory,
  requireAuthorization = false,
}) {
  if (![recordPath, originalsDirectory, outputDirectory].every((value) => typeof value === "string" && path.isAbsolute(value))) {
    fail("absolute record, originals, and output paths are required");
  }
  const recordBytes = await readFile(recordPath);
  let record;
  try {
    record = JSON.parse(recordBytes.toString("utf8"));
  } catch {
    fail("record is not valid JSON");
  }
  assertNativeInputBundle(record);
  if (requireAuthorization && record.classification !== "authorization") {
    fail("authorization record required");
  }
  const originals = { custody: {}, toolchain: {} };
  const originalsDirectoryPath = path.resolve(originalsDirectory);
  const read = (name) => readOriginal(originalsDirectoryPath, name);
  originals.profile = await read(record.profile.name);
  originals.abi = await read(record.abi.name);
  originals.geometry = await read(record.geometry.name);
  originals.custody.entry = await read(record.custody.entry.name);
  originals.custody.provider = await read(record.custody.provider.name);
  originals.custody.family = await read(record.custody.family.name);
  originals.custody.finance = await read(record.custody.finance.name);
  originals.toolchain.go = await read(record.toolchain.go.name);
  originals.toolchain.node = await read(record.toolchain.node.name);
  originals.toolchain.python = await read(record.toolchain.python.name);
  if (record.platform === "win32") originals.windowsQueryProfile = await read(record.windowsQueryProfile.name);

  const output = path.resolve(outputDirectory);
  let created = false;
  try {
    await mkdir(output, { mode: 0o700 });
    created = true;
    await writeNativeInputBundle({ directory: output, record, originals });
    const loaded = await readNativeInputBundle({
      directory: output,
      expected: { source: record.source, version: record.version, platform: record.platform },
      requireAuthorization,
    });
    return {
      classification: loaded.record.classification,
      platform: loaded.record.platform,
      files: requiredNames(loaded.record).length + 1,
      bundle: path.join(output, NATIVE_INPUT_FILE),
    };
  } catch (error) {
    if (created) await rmdir(output).catch(() => {});
    throw error;
  }
}

async function main() {
  const args = process.argv.slice(2);
  const requireAuthorization = args.includes("--authorization");
  const values = args.filter((value) => value !== "--authorization");
  if (values.length !== 3) {
    process.stderr.write("usage: produce-native-input-bundle <record.json> <originals-dir> <output-dir> [--authorization]\n");
    process.exitCode = 2;
    return;
  }
  try {
    const result = await produceNativeInputBundle({
      recordPath: path.resolve(values[0]),
      originalsDirectory: path.resolve(values[1]),
      outputDirectory: path.resolve(values[2]),
      requireAuthorization,
    });
    process.stdout.write(`${JSON.stringify(result)}\n`);
  } catch (error) {
    process.stderr.write(`${error.message}\n`);
    process.exitCode = 1;
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) {
  await main();
}
