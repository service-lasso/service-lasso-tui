import assert from 'node:assert/strict';
import test from 'node:test';
import {assertSummary} from './run-windows-native-lifecycle.mjs';

const operation = (id, action) => ({
  operationId: 'mcp-operation-' + id,
  action,
  targetIds: ['tui20-fixture'],
  status: 'succeeded',
  outcome: 'succeeded',
  cancellationSupported: false,
});
const good = {
  schemaVersion: 1,
  kind: 'tui23-windows-native-lifecycle',
  status: 'passed',
  scope: 'issue-23-windows-five-action-acceptance',
  sourceCommit: 'a'.repeat(40),
  binarySHA256: 'b'.repeat(64),
  coreCommit: 'c'.repeat(40),
  transport: 'ConPTY',
  actions: ['install', 'config', 'start', 'stop', 'restart'],
  operationCount: 5,
  operations: [
    operation('aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa', 'service_install'),
    operation('bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb', 'service_configure'),
    operation('cccccccc-cccc-cccc-cccc-cccccccccccc', 'service_start'),
    operation('dddddddd-dddd-dddd-dddd-dddddddddddd', 'service_stop'),
    operation('eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee', 'service_restart'),
  ],
  coreAudit: [
    {operationId: 'mcp-operation-aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa', coreAuditCount: 1},
    {operationId: 'mcp-operation-bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb', coreAuditCount: 1},
    {operationId: 'mcp-operation-cccccccc-cccc-cccc-cccc-cccccccccccc', coreAuditCount: 1},
    {operationId: 'mcp-operation-dddddddd-dddd-dddd-dddd-dddddddddddd', coreAuditCount: 1},
    {operationId: 'mcp-operation-eeeeeeee-eeee-eeee-eeee-eeeeeeeeeeee', coreAuditCount: 1},
  ],
  terminals: [
    {terminal: 'allowed', exit: 'terminal_exited_zero'},
    {terminal: 'reconnect', exit: 'terminal_exited_zero'},
  ],
  reconnect: {completedOperationNoReplay: true},
  unrelatedService: {id: 'tui20-unrelated', unchangedAfterFiveActions: true},
  resize: true,
  previewFrozen: true,
  reloadDenied: true,
  ownedRuntimeClosed: true,
};

test('accepts the closed Windows native five-action summary', () => {
  assert.doesNotThrow(() => assertSummary(structuredClone(good)));
});

for (const [label, mutate] of [
  ['failed status', value => { value.status = 'failed'; }],
  ['missing action', value => { value.actions.pop(); }],
  ['operation target', value => { value.operations[0].targetIds = ['other']; }],
  ['operation duplicate', value => { value.operations[1].operationId = value.operations[0].operationId; }],
  ['operation outcome', value => { value.operations[2].outcome = 'failed'; }],
  ['cancellation advertised', value => { value.operations[3].cancellationSupported = true; }],
  ['Core audit', value => { value.coreAudit[0].coreAuditCount = 2; }],
  ['terminal exit', value => { value.terminals[0].exit = 'terminal_exit_code_1'; }],
  ['missing reconnect', value => { value.terminals.pop(); }],
  ['replay', value => { value.reconnect.completedOperationNoReplay = false; }],
  ['unrelated changed', value => { value.unrelatedService.unchangedAfterFiveActions = false; }],
  ['missing resize', value => { value.resize = false; }],
  ['missing frozen preview', value => { value.previewFrozen = false; }],
  ['runtime left open', value => { value.ownedRuntimeClosed = false; }],
]) test(`rejects ${label}`, () => {
  const value = structuredClone(good);
  mutate(value);
  assert.throws(() => assertSummary(value));
});
