import { createHash } from "node:crypto";
import { unlink, writeFile } from "node:fs/promises";
import path from "node:path";
import { assertSource } from "./scoped-native-contract.mjs";
import { assertScope, assertSourcePolicy, SCOPE } from "./scoped-policy.mjs";
import { parseStrictJSON } from "./scoped-json.mjs";
import { candidateIdentity, readBoundedRegularLocalAsset } from "./verify-scoped-candidate-publication.mjs";
import { decodeCandidateProfile, decodeNativeGeometry } from "./candidate-profile-source.mjs";

export const NATIVE_INPUT_SCHEMA = "service-lasso.tui-native-input-bundle.v1";
export const NATIVE_INPUT_FILE = "native-inputs.json";
export const NATIVE_INPUT_PLATFORMS = Object.freeze(["win32", "linux"]);
export const NATIVE_INPUT_CLASSIFICATIONS = Object.freeze(["authorization", "development-synthetic"]);
const MAX_BUNDLE_BYTES = 1024 * 1024;
const MAX_INPUT_BYTES = 1024 * 1024 * 1024;
const SHA256 = /^[a-f0-9]{64}$/u;
const NAME = /^[A-Za-z0-9._-]{1,200}$/u;
const fail = message => { throw new Error(`native input bundle denied: ${message}`); };
const denied = error => {
  if (typeof error?.message === "string" && error.message.startsWith("native input bundle denied:")) return error;
  return new Error(`native input bundle denied: ${error?.message ?? "invalid"}`);
};
const closed = (value, keys, name) => {
  if (!value || typeof value !== "object" || Array.isArray(value)) fail(`${name} must be an object`);
  const actual = Object.keys(value).sort();
  const expected = [...keys].sort();
  if (actual.length !== expected.length || actual.some((key, index) => key !== expected[index])) fail(`${name} has an unexpected shape`);
  return value;
};
const positive = (value, name) => { if (!Number.isSafeInteger(value) || value <= 0) fail(`${name} is invalid`); return value; };
const nonnegative = (value, name) => { if (!Number.isSafeInteger(value) || value < 0) fail(`${name} is invalid`); return value; };
const byteRef = (value, name) => {
  closed(value, ["name", "sha256", "size"], name);
  if (typeof value.name !== "string" || !NAME.test(value.name) || !SHA256.test(value.sha256)) fail(`${name} is invalid`);
  positive(value.size, `${name}.size`);
  return value;
};
export const digest = bytes => createHash("sha256").update(bytes).digest("hex");

function assertProfile(value) {
  closed(value, ["name", "sha256", "size", "representationBytes", "bodyBytes"], "profile");
  byteRef({ name: value.name, sha256: value.sha256, size: value.size }, "profile");
  positive(value.representationBytes, "profile.representationBytes");
  positive(value.bodyBytes, "profile.bodyBytes");
  if (value.bodyBytes !== value.representationBytes) fail("profile body bytes do not equal representation bytes");
}

function assertAbi(value) {
  closed(value, ["name", "sha256", "size", "cSizeTBits", "goIntBits"], "abi");
  byteRef({ name: value.name, sha256: value.sha256, size: value.size }, "abi");
  if (value.cSizeTBits !== 64 || value.goIntBits !== 64) fail("abi is not the closed amd64 profile");
}

function assertGeometry(value) {
  closed(value, ["name", "sha256", "size", "representationBytes", "bodyBytes", "terminalCalls", "requests", "securityLabelBytes"], "geometry");
  byteRef({ name: value.name, sha256: value.sha256, size: value.size }, "geometry");
  positive(value.representationBytes, "geometry.representationBytes");
  positive(value.bodyBytes, "geometry.bodyBytes");
  nonnegative(value.terminalCalls, "geometry.terminalCalls");
  positive(value.requests, "geometry.requests");
  nonnegative(value.securityLabelBytes, "geometry.securityLabelBytes");
  if (value.bodyBytes !== value.representationBytes) fail("geometry body bytes do not equal representation bytes");
  if (value.requests !== value.terminalCalls + 1) fail("geometry request count does not bind terminal calls");
}

function assertCustody(value) {
  closed(value, ["entry", "provider", "family", "finance"], "custody");
  for (const key of ["entry", "provider", "family", "finance"]) byteRef(value[key], `custody.${key}`);
}

