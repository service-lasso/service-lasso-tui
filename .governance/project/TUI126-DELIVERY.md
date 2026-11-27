# Issue #126 connection/login delivery

SPEC-007 TUI-CONNECT: Core #1985 workspace launcher selects local runtime or
passes --connect. Forced connect skips default profile/credential resolution,
starting empty unless --api or SERVICE_LASSO_API_URL prefills the exact target.
The dark keyboard flow edits URL, confirms target, masks secret, verifies actual
operator discovery and reconciliation context, then enters the existing operator
bootstrap. Missing selected credentials and rejected interactive authentication
allow login/retry. Nonterminal failures and invalid explicit profiles remain safe.
Remote requires explicit OAuth bearer choice, HTTPS and general-read admission;
server entitlements still govern actions. No redirect, secret persistence or raw
error disclosure. Credential-bearing URL fragments are hidden while editing and
rejected by admission. Ctrl+C quits, Esc clears credential and returns to target,
Ctrl+U clears the field. The credential editor clears before network dispatch.

Source proof 2026-11-27: Go1.26.0 full go test ./..., go vet ./... and go build
passed. Tests exercise real fixture HTTP auth failure/retry, discovery/context
read success before handoff, masking, target confirmation before network, all
unsafe URL cases, explicit remote scope/transport, profile URL retention,
nonterminal --connect rejection, forced connect avoiding profiles/default/token,
120x32/80x24 screen bounds and selection controls. Parent retains live Core PTY
and recording proof. Source fixtures are not native qualification or GA.

Owned managed worktree /home/maxbarrass/.codex/worktrees/tui-126-connect/service-lasso-tui
branch feature/126-interactive-connect starts from current develop fde65b5, after
#125 merged. Primary checkout and unrelated state remain untouched. Exact review,
PR/merge, hosted outcomes, postmerge proof and archival are recorded on issue #126.
