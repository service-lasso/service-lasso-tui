# Continuity guidance

Keep current scope and blockers in the issue, specification, backlog, and pull
request. Record a checkpoint when API contracts, launch packaging, ownership,
or validation state changes. Store durable decisions in governance artifacts;
do not rely on terminal history or an uncommitted local state.

## 2026-10-02: PR #22 external recovery ownership checkpoint

The `d9570fd` helper-only timeout handling is not durable recovery: returning
or raising from that helper closes its owned PTY and immutable execution-object
descriptors and allows the JavaScript parent to stop Core and JWKS while the
terminal child may still be live. The repair must keep a real parent process
live with authority over the child, PTY, execution object, and Core/JWKS
lifecycle until it directly observes exit. It may write a bounded closed
unresolved receipt while waiting, but must not force exit, widen deadlines, or
claim an in-process descriptor survives process exit. `ECHILD` and
`ChildProcessError` mean ownership was lost to reaping: retain the primary
result, record an unresolved recovery outcome, and do not suppress the primary
failure in finalization. The guarded Python harness must be wired into source
CI and include real process-level live-child and reaped-child observations.

## 2026-10-02: PR #22 recovery repair evidence checkpoint

PR #22 head `9f39cfe05835c9dca6b2430d1379dffac840c06d` replaces the helper-only
retention path with an external JavaScript parent that keeps Core and JWKS live
until the Python owner exits, while the Python owner retains its PTY and
immutable execution object until it directly observes the terminal child. The
source guard launches those three processes plus a real reaped-child case;
live observation yields `terminal_exited_zero`, while the reaped case stays
`terminal_unknown` with `child_reaped_unowned`. Finalization now refuses a live
child without that recovery owner. Local source guards passed (19 Python
receipt checks, 10 native-harness checks with platform skips, and 52 Node
checks); they are source evidence only. Exact-head hosted Linux and macOS
native PTY/action/hash/exit/adverse receipts remain queued and are required
before native acceptance can be claimed.

## 2026-10-01: PR #15 exact-head and unavailable observation checkpoint

Natural PR run `36766315178` built and retained source `d51d1c82d884aa3bc5fa3e1707860c914634b64e`, a provider-generated merge commit, because the workflow used the default pull-request checkout. Its retained executable SHA-256 is `a34f4475ad5ea5bcb134dd8329986c1187710187082018485ddb0c8f2bceb05c`. The direct invalid-URL assertion passed for those held bytes; the following ConPTY unavailable probe returned only the closed `startup` / `timeout` / `timed_out` receipt. A controlled local replay of that same digest found the visible unavailable/retry frame and a clean `q` exit; its `q quit` footer was clipped from the terminal capture and was not a valid readiness predicate. The repaired bounded probe requires visible unavailable/retry labels and `q` exit; the TUI's retry transition remains covered by its state tests because this terminal renderer does not emit a separately readable redraw for an equivalent retry outcome. No terminal text, token, nonce, URL, or exception was retained. Repair scope is explicit head checkout and provenance binding, plus this retained executable replay before any rebuild. This preserves the failure and does not establish runtime acceptance, candidate qualification, release, deployment, or GA.

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