function assertToolchain(value) {
  closed(value, ["go", "node", "python"], "toolchain");
  for (const key of ["go", "node", "python"]) byteRef(value[key], `toolchain.${key}`);
}

function assertNativeInputBundleInner(value, expected = {}) {
  closed(value, ["schema", "scope", "source", "version", "platform", "host", "classification", "profile", "abi", "geometry", "custody", "toolchain", "windowsQueryProfile"], "bundle");
  if (value.schema !== NATIVE_INPUT_SCHEMA) fail("schema is not current");
  if (!NATIVE_INPUT_CLASSIFICATIONS.includes(value.classification)) fail("classification is not scoped");
  assertScope(value.scope);
  assertSource(value.source);
  candidateIdentity({ sourceRef: value.source.ref, sourceCommit: value.source.commit, version: value.version, tag: `candidate-${value.version}` });
  if (!NATIVE_INPUT_PLATFORMS.includes(value.platform)) fail("platform is not scoped");
  closed(value.host, ["platform", "arch"], "host");
  if (value.host.platform !== value.platform || value.host.arch !== "amd64") fail("host is not the scoped native platform");
  if (expected.source && JSON.stringify(expected.source) !== JSON.stringify(value.source)) fail("source does not match the expected authority");
  if (expected.version && expected.version !== value.version) fail("version does not match the expected authority");
  if (expected.platform && expected.platform !== value.platform) fail("platform does not match the expected authority");
  assertProfile(value.profile);
  assertAbi(value.abi);
  assertGeometry(value.geometry);
  assertCustody(value.custody);
  assertToolchain(value.toolchain);
  if (value.platform === "win32") byteRef(value.windowsQueryProfile, "windowsQueryProfile");
  else if (value.windowsQueryProfile !== null) fail("Linux bundle must not carry a Windows query profile");
  if (value.profile.representationBytes !== value.geometry.representationBytes || value.profile.bodyBytes !== value.geometry.bodyBytes) fail("profile and geometry bytes do not agree");
  const names = [
    value.profile.name, value.abi.name, value.geometry.name,
    value.custody.entry.name, value.custody.provider.name, value.custody.family.name, value.custody.finance.name,
    value.toolchain.go.name, value.toolchain.node.name, value.toolchain.python.name,
    ...(value.platform === "win32" ? [value.windowsQueryProfile.name] : []),
  ];
  if (new Set(names).size !== names.length) fail("input originals are aliased");
  return value;
}

export function assertNativeInputBundle(value, expected = {}) {
  try { return assertNativeInputBundleInner(value, expected); } catch (error) { throw denied(error); }
}

