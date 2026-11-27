// Admin-only controls are admitted by the operator dispatch preflight. These
// independently visible controls are re-read immediately before every mutation.
export const ACTIONS_REQUIRED_CHECKS = Object.freeze(['Linux test and build', 'Windows test and build', 'Windows owned GOROOT overlay tests', 'macOS test and build', 'Release asset cross-compilation']);
const deny = () => { throw new Error('Actions candidate policy denied'); };
export function assertActionsCandidatePolicy({ branch, environment, checks }, sha, operatorPreflightSHA) {
  if (!/^[a-f0-9]{40}$/.test(sha) || operatorPreflightSHA !== sha) deny();
  if (branch?.name !== 'develop' || branch?.protected !== true || branch?.commit?.sha !== sha || branch?.protection?.enabled !== true) deny();
  const required = branch.protection.required_status_checks;
  if (required?.enforcement_level !== 'everyone' || !Array.isArray(required.checks) || required.checks.length !== ACTIONS_REQUIRED_CHECKS.length) deny();
  if (new Set(required.checks.map(check => check.context)).size !== ACTIONS_REQUIRED_CHECKS.length || !required.checks.every(check => check.app_id === 15368 && ACTIONS_REQUIRED_CHECKS.includes(check.context))) deny();
  const wait = environment?.protection_rules?.filter(rule => rule.type === 'wait_timer');
  if (environment?.name !== 'development-candidate' || wait?.length !== 1 || !Number.isInteger(wait[0].wait_timer) || wait[0].wait_timer < 1 || wait[0].wait_timer > 30 || environment?.deployment_branch_policy?.protected_branches !== true || environment?.deployment_branch_policy?.custom_branch_policies !== false) deny();
  // Bounded response must contain the entire set, never accept a truncated page.
  if (!Number.isInteger(checks?.total_count) || checks.total_count > 100 || !Array.isArray(checks.check_runs) || checks.total_count !== checks.check_runs.length) deny();
  for (const name of ACTIONS_REQUIRED_CHECKS) {
    const matching = checks.check_runs.filter(check => check.name === name && check.app?.id === 15368);
    if (matching.length !== 1 || matching[0].head_sha !== sha || matching[0].status !== 'completed' || matching[0].conclusion !== 'success') deny();
  }
  // Pin only control semantics: timestamps and unrelated check runs may change.
  return { sha, requiredChecks: [...required.checks].sort((a,b) => a.context.localeCompare(b.context)), waitMinutes: wait[0].wait_timer, protectedBranches: true };
}
