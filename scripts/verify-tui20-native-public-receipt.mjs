import { readdirSync, readFileSync } from "node:fs";
import path from "node:path";

const root = process.argv[2];
if (!root) throw new Error("native receipt root is required");
const names = new Set(["binary-digest.json", "input-custody.json", "core-source-binding.json", "build-output.json", "native-public-projection.json"]);
const files = readdirSync(root, { recursive: true }).filter(file => names.has(path.basename(file))).map(file => path.join(root, file));
if (!files.length || files.length % names.size) throw new Error("native public receipt set is incomplete");
const exact = (value, keys) => value && typeof value === "object" && !Array.isArray(value) && Object.keys(value).sort().join(",") === [...keys].sort().join(",");
const forbiddenKey = /(?:^|[A-Z_])(?:path|pid|ppid|birth|private|image|tool|literalcommand)(?:$|[A-Z_])|(?:PID|PPID)$/;
const forbiddenText = /(?:owner-private|\b(?:pid|ppid|birth)\b|(?:^|["'\s])(?:\/|[A-Za-z]:[\\/]))/i;
const inspect = value => {
  if (Array.isArray(value)) return value.forEach(inspect);
  if (value && typeof value === "object") for (const [key, child] of Object.entries(value)) { if (forbiddenKey.test(key)) throw new Error(`forbidden public key: ${key}`); inspect(child); }
  else if (typeof value === "string" && forbiddenText.test(value)) throw new Error("private or host-specific text in public receipt");
};
for (const file of files) {
  const value = JSON.parse(readFileSync(file, "utf8")); inspect(value);
  switch (path.basename(file)) {
    case "binary-digest.json": if (!exact(value,["schemaVersion","kind","sha256","size"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-binary-digest" || !/^[a-f0-9]{64}$/.test(value.sha256) || !Number.isSafeInteger(value.size) || value.size <= 0) throw new Error("invalid binary digest schema"); break;
    case "input-custody.json": if (!exact(value,["schemaVersion","kind","source","core","ownership","verification"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-input-custody") throw new Error("invalid input custody schema"); break;
    case "core-source-binding.json": if (!exact(value,["schemaVersion","kind","coreCommit","coreTree","coreDirtyHash"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-core-source-binding") throw new Error("invalid Core binding schema"); break;
    case "build-output.json": if (!exact(value,["schemaVersion","kind","tuiCommit","nativeBinary"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-build-output") throw new Error("invalid build output schema"); break;
    case "native-public-projection.json": if (!exact(value,["schemaVersion","kind","sourceCommit","binarySHA256","result","actionsPassed","ownedRuntimeClosed"]) || value.schemaVersion !== 1 || value.kind !== "tui20-native-public-result") throw new Error("invalid public projection schema"); break;
  }
}
