# Issue #20 acceptance matrix

This record keeps source, hosted compilation, direct Core evidence, and
release acceptance distinct for the durable keyboard operator workflows.

## Exact TUI candidate

Current evidence is bound to `develop`
`c82f69048f992ea3635f6147e6401023cefc62c3`. The durable keyboard operations
first landed through [#21](https://github.com/service-lasso/service-lasso-tui/pull/21)
and later Linux lifecycle repairs through
[#39](https://github.com/service-lasso/service-lasso-tui/pull/39). The original
`e7d23695112e87b05d93a81e45117c8f080c6f83` head is retained only as historical
evidence for the first direct-Core attempt.

Both owning paths now implement the durable contract: the default operator/v2
client (`internal/api/operator_mutations.go`) and the explicit `--compat-v1`
legacy adapter (`internal/api/client.go`). The Linux native acceptance phase
runs the default operator/v2 path without `--compat-v1`.

## Requirement and evidence

| Requirement | Source evidence | Direct or hosted evidence | Status |
| --- | --- | --- | --- |
| Five durable actions: install, config, start, stop, restart | `internal/api/client_test.go` checks availability, exact preview, one submit, and readback for all five. `internal/app/model_test.go` checks each keyboard shortcut uses only the selected service and preserves the exact preview. | Linux real-Core: develop run `43702278820` job `132776264736`, artifact `14156787665` verified; `five-actions` result `succeeded`, `actionsPassed: true`, binary `cb8c0280a5f14195e98bd35d891d7f53d9fa2020e6c814e8f35523a91e3a79e1`. Exactly five operations were created for `install`, `config`, `start`, `stop`, and `restart`, each with one matching Core terminal-success audit. | Source and direct Linux real-Core acceptance. Windows remains partial and packaged three-OS evidence remains separate. |
| Denial and retired reload | The model refuses non-durable clients, unavailable Core actions, and `reload`; it creates no confirmation in those cases. | Linux real-Core: missing, invalid, and scope-denied profiles created zero new operations; the denied case produced ten Core denial audits. `reload` stayed disabled and pressing `l` created no operation or confirmation. | Source and direct Linux real-Core acceptance. |
| Frozen preview and one confirmed mutation | Confirmation accepts only `y` or Escape; late dashboard data cannot retarget a preview; repeated `y` does not resubmit. | Linux real-Core: each keyboard action added exactly one new operation with a unique ID and the expected target; an unconfirmed preview created none. | Source and direct Linux real-Core acceptance. |
| Durable readback, reconnect, and no replay | Accepted and uncertain operations retain their original client and refresh reads only that operation. | Linux real-Core: a separate reconnect terminal read back the same five operation IDs, actions, targets, and outcomes; no operation was replayed. | Source and direct Linux real-Core acceptance. |
| Persistent reconciliation context | The production adapter fails closed until Core #1553 provides its reviewed server-issued opaque actor/client/instance context. Tests only use a surrogate provider to prove the refusal boundary. | No current reviewed #1553 adapter is available. | Blocked; no URL, profile, or credential-derived substitute is used. |
| Advertised cancellation only | `z` is enabled only when Core readback advertises cancellation support, and a pending cancellation cannot be dispatched twice. | Linux real-Core: all five readbacks reported `cancellationSupported: false`, and the UI never offered cancellation. Advertised-true cancellation remains source-only. | Source and direct Linux negative path; advertised-true direct evidence remains outstanding. |
| Inbox and health-history read workflows | Existing bounded dashboard client consumes inbox summaries and health transition counts only, without inbox detail or mutation routes. | Not exercised by the native lifecycle phase. | Existing source coverage; direct evidence remains required. |
| Unrelated-service safety | The five-action keyboard matrix includes a second service and asserts availability and preview use the selected service only. | Linux real-Core: `tui20-unrelated` availability and safe lifecycle fields were compared before and after the five accepted actions and were unchanged. | Source and direct Linux real-Core acceptance. |

## Native and direct evidence

### Current Linux real-Core receipt (2026-11-17)

Develop run
[`43702278820`](https://github.com/service-lasso/service-lasso-tui/actions/runs/43702278820)
job `132776264736` passed the native POSIX lifecycle job and uploaded artifact
`14156787665` (`tui20-native-Linux-lifecycle-c82f690...`). The retained artifact
was downloaded and passed `node scripts/verify-tui20-native-public-receipt.mjs`.
It contains seven separate phases: `preflight`, `five-actions`,
`adverse-controller`, `adverse-owner-death`, `finalization-failure`,
`handoff-timeout`, and `shutdown-pipe-failure`.

The `five-actions` projection records:

```json
{"schemaVersion":1,"kind":"tui20-native-public-result","sourceCommit":"c82f69048f992ea3635f6147e6401023cefc62c3","binarySHA256":"cb8c0280a5f14195e98bd35d891d7f53d9fa2020e6c814e8f35523a91e3a79e1","result":"succeeded","actionsPassed":true,"ownedRuntimeClosed":true}
```

The same receipt was also verified at the merged PR #39 head
`de86c24f1bf3513f71e78c7a9be4fa080b054b3b` from run `43561737841` job
`132347213873`, artifact `14101410120`, with binary
`4108bc929f8186b14b7dfdb60e72bba238c3cc6a23cf555e2f1f3fb89452930a`.

These receipts establish direct Linux real-Core five-action acceptance against
Core pin `2633c07be25512d0a84f9bfa28de6be5edff35e8`. They do not establish a
genuine Windows five-action receipt, packaged candidate acceptance, macOS
acceptance (deferred `N/A`, never PASS), release qualification, deployment, or
GA.

### Historical direct-Core attempts (2026-10-02)

An earlier hosted CI run `36870493722` passed against the then-current TUI head
`e7d2369`:

- Windows: source test/build plus the bounded native AMD64 ConPTY
  unavailable-state probe.
- Linux and macOS: native hosted source test/build.
- Linux release job: cross-compiled all declared release targets.

Those receipts established exact-head source compilation and the bounded
Windows unavailable-state probe only.

For that earlier direct current-Core check, owned detached checkouts at
`93d9d343a058d296069c017d17f4f8d1fc1505ea` were built after clean dependency
installations. The initial shared-registry attempt failed before fixture
execution and is retained as failed evidence. A later run used fresh owned
`SERVICE_LASSO_WORKSPACE_ROOT`, `SERVICE_LASSO_INSTANCE_REGISTRY_PATH`, and
`SERVICE_LASSO_HOST_PORT_REGISTRY_PATH` values. Its durable HTTP suite passed
nine of ten cases, including confirmation, idempotency, readback, safe
cancellation, actor scoping, restart recovery, and redaction. The remaining
case failed only while removing its own temporary fixture directory with
`ENOTEMPTY`; no shared registry or service state was changed.

The linked worktree build was correctly rejected because Go did not stamp
linked-worktree VCS metadata. A fresh owned non-worktree clone at the exact
TUI head was then admitted with `vcs.revision=e7d2369…` and
`vcs.modified=false`, and its AMD64 binary reached the real current-Core
dashboard and `echo-service` detail over ConPTY. A missing credential produced
the retained startup denial; an arbitrary local-admin token produced a Core
403 on lifecycle availability. Neither attempt submitted an operation.

The subsequent owned OAuth-bearer profile used the documented loopback Core
JWKS, issuer, audience, actor, client, and lifecycle scopes. It reached the
Core lifecycle availability contract, which returned `409 provider_not_ready`
for every service in the unchanged copied Core service set, including `@node`.
Core therefore issued no preview and the TUI could not submit or mutate. This
is a real current-Core precondition failure, not an accepted-operation receipt.
The ConPTY helper now forwards only the token and connection-profile file that
the explicitly selected TUI profile requires; it neither writes credentials
nor relaxes profile admission.

## Native Windows evidence repair status

The earlier retained experiment at
`D:\projects\service-lasso\_verification\tui20-native-five-action-20261002-020000`
remains preserved as failed receipt evidence. Its bearer fixture was protected
to the owning user without reading it, but it had previously been retained
under permissive access and its harness did not hold the executable or retain
per-terminal true-exit results. It cannot support a complete native acceptance
claim.

The current harness repair keeps generated bearer credentials in the owned
Core parent and inherited child memory only. It retains no credential file,
holds and hashes the executable through every ConPTY launch, records a closed
exit classification per terminal, reads back Core's materialized instance and
both registries after startup for the three distinct owned paths (`SERVICE_LASSO_WORKSPACE_ROOT`,
`SERVICE_LASSO_INSTANCE_REGISTRY_PATH`, and
`SERVICE_LASSO_HOST_PORT_REGISTRY_PATH`), compares named safe lifecycle fields
before and after for `tui20-unrelated`, and records direct Core audit-event
counts plus operation counts for each adverse case. It records completed
operation reconnect separately from pending reconciliation, which remains
blocked on Core #1553. The current fixture does not advertise cancellation, so
it does not fabricate an advertised-true cancellation result.

A fresh owned rerun with the product-identical clean `6f6dfd9` executable
(`a5fd433c78f479391fa6d426ac6087a1ebed01caf9144771fafb675eccbf9400`) retained
its failed transcript when the Core readback became unavailable while awaiting
the restart result. The retained Core audit later records that the restart
operation reached its terminal success, while the TUI had already shown an
unavailable readback and the parent stopped Core only after the child exited;
there is no Core stderr artifact or still-running owned PID. The repaired
harness reconnects only to read that retained operation and records a closed
failure category if that readback cannot complete. The failed run remains not
an acceptance receipt. The later harness
head changes no Go product source, but this must continue to be stated as a
`6f6dfd9` binary claim until a clean exact-head binary is independently built
and rerun.

Windows direct lifecycle acceptance is therefore **partial and blocked**:
source tests and the earlier bounded fixture exercise remain useful, while a
successful secrecy-safe, same-handle, true-exit receipt remains required.
The Linux real-Core native journey now has a completed native-host receipt on
the current head, using the executable native-host workflow introduced on TUI
PR #22. It obtains clean detached Core
`2633c07be25512d0a84f9bfa28de6be5edff35e8`, runs `npm ci` and `npm run build`,
and starts that actual Core source only after a fresh shell creates three
distinct, non-link owned workspace/instance-registry/host-port-registry paths
with both registries absent. Before Core import, dependency installation, or a
build tool runs, that shell retains source tree and dirty hashes, its actual
PID/PPID/birth/image, the planned command, and hash-and-size identities for the
native compiler tools in `input-custody.json`. Each native host builds a
clean VCS-stamped TUI binary, acquires a no-follow verified descriptor, and
drives it through a POSIX PTY only from a platform-enforced immutable execution
object (`fexecve` on a sealed Linux `memfd` and Darwin relative-pathname
`execve` after `fchdir` through a held directory descriptor on a
system-immutable object). Before acceptance actions, the
harness replaces the executable pathname by rename and symlink attacks and
mutates the original inode in place; each immutable-object launch must still
reach the expected missing-credential boundary. The retained receipt contract includes binary
  digest/size, source-tree heads and dirty hashes, Core path
  materialization readback, true terminal exits, five accepted operations,
adverse zero-operation/direct-audit counts, exactly one matching Core terminal
success audit event per distinct operation ID, no-replay reconnect, and named
  unrelated-service comparison. Preflight, five-action, injected-finalization,
  and controller-loss phases retain separate attempt roots, binary hashes, and
  owner birth/close records; no later phase may replace another phase's primary
  or cleanup receipt. The injected cleanup-failure phase proves that a real
  owner gracefully closes Core/JWKS after finalization cleanup has failed.
  Core #1553 remains fail-closed for persistent
reconciliation. The exact-head Linux job completed and its retained receipt was
verified (run `43702278820`, artifact `14156787665`). macOS remains deferred
`N/A`, never PASS. Windows remains **Blocked** until a genuine five-action,
secrecy-safe, same-handle, true-exit receipt exists; hosted builds and
cross-compilation are not replacements.

### Executable-byte binding

The receipt must show that the exact bytes launched by the PTY are
platform-enforced immutable. Linux uses a verified `memfd` with kernel readback
of `F_SEAL_WRITE`, `F_SEAL_GROW`, `F_SEAL_SHRINK`, and `F_SEAL_SEAL`, then
launches that anonymous object with `fexecve`. macOS requires a copied verified
object with the OS-reported `SF_IMMUTABLE` system flag and executes through its
held directory descriptor with relative pathname `execve`; the runner must be
able to set and later clear that system flag through non-interactive privilege.
It retains a writable leaf descriptor before leaf activation and requires the
kernel to deny its post-activation in-place write, hashes the immutable leaf
against the held candidate digest, then flags and reads back the parent. A user
immutable flag, POSIX
permissions, an ACL/DACL, a hash re-run, or an open read descriptor is not
equivalent. The harness performs an adversarial in-place mutation of the source
inode after Linux sealing and requires the sealed launch to succeed, while the
Darwin system flag must deny both a new writable open and the pre-existing
writable descriptor. The primary metadata-only exit receipt is fsync-persisted
before immutable-object teardown; a separate closed cleanup receipt records
cleanup outcome and recovery retention without replacing the primary child
outcome. If leaf activation succeeds but parent activation/readback fails, the
held cleanup descriptor reaches primary-before-cleanup finalization; rollback
is attempted there and any unproven rollback truthfully retains recovery
material without recording a filesystem path. A bounded `q` observation never
force-kills its owned child or tears down its execution object while that child
is live; an observed negative wait status is recorded as `terminal_signaled`.
A missing seal/flag readback, denied-write probe, or native-host
receipt is **Blocked**.

Every phase records its clean TUI source facts and requested Core revision
before it fetches Core, installs dependencies, builds, or imports runtime code.
The owner-private custody record preserves actual path, process, image, tool,
and literal-command details with fsync durability, but is never uploaded. The
public artifact uses separate closed input, Core-source-binding, and build
output schemas. It contains only source identities, ownership predicates, and
verified tool names. PTY bytes remain memory-only assertion input and are never
written to a terminal transcript or artifact. Existing private failure evidence
is preserved under its existing authority; this workflow does not create a new
private artifact permission.

The uploaded executable receipt is a closed JSON digest-and-size schema, rather
than `sha256sum` output that includes the runner-local binary path. The upload
step parses every selected public receipt before transfer, requires its exact
schema, and rejects paths, PID/PPID or birth fields, private custody labels,
images, tool records, and literal commands. CI and candidate-publication
Actions are resolved to verified committed SHA references; their version labels
remain comments for auditability.

The mechanisms are constrained by their native operating systems, rather than
by advisory process behavior: Linux documents sealing for `memfd_create` and
its `F_SEAL_WRITE` restriction in [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html),
and `fexecve(3)` documents descriptor-selected execution. Apple documents that
the owner may clear `UF_IMMUTABLE`, while only the superuser can set or clear
`SF_IMMUTABLE`, in [chflags(2)](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/chflags.2.html).

## Remaining boundary

The Linux default operator/v2 path now has a verified real-Core five-action
receipt on `c82f690`. The following remain outstanding and are not implied by
that receipt:

- A separate reviewed Core #1553 context adapter is required before persistent
  operation reconciliation can be claimed.
- Genuine Windows five-action acceptance requires a secrecy-safe, same-handle,
  true-exit receipt; the existing Windows result remains partial and blocked.
- macOS remains deferred `N/A` and is never recorded as PASS.
- Packaged same-byte Core+TUI acceptance and release qualification remain
  separate gates and are not established by the source-built native receipt.
