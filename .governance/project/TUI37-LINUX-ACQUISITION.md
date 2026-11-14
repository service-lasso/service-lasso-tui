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
