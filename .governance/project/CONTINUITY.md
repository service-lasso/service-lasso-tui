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

## 2026-10-01: PR #15 clean-build and archive-identity checkpoint

Run `36758877647` is retained as a failed exact-head CI record: Windows job
`110036016339` failed after source and VCS admission with only the closed
terminal and API-client categories, while Linux, macOS, and cross-compilation
passed. Its full failed-log SHA-256 is
`4c72a395488b6d72e74fba8149001393b9d10f97a0db1e065109aa6dc2ce0826`.
The executable's `a1e768` source stamp is the GitHub PR merge candidate with
the documented base/head parents and computed merge tree, so it is not a
branch-head mismatch. The present repair closes the unguarded local
real-Core source build and adds post-extraction VCS checks for every candidate
archive executable. Controlled clean detached Windows evidence passes the
unavailable ConPTY probe for the same built executable and preserves the
typed direct configuration exit. This does not resolve the hosted
source/runtime contradiction. Fresh natural CI and independent review remain
the next actions; release, publication, deployment, and GA are not authorised.
