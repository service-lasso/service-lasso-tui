import test from 'node:test';
import assert from 'node:assert/strict';
import { ACTIONS_REQUIRED_CHECKS as names, assertActionsCandidatePolicy as verify } from './actions-candidate-policy.mjs';
const sha = 'a'.repeat(40);
const fixture = () => ({ branch: { name: 'develop', protected: true, commit: { sha }, protection: { enabled: true, required_status_checks: { enforcement_level: 'everyone', checks: names.map(context => ({ context, app_id: 15368 })) } } }, environment: { name: 'development-candidate', protection_rules: [{ type: 'wait_timer', wait_timer: 1 }], deployment_branch_policy: { protected_branches: true, custom_branch_policies: false } }, checks: { total_count: names.length, check_runs: names.map(name => ({ name, app: { id: 15368 }, head_sha: sha, status: 'completed', conclusion: 'success' })) } });
test('accept exact protected source and all genuine successful required checks', () => assert.equal(verify(fixture(), sha, sha).sha, sha));
test('deny missing or mismatched operator attestation', () => { for (const value of [undefined, 'b'.repeat(40)]) assert.throws(() => verify(fixture(), sha, value)); });
for (const [name, mutate] of Object.entries({
  moved: f => f.branch.commit.sha = 'b'.repeat(40),
  unprotected: f => f.branch.protected = false,
  unenforced: f => f.branch.protection.required_status_checks.enforcement_level = 'non_admins',
  spoofedRequiredApp: f => f.branch.protection.required_status_checks.checks[0].app_id = 1,
  duplicateRequired: f => f.branch.protection.required_status_checks.checks[0] = f.branch.protection.required_status_checks.checks[1],
  missingRequired: f => f.branch.protection.required_status_checks.checks.pop(),
  noTimer: f => f.environment.protection_rules = [],
  noProtectedBranches: f => f.environment.deployment_branch_policy.protected_branches = false,
  wrongSource: f => f.checks.check_runs[0].head_sha = 'b'.repeat(40),
  failed: f => f.checks.check_runs[0].conclusion = 'failure',
  pending: f => f.checks.check_runs[0].status = 'in_progress',
  spoofedCheckApp: f => f.checks.check_runs[0].app.id = 1,
  truncated: f => f.checks.total_count = 101,
  duplicateCheck: f => { f.checks.check_runs.push(f.checks.check_runs[0]); f.checks.total_count++; },
})) test(`deny ${name}`, () => { const f = fixture(); mutate(f); assert.throws(() => verify(f, sha, sha)); });

test('actual publisher rejects missing Actions attestation before local or provider access', async () => {
  const { publishCandidate } = await import('./publish-candidate.mjs');
  let calls = 0;
  await assert.rejects(publishCandidate({ assetDirectory: '/nonexistent-unread-candidate', identity: {sourceRef: 'refs/heads/develop',sourceCommit: sha,version: '2026.11.28-aaaaaaa',tag:'candidate-2026.11.28-aaaaaaa'},token: 'held-token',policyMode:'actions',fetchImpl:async()=>{calls++;throw new Error('unexpected provider');} }), /operator preflight binding denied/);
  assert.equal(calls, 0);
});
