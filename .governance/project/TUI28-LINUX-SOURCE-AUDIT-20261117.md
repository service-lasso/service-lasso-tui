# TUI28 Linux source audit (2026-11-17)

Issue: https://github.com/service-lasso/service-lasso-tui/issues/28
Base: develop c82f69048f992ea3635f6147e6401023cefc62c3
Scope: Linux source route for SPEC-004 TUI28-CANDIDATE/NATIVE/AGGREGATE/
PUBLISH/DENIALS. This record is source inspection only. It does not dispatch,
build, run, publish, qualify, admit, merge or close anything.

## Result

No Linux-specific source gap was found in the scoped workflow, aggregate,
archive verifier or protected publisher. The Linux native acquisition step is
intentionally fail-closed, so the source-owned route cannot produce a candidate
until the separate #37 inputs and constructors exist. That is a truthful source
boundary, not a Linux wiring defect.

Canonical policy bytes remain unchanged:
`159d644c161cf532c94d3bfe17ed55e32bf94c5d2843928945c450f6d8140c12`
(blob `e694c3e314c1bf11e03dc4656730a96467a765b8`).

## Findings by contract

- TUI28-CANDIDATE: `.github/workflows/release-scoped.yml` is selected only for
  `refs/heads/develop`, checks out the exact bound commit, and builds the native
  matrix on `windows-latest` and `ubuntu-latest`. The Linux job runs the producer
  only on the matching host and uploads the current
  `scoped-native-linux-${run_id}-${run_attempt}` artifact. No macOS or Darwin
  prerequisite is present. `bind-scoped-source.mjs` rechecks clean source,
  workflow identity, version-to-short-SHA binding and canonical policy.
- TUI28-NATIVE: `produce-scoped-native.mjs` `produceNative()` performs only the
  policy assertion and then throws
  `genuine native-owner production acquisition unavailable`. It accepts no
  caller authority, contacts no provider, creates no output and cannot fall back
  to the private historical Go-only body. `native-pair-wiring.test.mjs` asserts
  that refusal, no provider call and no output directory. Linux execution remains
  blocked on authentic SLCPROF1, closed Go/C ABI JSON, native geometry,
  constructors, named host/toolchain owners and #37 admission. Windows parity and
  the reviewed query-profile input also remain outstanding.
- TUI28-AGGREGATE: `bind-scoped-source.mjs select-native` obtains artifact IDs
  from `currentAttempt`, which requires the successful `validate-source` job and
  exactly the two current native jobs/artifacts. `assemble-scoped-candidate.mjs`
  re-fetches the same provider jobs and raw artifact ZIP bytes, then
  `assertAggregate` binds source, run, attempt, workflow SHA, platform order,
  job IDs, artifact IDs, wrapper schema, archive and binary. The Linux archive
  verifier requires the canonical USTAR gzip, mode 0755, one executable member,
  no trailing/path-escape members, x86-64 ELF identity and exact producer-byte
  reconstruction.
- TUI28-PUBLISH: the publisher requires the protected `development-candidate`
  environment, immutable releases, a bounded wait timer, and the four scoped
  required checks (`Linux test and build`, `Windows test and build`, `Windows
  owned GOROOT overlay tests`, `Scoped release asset cross-compilation`) with no
  Darwin prerequisite. It admits the four public files, repeats policy equality
  before each mutation, denies existing-tag/orphan collisions without overwrite,
  and requires exact private-draft and immutable public readback.
- TUI28-DENIALS: existing source regressions cover missing and legacy-only CI
  substitution, mixed attempt, wrong-source aggregate, schema downgrade, wrong
  ELF architecture, path escape, archive substitution, coherent held/parser
  substitution and stale artifact identity. No Linux-only denial was missing
  from that inventory.

## Residual observations

- Simultaneous dispatches for the same tag are not serialized by a workflow
  `concurrency` group. Collision checks still fail closed at tag creation and
  release lookup, so there is no overwrite or duplicate-success path in source.
- The native jobs rely on the provider's default job deadline. The publisher
  retains its required 30-minute deadline. This is not a Linux correctness gap.
- The producer fail-closed check is the only reachable production entry. The
  historical Go-only body is private and cannot issue a v1 or v2 wrapper.

## Verification and disposition

The same focused source suites used for the #29 packet (`scoped-candidate`,
`native-owner-package`, `native-pair-wiring`, `verify-candidate-publication` and
its fresh-job case) pass 117/117 with the cached Node 22.23.2 toolchain. This is
bounded source validation, not native proof, provider execution or admission.

No source change is justified by this audit. Independent ENTIRE review and a NEW
complete-input ROOT admission remain mandatory. #28 stays open and source-only;
no candidate, publication, qualification, Core pin, GA or production catalog
state is claimed.
