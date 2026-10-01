import assert from "node:assert/strict";
import { mkdtemp, readFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { spawn } from "node:child_process";
import { once } from "node:events";
import test from "node:test";

const scripts = path.dirname(fileURLToPath(import.meta.url));

test("external recovery owner keeps live PTY, sealed execution, Core, and JWKS after helper crash", { skip: process.platform !== "linux" }, async () => {
  const root = await mkdtemp(path.join(tmpdir(), "tui20-recovery-process-"));
  try {
    const child = spawn(process.execPath, [path.join(scripts, "tui20-native-core.mjs"), "--root", root, "--recovery-self-test", "--helper", path.join(scripts, "tui20-native-posix-five-action.py")], { stdio: "inherit" });
    const [code, signal] = await once(child, "exit");
    assert.equal(code, 0); assert.equal(signal, null);
    const [owner, parent, unresolved] = await Promise.all(["recovery-owner-proof.json", "recovery-parent-proof.json", "recovery-unresolved-receipt.json"].map(async name => JSON.parse(await readFile(path.join(root, name), "utf8"))));
    assert.equal(owner.helperCrashObserved, true);
    assert.equal(owner.childLiveAfterHelperCrash, true);
    assert.equal(owner.ptyHeldAfterHelperCrash, true);
    assert.equal(owner.immutableExecutionHeld, true);
    assert.equal(owner.dependenciesLiveAfterHelperCrash, true);
    assert.equal(owner.ownedExitObserved, "terminal_exited_zero");
    assert.equal(owner.reapedChildOutcome, "terminal_unknown");
    assert.equal(owner.reapedChildRecovery, "child_reaped_unowned");
    assert.equal(unresolved.recoveryRetained, true);
    assert.equal(parent.recoveryOwnerExitObserved, true);
    assert.equal(parent.coreStopAfterOwnerExit, true);
    assert.equal(parent.jwksStopAfterOwnerExit, true);
  } finally { await rm(root, { recursive: true, force: true }); }
});
