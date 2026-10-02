// Volatile acceptance controller. The POSIX owner owns terminal resources and
// starts the real Core/JWKS runtime as its child.
import { once } from "node:events";
import { access, mkdir } from "node:fs/promises";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";
import path from "node:path";
const arg = name => { const i = process.argv.indexOf(name); return i < 0 ? undefined : process.argv[i + 1]; };
const root = arg("--root") ?? process.argv[2], executable = arg("--executable"), sourceCommit = arg("--source-commit"), coreCommit = arg("--core-commit") ?? "2633c07be25512d0a84f9bfa28de6be5edff35e8", python = arg("--python") ?? "python";
if (!root || !executable || !sourceCommit) throw new Error("root, executable, and source commit are required");
const scripts = path.dirname(fileURLToPath(import.meta.url));
const owner = arg("--helper") ? path.resolve(arg("--helper")) : path.join(scripts, "tui20-native-posix-five-action.py");
const observer = path.join(scripts, "tui20-native-owner-observer.py");
const runtime = path.join(scripts, "tui20-native-runtime.mjs");
const adverse = arg("--adverse-controller-crash") === "true", adverseOwnerDeath = arg("--adverse-owner-death") === "true", invalidReadyReceipt = arg("--invalid-ready-receipt") === "true", injectFinalizationCleanupFailure = arg("--inject-finalization-cleanup-failure") === "true", runtimeReadyMode = arg("--runtime-ready-mode"), shutdownPipeFailure = arg("--shutdown-pipe-failure") === "true";
await mkdir(root, { recursive: true });
// The observer is the parent of the resource owner.  It alone can attest the
// owner's actual wait status after a volatile controller disappears.
const child = spawn(python, [observer, "--owner", owner, "--root", root, "--executable", executable, "--source-commit", sourceCommit, "--core-commit", coreCommit, "--runtime-script", runtime, "--node", process.execPath, ...(adverse ? ["--adverse-controller-crash", "--controller-pid", String(process.pid)] : []), ...(adverseOwnerDeath ? ["--adverse-owner-death"] : []), ...(invalidReadyReceipt ? ["--invalid-ready-receipt"] : []), ...(injectFinalizationCleanupFailure ? ["--inject-finalization-cleanup-failure"] : []), ...(runtimeReadyMode ? ["--runtime-ready-mode", runtimeReadyMode] : []), ...(shutdownPipeFailure ? ["--shutdown-pipe-failure"] : [])], { stdio: "inherit", env: { ...process.env } });
if (adverse) {
  const marker = path.join(root, "external-owner-live.json");
  for (let attempt = 0; attempt < 300; attempt += 1) {
    try { await access(marker); break; } catch { await new Promise(resolve => setTimeout(resolve, 20)); }
    if (attempt === 299) throw new Error("external owner did not establish the actual live-child boundary");
  }
  // The actual controller dies here; no descriptor is claimed to outlive it.
  process.kill(process.pid, "SIGKILL");
}
const [code, signal] = await once(child, "exit");
process.exitCode = code ?? 1;