export async function readNativeInputBundle({ directory, expected = {}, requireAuthorization = false } = {}) {
  if (typeof directory !== "string" || !directory) fail("directory is required");
  await assertSourcePolicy();
  const read = async (name, maxBytes) => {
    try { return await readBoundedRegularLocalAsset(directory, name, maxBytes); } catch (error) { throw denied(error); }
  };
  let parsed;
  try { parsed = parseStrictJSON(await read(NATIVE_INPUT_FILE, MAX_BUNDLE_BYTES)); } catch (error) { throw denied(error); }
  const record = assertNativeInputBundle(parsed, expected);
  if (requireAuthorization && record.classification !== "authorization") fail("authorization input bundle required");
  if (record.host.platform !== process.platform || record.host.arch !== "amd64" || process.arch !== "x64") fail("bundle host is not the actual host");
  const entries = [
    ["profile", record.profile],
    ["abi", record.abi],
    ["geometry", record.geometry],
    ["custody.entry", record.custody.entry],
    ["custody.provider", record.custody.provider],
    ["custody.family", record.custody.family],
    ["custody.finance", record.custody.finance],
    ["toolchain.go", record.toolchain.go],
    ["toolchain.node", record.toolchain.node],
    ["toolchain.python", record.toolchain.python],
    ...(record.platform === "win32" ? [["windowsQueryProfile", record.windowsQueryProfile]] : []),
  ];
  const files = {};
  for (const [role, ref] of entries) {
    const bytes = await read(ref.name, MAX_INPUT_BYTES);
    if (bytes.length !== ref.size || digest(bytes) !== ref.sha256) fail(`${role} original bytes do not match the bundle`);
    const parts = role.split(".");
    let target = files;
    for (const part of parts.slice(0, -1)) target = target[part] ??= {};
    target[parts.at(-1)] = bytes;
  }
  if (record.platform === "linux") files.windowsQueryProfile = null;
  let profile, abi, geometry, nativeGeometry;
  try {
    profile = decodeCandidateProfile(files.profile);
    abi = parseStrictJSON(files.abi);
    closed(abi, ["cSizeTBits", "goIntBits"], "abi original");
    if (abi.cSizeTBits !== record.abi.cSizeTBits || abi.goIntBits !== record.abi.goIntBits) fail("abi original does not match the bundle");
    geometry = parseStrictJSON(files.geometry);
    nativeGeometry = decodeNativeGeometry(geometry, abi.cSizeTBits, profile.quantities);
  } catch (error) { throw denied(error); }
  const quantities = profile.quantities;
  for (const value of Object.values(quantities)) if (value > BigInt(Number.MAX_SAFE_INTEGER)) fail("profile quantity is not representable");
  if (Number(quantities.representationBytes) !== record.profile.representationBytes ||
      Number(quantities.representationBytes) !== record.profile.bodyBytes ||
      Number(nativeGeometry.values.body_bytes) !== record.geometry.bodyBytes ||
      Number(quantities.representationBytes) !== record.geometry.representationBytes ||
      Number(quantities.terminalCalls) !== record.geometry.terminalCalls ||
      Number(quantities.securityLabelBytes) !== record.geometry.securityLabelBytes ||
      Number(nativeGeometry.values.requests) !== record.geometry.requests) fail("original profile and geometry do not match the bundle quantities");
  return { record, files, profile, abi, geometry, nativeGeometry };
}

export { SCOPE };

export function nativeInputRef(name, bytes) {
  if (typeof name !== "string" || !NAME.test(name)) fail("original name is invalid");
  if (!Buffer.isBuffer(bytes) || bytes.length === 0) fail("original bytes are required");
  return { name, sha256: digest(bytes), size: bytes.length };
}

export async function writeNativeInputBundle({ directory, record, originals } = {}) {
  if (typeof directory !== "string" || !directory) fail("directory is required");
  await assertSourcePolicy();
  const validated = assertNativeInputBundle(record);
  const roles = validated.platform === "win32"
    ? ["profile", "abi", "geometry", "custody", "toolchain", "windowsQueryProfile"]
    : ["profile", "abi", "geometry", "custody", "toolchain"];
  closed(originals, roles, "originals");
  closed(originals.custody, ["entry", "provider", "family", "finance"], "originals.custody");
  closed(originals.toolchain, ["go", "node", "python"], "originals.toolchain");
  const entries = [
    ["profile", validated.profile],
    ["abi", validated.abi],
    ["geometry", validated.geometry],
    ["custody.entry", validated.custody.entry],
    ["custody.provider", validated.custody.provider],
    ["custody.family", validated.custody.family],
    ["custody.finance", validated.custody.finance],
    ["toolchain.go", validated.toolchain.go],
    ["toolchain.node", validated.toolchain.node],
    ["toolchain.python", validated.toolchain.python],
    ...(validated.platform === "win32" ? [["windowsQueryProfile", validated.windowsQueryProfile]] : []),
  ];
  const original = role => role.split(".").reduce((value, key) => value?.[key], originals);
  const written = [];
  try {
    for (const [role, ref] of entries) {
      const bytes = original(role);
      if (!Buffer.isBuffer(bytes) || bytes.length !== ref.size || digest(bytes) !== ref.sha256) fail(`${role} original bytes do not match the bundle`);
      const target = path.join(directory, ref.name);
      await writeFile(target, bytes, { flag: "wx", mode: 0o600 });
      written.push(target);
    }
    await writeFile(path.join(directory, NATIVE_INPUT_FILE), JSON.stringify(validated) + "\n", { flag: "wx", mode: 0o600 });
  } catch (error) {
    for (const target of written.reverse()) await unlink(target).catch(() => {});
    throw denied(error);
  }
  return validated;
}
