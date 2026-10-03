import { parseStrictJSON } from "./scoped-json.mjs";
import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";

export function verifyNativePublicReceipt(root) {
if (!root) throw new Error("native receipt root is required");
const names = ["binary-digest.json", "input-custody.json", "core-source-binding.json", "build-output.json", "native-public-projection.json"];
// Acquire each fixed public sibling once; private trees remain outside this namespace.
const files = Object.fromEntries(readdirSync(root).filter(name => names.includes(name)).map(name => [name, readFileSync(path.join(root, name))]));
return verifyHeldNativePublicReceipt(files);
}

export function verifyHeldNativePublicReceipt(files) {
const names = new Set(["binary-digest.json", "input-custody.json", "core-source-binding.json", "build-output.json", "native-public-projection.json"]);
const exact = (value, keys) => value && typeof value === "object" && !Array.isArray(value) && Object.keys(value).sort().join(",") === [...keys].sort().join(",");
const hash = value => typeof value === "string" && /^[a-f0-9]{64}$/.test(value);
// These repositories produce SHA1 Git objects; SHA256 byte digests are separate.
const gitObject = value => typeof value === "string" && /^[a-f0-9]{40}$/.test(value);
const commit = gitObject;
const size = value => Number.isSafeInteger(value) && value > 0;
const fail = message => { throw new Error(message); };
const forbiddenKey = /(?:^|[A-Z_])(?:path|pid|ppid|birth|private|image|tool|literalcommand)(?:$|[A-Z_])|(?:PID|PPID)$/;
const forbiddenText = /(?:owner-private|\b(?:pid|ppid|birth)\b|(?:^|["'\s])(?:\/|[A-Za-z]:[\\/]))/i;
const inspect = value => {
  if (Array.isArray(value)) return value.forEach(inspect);
  if (value && typeof value === "object") for (const [key, child] of Object.entries(value)) { if (forbiddenKey.test(key)) fail(`forbidden public key: ${key}`); inspect(child); }
  else if (typeof value === "string" && forbiddenText.test(value)) fail("private or host-specific text in public receipt");
};
const read = bytes => { if (!Buffer.isBuffer(bytes)) fail("native public receipt buffer required"); const value = parseStrictJSON(bytes); inspect(value); return value; };
const assertInputCustody = value => {
  if (!exact(value, ["schemaVersion", "kind", "source", "core", "ownership", "verification"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-input-custody") fail("invalid input custody schema");
  if (!exact(value.source, ["tuiCommit", "tuiTree", "tuiDirtyHash", "tuiInventoryHash"]) || !commit(value.source.tuiCommit) || !gitObject(value.source.tuiTree) || !hash(value.source.tuiDirtyHash) || !hash(value.source.tuiInventoryHash)) fail("invalid input custody source");
  if (!exact(value.core, ["requestedCommit"]) || !commit(value.core.requestedCommit)) fail("invalid input custody Core binding");
  if (!exact(value.ownership, ["threeDistinctPaths", "allParentsNonLink", "registriesInitiallyAbsent"]) || Object.values(value.ownership).some(item => item !== true)) fail("invalid input custody ownership");
  if (!exact(value.verification, ["freshEnvironment", "requiredToolsVerified"]) || value.verification.freshEnvironment !== true || !Array.isArray(value.verification.requiredToolsVerified) || !value.verification.requiredToolsVerified.length || new Set(value.verification.requiredToolsVerified).size !== value.verification.requiredToolsVerified.length || value.verification.requiredToolsVerified.some(item => typeof item !== "string" || !/^[a-z0-9-]+$/.test(item))) fail("invalid input custody verification");
};
const assertCoreBinding = value => {
  if (!exact(value, ["schemaVersion", "kind", "coreCommit", "coreTree", "coreDirtyHash"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-core-source-binding" || !commit(value.coreCommit) || !gitObject(value.coreTree) || !hash(value.coreDirtyHash)) fail("invalid Core binding schema");
};
const assertBinaryDigest = value => {
  if (!exact(value, ["schemaVersion", "kind", "sha256", "size"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-binary-digest" || !hash(value.sha256) || !size(value.size)) fail("invalid binary digest schema");
};
const assertBuildOutput = value => {
  if (!exact(value, ["schemaVersion", "kind", "tuiCommit", "nativeBinary"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-build-output" || !commit(value.tuiCommit) || !exact(value.nativeBinary, ["sha256", "size"]) || !hash(value.nativeBinary.sha256) || !size(value.nativeBinary.size)) fail("invalid build output schema");
};
const assertProjection = value => {
  if (!exact(value, ["schemaVersion", "kind", "sourceCommit", "binarySHA256", "result", "actionsPassed", "ownedRuntimeClosed"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-public-result" || !commit(value.sourceCommit) || !hash(value.binarySHA256) || !["succeeded", "failed", "unresolved"].includes(value.result) || typeof value.actionsPassed !== "boolean" || typeof value.ownedRuntimeClosed !== "boolean") fail("invalid public projection schema");
};

if (!files || !exact(files, [...names])) fail("native public receipt set is incomplete");
const binary = read(files["binary-digest.json"]);
const input = read(files["input-custody.json"]);
const core = read(files["core-source-binding.json"]);
const build = read(files["build-output.json"]);
const projection = read(files["native-public-projection.json"]);
assertBinaryDigest(binary); assertInputCustody(input); assertCoreBinding(core); assertBuildOutput(build); assertProjection(projection);
if (input.source.tuiCommit !== build.tuiCommit || input.source.tuiCommit !== projection.sourceCommit) fail("TUI commit mismatch in native public receipts");
if (input.core.requestedCommit !== core.coreCommit) fail("Core commit mismatch in native public receipts");
if (binary.sha256 !== build.nativeBinary.sha256 || binary.sha256 !== projection.binarySHA256 || binary.size !== build.nativeBinary.size) fail("native executable identity mismatch in native public receipts");
return { binary, input, core, build, projection };
}
