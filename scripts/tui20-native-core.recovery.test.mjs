import assert from "node:assert/strict";
import { mkdtemp, readFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { spawn } from "node:child_process";
import { once } from "node:events";
import test from "node:test";

const scripts = path.dirname(fileURLToPath(import.meta.url));

test("external parent keeps its helper alive through a live child and records reaped ownership loss", { skip: process.platform === "win32" }, async () => {
  const root = await mkdtemp(path.join(tmpdir(), "tui20-recovery-process-"));
  try {
    const child = spawn(process.execPath, [path.join(scripts, "tui20-native-core.mjs"), "--root", root, "--recovery-self-test", "--helper", path.join(scripts, "tui20-native-posix-five-action.py")], { stdio: "inherit" });
    const [code, signal] = await once(child, "exit");
    assert.equal(code, 0); assert.equal(signal, null);
    const [helper, parent] = await Promise.all(["recovery-process-proof.json", "recovery-parent-proof.json"].map(async name => JSON.parse(await readFile(path.join(root, name), "utf8"))));
    assert.equal(helper.childLiveAtUnresolvedReceipt, true);
    assert.equal(helper.ownedExitObserved, "terminal_exited_zero");
    assert.equal(helper.reapedChildOutcome, "terminal_unknown");
    assert.equal(helper.reapedChildRecovery, "child_reaped_unowned");
    assert.equal(parent.helperExitObserved, true);
    assert.equal(parent.coreStopRequestedAfterHelperExit, true);
  } finally { await rm(root, { recursive: true, force: true }); }
});
