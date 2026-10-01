# Issue #20 acceptance matrix

This record keeps source, hosted compilation, direct Core evidence, and
release acceptance distinct for the durable keyboard operator workflows.

## Exact TUI candidate

- Branch: `feature/20-durable-keyboard-operations`
- Qualified direct-binary source: `e7d23695112e87b05d93a81e45117c8f080c6f83`
- Base: `develop` `f99e8d493b19b088c9f6c9d9b2aacb9d25388094`
- Pull request: [#21](https://github.com/service-lasso/service-lasso-tui/pull/21)

## Requirement and evidence

| Requirement | Source evidence | Direct or hosted evidence | Status |
| --- | --- | --- | --- |
| Five durable actions: install, config, start, stop, restart | `internal/api/client_test.go` checks availability, exact preview, one submit, and readback for all five. `internal/app/model_test.go` checks each keyboard shortcut uses only the selected service and preserves the exact preview. | CI run `36870493722` compiled and tested this exact head on Windows, Linux, and macOS. | Source and hosted compilation covered; no full real-Core TUI mutation receipt. |
| Denial and retired reload | The model refuses non-durable clients, unavailable Core actions, and `reload`; it creates no confirmation in those cases. | Not exercised against the current Core runtime. | Source covered. |
| Frozen preview and one confirmed mutation | Confirmation accepts only `y` or Escape; late dashboard data cannot retarget a preview; repeated `y` does not resubmit. | Not exercised against the current Core runtime. | Source covered. |
| Durable readback, reconnect, and no replay | Accepted and uncertain operations retain their original client and refresh reads only that operation. | Not exercised against the current Core runtime. | Source covered. |
| Persistent reconciliation context | The production adapter fails closed until Core #1553 provides its reviewed server-issued opaque actor/client/instance context. Tests only use a surrogate provider to prove the refusal boundary. | No current reviewed #1553 adapter was available. | Blocked; no URL, profile, or credential-derived substitute is used. |
| Advertised cancellation only | `z` is enabled only when Core readback advertises cancellation support, and a pending cancellation cannot be dispatched twice. | Current-Core lifecycle run did not reach operation execution. | Source covered. |
| Inbox and health-history read workflows | Existing bounded dashboard client consumes inbox summaries and health transition counts only, without inbox detail or mutation routes. | Not re-executed in the current-Core attempt. | Existing source coverage; direct evidence remains required. |
| Unrelated-service safety | The five-action keyboard matrix includes a second service and asserts availability and preview use the selected service only. | Not exercised against the current Core runtime. | Source covered. |

## Native and direct evidence

CI run `36870493722` passed against the exact TUI head:

- Windows: source test/build plus the existing native AMD64 ConPTY
  unavailable-state probe.
- Linux and macOS: native hosted source test/build.
- Linux release job: cross-compiled all declared release targets.

Those receipts establish exact-head source compilation and the bounded Windows
unavailable-state probe. They do not establish an authenticated Core lifecycle
operation, packaged candidate acceptance, release qualification, deployment,
or GA.

For a direct current-Core check, owned detached checkouts at
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
Linux and macOS real-Core native journeys now have an executable native-host
workflow on TUI PR #22. It obtains clean detached Core
`2633c07be25512d0a84f9bfa28de6be5edff35e8`, runs `npm ci` and `npm run build`,
and starts that actual Core source only after setting three distinct owned
workspace/instance-registry/host-port-registry paths. Each native host builds a
clean VCS-stamped TUI binary, acquires a no-follow verified descriptor, and
drives it through a POSIX PTY only from a platform-enforced immutable execution
object (`fexecve` on a sealed Linux `memfd` and Darwin relative-pathname
`execve` after `fchdir` through a held directory descriptor on a
system-immutable object). Before acceptance actions, the
harness replaces the executable pathname by rename and symlink attacks and
mutates the original inode in place; each immutable-object launch must still
reach the expected missing-credential boundary. The retained receipt contract includes binary
digest, source-tree heads, Core path
materialization readback, true terminal exits, five accepted operations,
adverse zero-operation/direct-audit counts, exactly one matching Core terminal
success audit event per distinct operation ID, no-replay reconnect, and named
unrelated-service comparison. Core #1553 remains fail-closed for persistent
reconciliation. Until the exact-head Linux and macOS jobs complete and their
receipts are read back, this is an executable path and remains **Blocked**, not
native acceptance; hosted builds and cross-compilation are not replacements.

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

The mechanisms are constrained by their native operating systems, rather than
by advisory process behavior: Linux documents sealing for `memfd_create` and
its `F_SEAL_WRITE` restriction in [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html),
and `fexecve(3)` documents descriptor-selected execution. Apple documents that
the owner may clear `UF_IMMUTABLE`, while only the superuser can set or clear
`SF_IMMUTABLE`, in [chflags(2)](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/chflags.2.html).

## Remaining boundary

A separate reviewed Core #1553 context adapter is required before persistent
operation reconciliation can be claimed.
