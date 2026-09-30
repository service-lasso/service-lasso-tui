# Continuity guidance

Keep current scope and blockers in the issue, specification, backlog, and pull
request. Record a checkpoint when API contracts, launch packaging, ownership,
or validation state changes. Store durable decisions in governance artifacts;
do not rely on terminal history or an uncommitted local state.

## 2026-10-01: PR #15 Windows EOF qualification checkpoint

Exact head `dcce259bddd888c3e7b92731b4406d046aaad0f5` failed Windows job
`109903285182` in run `36720265137` after build and helper installation. The
20,365-byte raw log is retained at
`.governance/evidence/tui-pr15-dcce259-run36720265137/` with SHA-256
`1cb86198621a29a333ff556f357d003bcd940775cf8c50ead19c2d9e548a56d6`.
It records only `terminal_closed`, so it does not attribute the EOF. The
bounded follow-up classifies only owned PTY liveness, exit status, and signal
status; a live EOF retains the existing deadline with a short delay. Fresh
exact-head hosted Windows helper CI and independent review remain required;
there is no candidate acceptance, release, deployment, or GA claim.
