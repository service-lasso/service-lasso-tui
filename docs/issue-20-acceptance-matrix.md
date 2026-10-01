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

## Required next evidence

The Core owner must provide a governed, ready normal service state whose
documented lifecycle provider prerequisites are met. Then a fresh owned
exact-Core run must execute the TUI binary through native terminal input
against isolated services and a unique loopback port, covering authenticated
allow and deny, the five action matrix, confirmation, one submission, readback,
advertised cancellation, reconnect, and preservation of an unrelated service.
A separate reviewed Core #1553 context adapter is required before persistent
operation reconciliation can be claimed.
