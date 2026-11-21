import { open, readFile } from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { decodeCandidateProfile, decodeNativeGeometry } from "./candidate-profile-source.mjs";

function fail(message) {
  throw new Error(`native geometry production denied: ${message}`);
}

async function readJSON(file, role) {
  let value;
  try {
    value = JSON.parse((await readFile(file)).toString("utf8"));
  } catch {
    fail(`${role} is not valid JSON`);
  }
  if (!value || typeof value !== "object" || Array.isArray(value)) fail(`${role} must be an object`);
  return value;
}

export async function produceNativeGeometry({ profilePath, abiPath, geometryPath, outputPath }) {
  if (![profilePath, abiPath, geometryPath, outputPath].every((value) => typeof value === "string" && path.isAbsolute(value))) {
    fail("absolute profile, ABI, geometry, and output paths are required");
  }
  const profile = await readFile(profilePath);
  decodeCandidateProfile(profile);
  const abi = await readJSON(abiPath, "ABI");
  if (Object.keys(abi).sort().join(",") !== "cSizeTBits,goIntBits" || abi.cSizeTBits !== 64 || abi.goIntBits !== 64) {
    fail("ABI must be the closed amd64 profile");
  }
  const geometry = await readJSON(geometryPath, "geometry");
  decodeNativeGeometry(geometry, abi.cSizeTBits, decodeCandidateProfile(profile).quantities);
  const handle = await open(outputPath, "wx", 0o600);
  try {
    await handle.writeFile(`${JSON.stringify(geometry)}\n`);
    await handle.sync();
  } finally {
    await handle.close();
  }
  return { bytes: Buffer.byteLength(JSON.stringify(geometry)) + 1, bodyBytes: decodeNativeGeometry(geometry, abi.cSizeTBits, decodeCandidateProfile(profile).quantities).values.body_bytes.toString() };
}

async function main() {
  const [profilePath, abiPath, geometryPath, outputPath] = process.argv.slice(2);
  if (!profilePath || !abiPath || !geometryPath || !outputPath || process.argv.length !== 6) {
    process.stderr.write("usage: produce-native-geometry <profile.slcprof1> <abi.json> <geometry.json> <output.json>\n");
    process.exitCode = 2;
    return;
  }
  try {
    const result = await produceNativeGeometry({
      profilePath: path.resolve(profilePath),
      abiPath: path.resolve(abiPath),
      geometryPath: path.resolve(geometryPath),
      outputPath: path.resolve(outputPath),
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
