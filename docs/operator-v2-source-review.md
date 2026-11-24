# Operator/v2 source review

Status: source review only. Not independent ROOT, qualification, or GA.

Current status (24 November 2026): the v2 code-gap findings below were addressed
in Core #1945-#1960 and the follow-up mutable-fixture work. The mutable harness
passes against one Core instance with CLI preview/execute/readback/stop and TUI
detail/availability/preview. The sections below are retained as historical review
context; the remaining non-code blockers are authentic production originals and
independent admission/ROOT/GA.

## Scope

Reviewed the Core v2 contract/routes, CLI v2 client/commands, TUI v2
client/probe, and the cross-client acceptance harness.

## Verified

- TUI `go test ./...` passes.
- CLI v2 client and command tests pass.
- Core v2 contract and route tests pass.
- Cross-client probe passes: one Core instance serves CLI discovery/services and
  the TUI v2 probe with the same discovery revision.
- V1 lifecycle HTTP routes and v1 TUI/CLI product paths are removed.
- Mutable cross-client harness passes with the admitted local-disposable fixture.
- v2 service and context auth preserve MCP denial when OAuth is configured.

## Historical gaps (addressed in current develop)

### Critical: v2 service revisions are placeholders

`src/server/operator-v2.ts` and the route block use `definition-1`,
`generation-1`, and a static model epoch. `src/server/index.ts` hard-codes
`definitionRevision: "definition-1"` and `expectedRevision: "definition-1"` in
the preview response. The TUI validates the response against its request, so a
real revision cannot be used without changing Core.

### Critical: v2 execute ignores plan and revision binding

`src/server/index.ts` accepts `planId` and `expectedRevision` but does not
compare them to the preview or the service. It executes from the confirmation
phrase. A stale or foreign plan can reach the guarded action as long as Core's
confirmation checks pass.

### High: lifecycle availability is static

`buildOperatorV2LifecycleAvailabilityReady` marks all eight actions available
without evaluating actor scopes or per-service preflight. Core rechecks, so this
does not grant authority, but clients are told actions are available when they
are not.

### High: discovery capabilities are not scope-aware

`buildOperatorV2Discovery` advertises `read_logs` with the logs scope regardless
of the authenticated actor. Discovery is a static capability list, not an
authorization result.

### High: service detail is placeholder data

`buildOperatorV2Service` sets `installed: "yes"`, `configured: "unknown"`, and
static revisions. Dependencies, endpoints, setup, and action metadata are empty.
The TUI detail/lifecycle surfaces therefore do not reflect the real service.

### High: detached operation recovery is not wired

The v2 readback/cancel routes construct `McpOperationService` without the
detached recovery callbacks used by the retired v1 route. A detached operation
may not reconcile/cancel correctly through v2.

### High: v2 context auth is MCP-only and untested with TUI local-admin

`/api/operator/v2/reconciliation-context` uses the MCP auth path. The
cross-client test covers discovery/services only; it does not prove TUI
local-admin context binding.

### Medium: errors are not v2 envelopes

Core v2 routes throw the shared `ApiError` response shape, not the operator/v2
error envelope expected by the TUI’s `OperatorPublicError` decoder. Error paths
can fail to produce typed v2 messages.

### Medium: CLI defaults are not bound to availability

`operator preview` defaults both revisions to `definition-1`, and `operator
cancel` does not require `--confirm`. The caller can send a stale plan; Core
does not reject it.

### Entry boundary gaps remain

The producer still maps bundle files into image sections rather than
constructing native originals, and the product entry still returns
`NativeRetainedUnavailable` without authentic originals.

## Review result

Source review: findings above.
ROOT admission: not issued.
Qualification: `NOT_QUALIFIED`.
Release status: `NOT_GA`.
