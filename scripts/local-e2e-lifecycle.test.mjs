// UNRUN source fixtures only. Fake adapters prove contract/fail-closed behavior,
// never a native lifecycle or acquired-input authority.
import test from "node:test";
import assert from "node:assert/strict";
import { runLifecycle, LIFECYCLE_ACTIONS, TARGET_SERVICE } from "./local-e2e-lifecycle.mjs";

function fake(hooks = () => {}) {
  const calls = [];
  const operations = [];
  const audits = new Map();
  let current = null;
  let sequence = 0;
  let unrelated = { id: "tui20-unrelated", enabled: true, runtimeState: "stopped", running: false, availability: true };
  const controls = {
    calls, operations, audits,
    mutateRecord: record => record,
    auditValue: undefined,
    setUnrelated(value) { unrelated = value; },
    get current() { return current; },
  };
  controls.adapter = {
    calls, operations, audits,
    async ready() { calls.push("ready"); },
    async selectService(id) { calls.push(`select:${id}`); },
    async openAction(action) { current = action; calls.push(`action:${action.name}`); return { preview: `Core preview: ${action.name}`, confirmation: `Confirm ${action.name}` }; },
    async assertPreviewFrozen() { calls.push("frozen"); return true; },
    async confirm() {
      calls.push(`confirm:${current.name}`);
      const record = controls.mutateRecord({ operationId: `op-${++sequence}`, action: current.operation, targetIds: [TARGET_SERVICE], status: "succeeded", outcome: "succeeded", cancellationSupported: false });
      audits.set(record.operationId, controls.auditValue ?? 1);
      operations.push(record);
    },
    async durableOperations() { calls.push("operations"); return operations.slice(); },
    async auditCount(operationId) { calls.push(`audit:${operationId}`); return audits.get(operationId) ?? 0; },
    async requestReloadRefusal() { calls.push("reload"); },
    async reconnect() { calls.push("reconnect"); return []; },
    async resize(columns, rows) { calls.push(`resize:${columns}x${rows}`); },
    async unrelatedSnapshot() { calls.push("unrelated"); return structuredClone(unrelated); },
    async quit() { calls.push("quit"); return { observedExit: true, exitCode: 0 }; },
  };
  hooks(controls);
  return controls;
}

const reject = async (controls, pattern) => {
  await assert.rejects(runLifecycle(controls.adapter), pattern);
};

test("five adapter actions run once each in order and return a source-only summary", async () => {
  const controls = fake();
  const summary = await runLifecycle(controls.adapter);
  assert.equal(summary.status, "passed");
  assert.equal(summary.scope, "source-adapter-contract");
  assert.equal(summary.nativeQualified, false);
  assert.deepEqual(summary.actions, ["install", "config", "start", "stop", "restart"]);
  assert.equal(summary.operationCount, 5);
  assert.equal(summary.auditCount, 5);
  assert.equal(summary.frozenPreview, true);
  assert.deepEqual(summary.resize, [[80, 24], [150, 44]]);
  const order = controls.calls.filter(value => value.startsWith("action:") || value.startsWith("confirm:"));
  assert.deepEqual(order, ["action:install", "confirm:install", "action:config", "confirm:config", "action:start", "confirm:start", "action:stop", "confirm:stop", "action:restart", "confirm:restart"]);
  for (const action of LIFECYCLE_ACTIONS) assert.ok(controls.calls.includes(`action:${action.name}`));
  assert.equal(JSON.stringify(summary).includes("token"), false);
});

test("a missing adapter stage fails closed before any action", async () => {
  const controls = fake(({ adapter }) => { delete adapter.auditCount; });
  await reject(controls, /stage unavailable: auditCount/u);
  assert.equal(controls.operations.length, 0);
});

test("preview must show the frozen Core preview and confirmation", async () => {
  const missing = fake(({ adapter }) => {
    adapter.openAction = async action => ({ preview: `Core preview: ${action.name}`, confirmation: "Confirm something else" });
  });
  await reject(missing, /frozen preview mismatch/u);

  const changed = fake(({ adapter }) => { adapter.assertPreviewFrozen = async () => false; });
  await reject(changed, /preview changed by blocked input/u);
});

test("each action must add exactly one matching durable operation", async () => {
  const none = fake(({ adapter }) => { adapter.confirm = async () => {}; });
  await reject(none, /exactly one durable operation required/u);

  const duplicate = fake(controls => {
    controls.adapter.confirm = async () => {
      for (let index = 0; index < 2; index++) {
        const record = controls.mutateRecord({ operationId: `dup-${index}-${controls.current.name}`, action: controls.current.operation, targetIds: [TARGET_SERVICE], status: "succeeded", outcome: "succeeded", cancellationSupported: false });
        controls.audits.set(record.operationId, 1);
        controls.operations.push(record);
      }
    };
  });
  await reject(duplicate, /exactly one durable operation required/u);
});

const mutations = [
  ["wrong action", record => ({ ...record, action: "service_stop" })],
  ["wrong target", record => ({ ...record, targetIds: ["foreign"] })],
  ["failed status", record => ({ ...record, status: "failed" })],
  ["cancellation advertised", record => ({ ...record, cancellationSupported: true })],
];
for (const [label, mutate] of mutations) {
  test(`durable readback rejects ${label}`, async () => {
    const controls = fake(controls => { controls.mutateRecord = mutate; });
    await reject(controls, /durable (action|target|status) mismatch|unsupported cancellation/u);
  });
}

test("Core audit must have exactly one entry per operation", async () => {
  for (const value of [0, 2]) {
    const controls = fake(controls => { controls.auditValue = value; });
    await reject(controls, /exactly one Core audit entry/u);
  }
});

test("reload refusal and reconnect must not submit or replay operations", async () => {
  const reload = fake(({ adapter, operations }) => {
    adapter.requestReloadRefusal = async () => {
      operations.push({ operationId: "reload", action: "service_install", targetIds: [TARGET_SERVICE], status: "succeeded", outcome: "succeeded", cancellationSupported: false });
    };
  });
  await reject(reload, /reload refusal submitted an operation/u);

  const replay = fake(({ adapter }) => { adapter.reconnect = async () => ["op-1"]; });
  await reject(replay, /replayed after reconnect/u);

  const submit = fake(({ adapter, operations }) => {
    adapter.reconnect = async () => {
      operations.push({ operationId: "reconnect", action: "service_install", targetIds: [TARGET_SERVICE], status: "succeeded", outcome: "succeeded", cancellationSupported: false });
      return [];
    };
  });
  await reject(submit, /reconnect submitted an operation/u);
});

test("unrelated service state and the actual owned exit are enforced", async () => {
  let snapshots = 0;
  const unrelated = fake(controls => {
    controls.adapter.unrelatedSnapshot = async () => {
      snapshots += 1;
      return structuredClone(snapshots === 1
        ? { id: "tui20-unrelated", enabled: true, runtimeState: "stopped", running: false, availability: true }
        : { id: "tui20-unrelated", enabled: false, runtimeState: "stopped", running: false, availability: true });
    };
  });
  await reject(unrelated, /unrelated service state changed/u);

  const unobserved = fake(({ adapter }) => { adapter.quit = async () => ({ observedExit: false, exitCode: 0 }); });
  await reject(unobserved, /exit was not observed/u);

  const nonzero = fake(({ adapter }) => { adapter.quit = async () => ({ observedExit: true, exitCode: 1 }); });
  await reject(nonzero, /exited nonzero/u);
});
