// Ordinary source adapter lifecycle contract. It is not native evidence and it
// never grants v2/acquired-input authority; any unavailable stage fails closed.
export const TARGET_SERVICE = "tui20-fixture";
export const UNRELATED_SERVICE = "tui20-unrelated";

export const LIFECYCLE_ACTIONS = Object.freeze([
  Object.freeze({ name: "install", key: "i", operation: "service_install" }),
  Object.freeze({ name: "config", key: "c", operation: "service_configure" }),
  Object.freeze({ name: "start", key: "s", operation: "service_start" }),
  Object.freeze({ name: "stop", key: "x", operation: "service_stop" }),
  Object.freeze({ name: "restart", key: "R", operation: "service_restart" }),
]);

const REQUIRED = Object.freeze([
  "ready", "selectService", "openAction", "assertPreviewFrozen", "confirm",
  "durableOperations", "auditCount", "requestReloadRefusal", "reconnect",
  "resize", "unrelatedSnapshot", "quit",
]);

function assertAdapter(adapter) {
  if (!adapter || typeof adapter !== "object") throw new Error("lifecycle adapter required");
  for (const name of REQUIRED) if (typeof adapter[name] !== "function") throw new Error(`lifecycle adapter stage unavailable: ${name}`);
}

function assertClosedOperation(record, expected) {
  if (!record || typeof record !== "object") throw new Error(`durable readback missing for ${expected.name}`);
  if (typeof record.operationId !== "string" || !record.operationId.length) throw new Error(`durable operation id missing for ${expected.name}`);
  if (record.action !== expected.operation) throw new Error(`durable action mismatch for ${expected.name}`);
  if (!Array.isArray(record.targetIds) || record.targetIds.length !== 1 || record.targetIds[0] !== TARGET_SERVICE) throw new Error(`durable target mismatch for ${expected.name}`);
  if (record.status !== "succeeded" || record.outcome !== "succeeded") throw new Error(`durable status mismatch for ${expected.name}`);
  if (record.cancellationSupported !== false) throw new Error(`unsupported cancellation advertising for ${expected.name}`);
}

export async function runLifecycle(adapter) {
  assertAdapter(adapter);
  await adapter.ready();
  await adapter.selectService(TARGET_SERVICE);
  const unrelatedBefore = await adapter.unrelatedSnapshot();
  if (!unrelatedBefore || typeof unrelatedBefore !== "object") throw new Error("unrelated service snapshot unavailable");
  const initial = await adapter.durableOperations();
  if (!Array.isArray(initial)) throw new Error("durable operation list unavailable");
  const known = new Set(initial.map(record => record?.operationId));
  const perAction = [];

  for (const action of LIFECYCLE_ACTIONS) {
    const preview = await adapter.openAction(action);
    if (!preview || typeof preview.preview !== "string" || typeof preview.confirmation !== "string") throw new Error(`preview unavailable for ${action.name}`);
    if (!preview.preview.includes(`Core preview: ${action.name}`) || !preview.confirmation.includes(`Confirm ${action.name}`)) throw new Error(`frozen preview mismatch for ${action.name}`);
    if (await adapter.assertPreviewFrozen() !== true) throw new Error(`preview changed by blocked input for ${action.name}`);
    await adapter.confirm();
    const after = await adapter.durableOperations();
    if (!Array.isArray(after)) throw new Error(`durable readback unavailable for ${action.name}`);
    const added = after.filter(record => !known.has(record?.operationId));
    if (added.length !== 1) throw new Error(`exactly one durable operation required for ${action.name}`);
    assertClosedOperation(added[0], action);
    const count = await adapter.auditCount(added[0].operationId, action.operation);
    if (count !== 1) throw new Error(`exactly one Core audit entry required for ${action.name}`);
    known.add(added[0].operationId);
    perAction.push(added[0]);
  }

  const beforeReload = await adapter.durableOperations();
  if (!Array.isArray(beforeReload)) throw new Error("durable operation list unavailable before reload");
  await adapter.requestReloadRefusal();
  const afterReload = await adapter.durableOperations();
  if (!Array.isArray(afterReload) || afterReload.length !== beforeReload.length) throw new Error("reload refusal submitted an operation");

  const beforeReconnect = afterReload.length;
  const replay = await adapter.reconnect();
  if (!Array.isArray(replay)) throw new Error("reconnect replay inventory unavailable");
  if (replay.length !== 0) throw new Error("completed operation replayed after reconnect");
  const afterReconnect = await adapter.durableOperations();
  if (!Array.isArray(afterReconnect) || afterReconnect.length !== beforeReconnect) throw new Error("reconnect submitted an operation");

  await adapter.resize(80, 24);
  await adapter.resize(150, 44);
  const unrelatedAfter = await adapter.unrelatedSnapshot();
  if (JSON.stringify(unrelatedAfter) !== JSON.stringify(unrelatedBefore)) throw new Error("unrelated service state changed");

  const exit = await adapter.quit();
  if (!exit || exit.observedExit !== true) throw new Error("actual owned terminal exit was not observed");
  if (exit.exitCode !== 0) throw new Error("owned terminal exited nonzero");

  return {
    schemaVersion: 1,
    stage: "lifecycle",
    scope: "source-adapter-contract",
    nativeQualified: false,
    status: "passed",
    target: TARGET_SERVICE,
    actions: LIFECYCLE_ACTIONS.map(action => action.name),
    operationCount: perAction.length,
    auditCount: perAction.length,
    targetOperations: perAction.map(record => ({ operationId: record.operationId, action: record.action, targetIds: record.targetIds, status: record.status, outcome: record.outcome })),
    unrelatedService: { id: UNRELATED_SERVICE, unchangedAfterFiveActions: true },
    frozenPreview: true,
    reloadRefused: true,
    noReplayAfterReconnect: true,
    resize: [[80, 24], [150, 44]],
    exitCode: exit.exitCode,
    observedExit: true,
  };
}
