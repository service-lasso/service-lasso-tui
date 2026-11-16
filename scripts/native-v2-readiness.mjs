// Source-only admission gate. It never authenticates an issuer, grants native
// authority, or claims a PASS; it only refuses admission unless every required
// group is present, closed, and internally consistent.
import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import { decodeCandidateProfile, decodeNativeGeometry } from "./candidate-profile-source.mjs";

const SHA256 = /^[0-9a-f]{64}$/u;
const READY = "READY";
const UNAVAILABLE = "UNAVAILABLE";
const INVALID = "INVALID";

export const READINESS_GROUPS = Object.freeze([
  Object.freeze({ id: "slcprof1-profile", requirement: "authentic nine-section SLCPROF1 original" }),
  Object.freeze({ id: "go-c-abi", requirement: "closed goIntBits/cSizeTBits ABI original" }),
  Object.freeze({ id: "native-geometry", requirement: "complete native geometry original joined to the profile" }),
  Object.freeze({ id: "constructors", requirement: "genuine source/domain/operation/family/provider constructors" }),
  Object.freeze({ id: "windows-inputs", requirement: "authentic Windows query profile/environment/tool/source inputs" }),
  Object.freeze({ id: "matching-host-ownership", requirement: "named matching Linux and Windows host/toolchain owners" }),
  Object.freeze({ id: "admission", requirement: "fresh independent review, new authentic ROOT, exact-command approval" }),
  Object.freeze({ id: "core-integration", requirement: "qualified Core context and same-published-byte disposition" }),
]);

const group = id => READINESS_GROUPS.find(row => row.id === id);
const row = (id, state, detail) => ({ id, requirement: group(id).requirement, state, detail });
const present = value => value !== undefined && value !== null;
const emptyStrings = value => typeof value !== "string" || !value.length;

function custodyRow(id, entry) {
  if (!present(entry)) return row(id, UNAVAILABLE, "not supplied");
  if (typeof entry !== "object" || Array.isArray(entry)) return row(id, INVALID, "custody record is not a closed object");
  const keys = Object.keys(entry).sort().join(",");
  if (keys !== "custodyRef,evidenceSha256,owner") return row(id, INVALID, "custody record fields are not closed");
  if (emptyStrings(entry.owner)) return row(id, INVALID, "custody owner missing");
  if (emptyStrings(entry.custodyRef)) return row(id, INVALID, "custody reference missing");
  if (!SHA256.test(entry.evidenceSha256)) return row(id, INVALID, "custody evidence digest is not a raw sha256");
  return row(id, READY, "custody record closed");
}

export function assessNativeV2Readiness(input = {}) {
  const rows = [];

  let quantities;
  if (!present(input.profile)) rows.push(row("slcprof1-profile", UNAVAILABLE, "not supplied"));
  else if (!Buffer.isBuffer(input.profile)) rows.push(row("slcprof1-profile", INVALID, "profile original is not a buffer"));
  else {
    try {
      quantities = decodeCandidateProfile(input.profile).quantities;
      rows.push(row("slcprof1-profile", READY, "profile original decoded under the closed header"));
    } catch (error) {
      rows.push(row("slcprof1-profile", INVALID, error.message));
    }
  }

  let abi;
  if (!present(input.abi)) rows.push(row("go-c-abi", UNAVAILABLE, "not supplied"));
  else if (typeof input.abi !== "object" || Array.isArray(input.abi) ||
    Object.keys(input.abi).sort().join(",") !== "cSizeTBits,goIntBits" ||
    ![32, 64].includes(input.abi.goIntBits) || ![32, 64].includes(input.abi.cSizeTBits)) {
    rows.push(row("go-c-abi", INVALID, "ABI original is not a closed goIntBits/cSizeTBits object"));
  } else {
    abi = { goIntBits: input.abi.goIntBits, cSizeTBits: input.abi.cSizeTBits };
    rows.push(row("go-c-abi", READY, "ABI original closed"));
  }

  if (!present(input.nativeGeometry)) rows.push(row("native-geometry", UNAVAILABLE, "not supplied"));
  else if (!quantities || !abi) rows.push(row("native-geometry", UNAVAILABLE, "profile/ABI join unavailable"));
  else {
    try {
      decodeNativeGeometry(input.nativeGeometry, abi.cSizeTBits, quantities);
      rows.push(row("native-geometry", READY, "geometry roster and shared quantities joined"));
    } catch (error) {
      rows.push(row("native-geometry", INVALID, error.message));
    }
  }

  const custody = input.custody && typeof input.custody === "object" && !Array.isArray(input.custody) ? input.custody : {};
  for (const id of ["constructors", "windows-inputs", "matching-host-ownership", "admission", "core-integration"]) {
    rows.push(custodyRow(id, custody[id]));
  }

  const blockers = rows.filter(entry => entry.state !== READY).map(entry => ({ id: entry.id, state: entry.state, detail: entry.detail }));
  return {
    admitted: blockers.length === 0,
    disposition: blockers.length === 0 ? "ADMITTED_FOR_REVIEW" : "NOT_READY_NOT_ADMITTED",
    groups: rows,
    blockers,
  };
}

export function custodyDigest(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function parseArgs(argv) {
  const options = {};
  for (let index = 0; index < argv.length; index += 2) {
    const flag = argv[index];
    const value = argv[index + 1];
    if (!["--profile", "--abi", "--geometry", "--custody"].includes(flag) || value === undefined) {
      throw new Error(`unexpected argument: ${flag}`);
    }
    options[flag.slice(2)] = value;
  }
  return options;
}

async function readOptional(file) {
  if (!file) return undefined;
  try {
    return await readFile(file);
  } catch {
    return undefined;
  }
}

async function parseJSONFile(file) {
  const bytes = await readOptional(file);
  if (!bytes) return undefined;
  try {
    return JSON.parse(bytes.toString("utf8"));
  } catch {
    return { invalid: true };
  }
}

export async function assessNativeV2ReadinessFromFiles(options) {
  const profile = await readOptional(options.profile);
  const abi = await parseJSONFile(options.abi);
  if (abi?.invalid) return assessNativeV2Readiness({ profile, abi: { invalid: true } });
  const geometry = await parseJSONFile(options.geometry);
  if (geometry?.invalid) return assessNativeV2Readiness({ profile, abi, nativeGeometry: { invalid: true } });
  const custody = await parseJSONFile(options.custody);
  return assessNativeV2Readiness({ profile, abi, nativeGeometry: geometry, custody: custody?.invalid ? {} : custody });
}

if (process.argv[1] && process.argv[1].endsWith("native-v2-readiness.mjs")) {
  try {
    const result = await assessNativeV2ReadinessFromFiles(parseArgs(process.argv.slice(2)));
    process.stdout.write(JSON.stringify(result, null, 2) + "\n");
    process.exitCode = result.admitted ? 0 : 1;
  } catch {
    process.stderr.write("readiness assessment denied\n");
    process.exitCode = 2;
  }
}
