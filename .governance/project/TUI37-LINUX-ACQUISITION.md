# Issue 37 Linux acquisition source slice

Issue: https://github.com/service-lasso/service-lasso-tui/issues/37
Base: develop 9637ac480c7a172daa44afb50c834a6a9c05f8a3.
Branch: feature/37-linux-acquisition. Current Linux session owns development/review/testing and normal eligible develop merge; release remains owner-only. The user clarified that the Windows handoff addresses this session.

## Intent and mapping

Implement an internal Linux executor for the existing linuxCandidateBuildPlan.
SPEC-004 TUI28-NATIVE maps ordered source generation, overlay test, payload,
complete owner/libsepol compilation and linking. TUI28-DENIALS maps missing and
aliased originals, changed retained inputs, command failures and output substitution.
TUI28-CLOSURE maps private retained failures, distinct review and truthful evidence.
This is a bounded source component, not completion of issue 37.

The executor retains case-sensitive Linux original input/output identities and
bytes, commands, separate stdout/stderr originals, first and later failures,
and explicit once-close custody. It consumes actual tool/source/profile/ABI/
native-geometry inputs and returns a private attempt owner; no callback or
readiness flag supplies authority. Windows behavior and public schemas stay unchanged.

## Acceptance

- Exact clean source and Linux amd64 Go 1.26.0 checked before build commands.
- Same complete input profile generates Go and C; original digests rejoin both outputs.
- Exact overlay precedes candidate tests and payload; complete source roster precedes link.
- Original input/output alias, replacement and changed-byte cases refuse.
- Independent capture/revalidation attempts preserve first opaque failure and later originals.
- No fallback, fabricated entry/profile, production activation or native PASS.
- Distinct review precedes admitted target execution; native proof remains conditional
  on authentic complete inputs, actual native entry/constructors and matching-host admission.

## Known prerequisites

The current Linux emitter reports nativeEntrySupplied:false and emits no main/_start.
A link failure therefore remains incomplete, never native acceptance.
Authentic SLCPROF1, both geometry originals and genuine entry/provider/family/finance
custody are not supplied by the handoff comment. Native geometry requires body_bytes
(equal to profile representationBytes), not representation_bytes. Windows work is deferred.
Current CLI develop df5c1b62d64061d3d8117aa998e96597fcd5834b documents missing
prior-domain producer, child-local transfer and fixed activation caller in
CLI39-INITIAL-LAUNCHER-PRODUCER36-FRONTIER.md. No substitute ABI is selected.

## Verification disposition

Executor source and refusal/custody fixtures are authored, pending distinct review. Bounded source tests were explicitly requested by the current user; their execution does not grant native input or release authority. Linux native testing needs the genuine
originals and current exact-command admission. Host availability alone grants none.

Host inspection: command -v finds no Go or clang; /usr/local/bin/node resolves to /opt/node-v24.21.0-linux-x64/bin/node, not the required Node 22.23.2. No Go/native build or proof command has run. Static git diff --check passes. Authentic input originals remain unavailable.

Bounded source validation: `node --test scripts/linux-candidate-executor.test.mjs scripts/file-backed-command-completion.test.mjs scripts/candidate-profile-source.test.mjs` passed 18/18, zero skips, on the available Node24 host. This is not Node22.23.2 matching-toolchain or native proof. Independent source review remains required before landing.

## Effective lifecycle diagnostic continuation

The current user explicitly requests fixing the inherited lifecycle stall and
landing PR39. Actual legacy CI is tui20-native-core.mjs -> tui20 observer.
Add literal helper stage markers plus a concurrent SAME stderr collector in
the actual tui20 observer: retain every raw chunk/read/forward original privately,
forward only exact complete allowlisted literal lines before helper exit, and
join the sole reader before once-close. No competing read, raw error projection,
forced shutdown, changed deadline, skipped proof, source pin or success shortcut.
Real child tests must prove early marker visibility, secret/nonexact-line denial,
raw retention and read/forward failure custody. These changes diagnose, never
qualify, the legacy compat fixture. Review and focused tests precede pushes.

Matching Go1.26.0 and Node22.23.2 are being acquired into an owned user cache
from official distributions with published digest verification to reproduce the
legacy matching-host probe; this grants no genuine v2 input/entry authority.

## Legacy recovery semantics repair

Independent bounded intent review permits the existing normal operator q on
failed legacy fixture recovery. Root the per-Terminal quit attempt before write,
at most once, retain first assertion and all later originals, persist failure
before recovery, and drain SAME PTY while observing actual owned waitpid exit.
Preserve normal close's original ten-second deadline; recovery adds no deadline.
EOF/EIO/read failure never means exit, and a refusing live child retains custody
indefinitely. No kill or false primary success. Real PTY tests must demonstrate
output-backpressure drain, direct reaping and failed primary retention.
A local prepare attempt built pinned Core successfully but refused dirty TUI
source; it is failed diagnostic preparation, not proof.

## Observed invalid-credential fixture mismatch

The local legacy probe at bb4bf4645b94af329c0125066333165488b95df2 built
from clean source and pinned Core, passed replacement/missing-credential stages,
then failed at invalid-credential and recovered via observed natural exit.
It returned failure without the former indefinite stall. Correct the invalid
credential journey to observe the exact safe 401 authentication-denied dashboard
state; an unauthorized connection cannot navigate service detail. Retain actual
zero-operation, zero-audit and terminal-zero expectations. Do not replace these
with generic error acceptance. Permission-denied assertions remain separately
verified against actual scoped Core behavior.

Go1.26 VCS discovery recognizes .git directories, not managed worktree .git
files. The detached validation worktree therefore produced no VCS stamp and
failed the required metadata grep. A clean issue-branch-only validation clone
with a .git directory passed metadata verification; no stamp guard was bypassed.

Regression evidence includes actual HTTP service-read responses for 401, 403 and 503, exact distinct safe titles, no returned services, no raw response body presentation, and exactly one GET. The invalid-terminal fixture must issue no detail/install interaction and retain zero-operation/audit and actual terminal-exit assertions.
