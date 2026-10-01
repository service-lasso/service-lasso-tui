import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import test from "node:test";

const scripts = path.dirname(fileURLToPath(import.meta.url));

test("native owner architecture is shared by Linux and Darwin and requires real adverse proof", async () => {
  const [controller, owner, runtime] = await Promise.all(["tui20-native-core.mjs", "tui20-native-posix-five-action.py", "tui20-native-runtime.mjs"].map(name => readFile(path.join(scripts, name), "utf8")));
  assert.match(controller, /--adverse-controller-crash/);
  assert.match(controller, /process\.kill\(process\.pid, "SIGKILL"\)/);
  assert.match(owner, /start_owned_runtime/);
  assert.match(owner, /TUI child did not survive actual controller failure/);
  assert.match(owner, /coreAndJwksLiveAfterFailure/);
  assert.match(owner, /naturalChildExit/);
  assert.match(owner, /Every post-start preflight stays inside this owner boundary/);
  assert.match(owner, /owner-preflight-cleanup\.json/);
  assert.match(runtime, /invalidReadyReceipt/);
  assert.match(owner, /sys_platform\(\)=="linux"/);
  assert.match(owner, /sys_platform\(\)=="darwin"/);
  assert.match(runtime, /startApiServer/);
  assert.match(runtime, /await server\.stop\(\)/);
});
