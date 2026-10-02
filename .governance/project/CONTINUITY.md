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

PR #22 head `9f39cfe05835c9dca6b2430d1379dffac840c06d` is an incomplete
recovery repair. Its JavaScript parent waits for a Python helper and then
unconditionally stops Core and JWKS. The Python helper is also the only owner
of the PTY and immutable execution object, so a helper crash can still release
all three while its terminal child remains live. The existing self-test starts
no Core or JWKS dependency and asserts a parent declaration rather than their
observed liveness.

The active repair creates a distinct Python recovery owner. The owner holds
the PTY and platform immutable execution object, directly observes its child,
and remains alive after a separately launched helper exits abnormally. The
JavaScript parent retains Core and JWKS until that owner writes an
owned-exit-or-unowned-reap outcome. The adverse guard must prove a live child,
held immutable resource, and live Core/JWKS endpoints after helper failure,
then prove natural owned exit before dependency cleanup. `ECHILD` stays an
unowned `terminal_unknown` result and does not replace the primary receipt.
This is source guard work only; exact-head hosted Linux and macOS native
PTY/action/hash/exit/adverse receipts remain required before native acceptance
can be claimed.

Local Windows source checks completed for this repair: 29 Python receipt and
native-harness checks, 52 Node checks, and the Go suite/build passed. The
process-level guard is intentionally Linux-only because it requires a real
sealed `memfd`; it was skipped on this Windows checkout. CI invokes the guard,
and native artifacts retain any recovery receipts it produces. A queued or
failed hosted run remains blocked evidence, not native acceptance.

## 2026-10-02: PR #22 actual external resource-owner checkpoint

The prior source guard remained unsound because the JavaScript controller
started the real Core/JWKS objects and the sole Python helper owned the PTY and
immutable execution object. This repair makes that controller volatile. A
distinct POSIX resource owner starts the actual Core/JWKS runtime as its child,
receives only an in-memory ready handoff, and itself retains the terminal PTY,
immutable executable object, and the Core runtime process identity. It stops
Core/JWKS only after terminal finalization records a verified child terminal
result or an explicit unowned-reap outcome.

The wired Linux and macOS native workflow now runs an adverse production-path
exercise after the five-action acceptance: it starts a real TUI child against
the real pinned Core/JWKS runtime, confirms the resource-owner boundary, kills
the actual JavaScript controller, verifies the TUI child and both dependencies
remain live, then requests normal TUI `q` exit and retains the closed receipt.
No descriptor is claimed to survive the killed controller, no unknown process
is killed, and no fixture lifetime is extended. Linux uses the sealed memfd
path and Darwin uses the existing system-immutable held-directory execve path;
the same owner/controller architecture is shared by both. Local source checks
are only guards. Exact-head hosted Linux and macOS receipts remain required
before native acceptance can be claimed.

## 2026-10-02: PR #22 closed public evidence checkpoint

The prior native artifact list uploaded raw `native-terminal.txt`, and its
pre-fetch `input-custody.json` combined public source facts with owner-private
absolute paths, process lineage, birth values, image locations, and tool
locations. The corrected workflow keeps raw PTY chunks only in process memory;
it does not delete any previously retained private failure evidence. Each
phase now fsyncs an owner-private actual-custody record before Core fetch,
dependency work, build, or import. That record contains the literal command
sequence and actual tool records, including the shell, Git, hash and text
utilities, Node/npm, Go/compiler, and Python. It is deliberately not an
uploaded artifact because this workflow has no typed private-artifact
permission. The public artifact set instead contains separate closed-schema
input, pre-`npm ci` Core-source-binding, and post-build-output receipts. No
host path, PID/PPID, birth value, image location, tool path, raw terminal
content, URL, token, or error payload is public evidence. Hosted run results
remain pending direct Linux/macOS evidence; source guards do not change that
classification.

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

## 2026-10-03: PR25 mandatory cleanup receipt persistence recovery
Explicit sole-writer recovery continues the existing fix/18-tag-policy-write-gates
from clean da17/tree d308 against provider develop6d357. All inherited tracked
source is retained; no new branch or main input. Issue20/SPEC001 successor fixes
mandatory cleanup receipt persistence independently of true OS cleanup and primary
results. Actual regressions remain unexecuted; fresh entire review and NEW ROOT
admission are mandatory. F3 failed Darwin runtime producer remains unresolved;
parent will disposition the separate read-only input investigation before freeze.
No local imports/execution/parser/tests/compiler/helpers/install/ACL/lifecycle,
rerun/dispatch/settings/safety weakening or retained-state cleanup is authorized.
## Issue20 PR25 F3 accepted explicit-FIFO descriptor source proposal
SPEC001 TUI-ACCEPTANCE-003 retains the mandatory actual native producer and
its selected Bash/parser/quoted heredoc/options. The parent accepted replacing
read -r -t 1 writer_ready <&9 with read -r -t 1 -u 9 writer_ready so peer-ready
FIFO consumption does not redirect script-input fd0. GNU/Apple published text
supports descriptor isolation, not an exact source-to-Darwin-image binding or
causal defect. Seekback/undo counterevidence is retained. Preserve status/variable,
actual jobs-alive policy/$! identity/reciprocal FIFO birth-image-PPID checks,
failed-frame/EOF/overflow denial/trap actual wait/private fsync/success wait.
Existing actual producer plus writer-death/producer-failure adversaries remain
primary and bind the new operation. Original da17 runtime failure remains FAILED;
paired -n stdin/-c zero results do not prove runtime success or causal repair.
Proposal remains UNQUALIFIED until exact new candidate natural mandatory Darwin
producer succeeds. DIFFERENT ENTIRE review and NEW ROOT admission precede local
execution; all R1 and prior native/publisher/full-delivery requirements remain.