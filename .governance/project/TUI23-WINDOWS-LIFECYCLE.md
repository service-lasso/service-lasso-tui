# TUI #23 Windows-native five-action lifecycle acceptance

Issue: https://github.com/service-lasso/service-lasso-tui/issues/23

## Scope

A dedicated Windows ConPTY acceptance for the current TUI. It builds an exact
VCS-stamped `service-lasso-tui.exe` from the checked-out source and runs the
five durable lifecycle actions once each against a source-built authenticated
Core/JWKS runtime:

- install
- config
- start
- stop
- restart

The acceptance also observes frozen preview/confirmation, durable operation
readback, no replay after reconnect, unchanged unrelated-service state, a real
terminal resize, and a real reconnect. It retains actual child exits and Core
operation/audit counts.

## Implementation

- `scripts/tui23-windows-lifecycle.py` — ConPTY owner for the current TUI.
- `scripts/run-windows-native-lifecycle.mjs` — verifies the exact TUI binary
  and built Core, runs the Windows observer/runtime/owner, validates closed
  receipts, and writes `windows-native-lifecycle-summary.json`.
- `scripts/run-windows-native-lifecycle.test.mjs` — closed-summary refusal tests.
- `scripts/local-e2e.test.mjs` / `local-e2e.yml` source checks: this PR adds
  only source tests; no native lifecycle CI job is enabled.

The target and unrelated services are the existing source-owned
`tui20-native-runtime.mjs` fixture services used by the POSIX native lifecycle
acceptance. The Core is real and authenticated; the fixture services are not a
substitute for Core or TUI bytes.

## Boundary

This is #23 lifecycle acceptance only. It is not the #37 v2 native-owner
acquisition, is not an installed same-published-byte qualification, and grants
no provider mutation, publication, GA, or release authority. Raw terminal
output and bearer credentials stay memory-only; the uploaded evidence is the
closed summary.

## Core identity

`f32270731ff42f2a2f6dbcbc02f188b3fdd8813b` is the exact Core pin used by
the merged #44 ordinary Windows ConPTY E2E (`local-e2e.yml`
`CORE_REVISION`). It is the merged Core develop commit
`test: support private Windows ordinary Core fixtures (#1926)` that added the
private Windows fixture support the runner requires, and the #44 hosted run at
that pin passed Windows and Linux (run 43617334472). It is an accepted source
pin, not merely an ancestor of Core develop.

## Evidence and disposition

Source-only. On Windows, `node --test scripts/local-e2e.test.mjs` passes 9/0/1
with only the intended POSIX-only skip. `node --test
scripts/run-windows-native-lifecycle.test.mjs` covers the closed summary,
preview/readback/replay/unrelated/resize/reconnect/exits, and fail-closed
refusals. **Windows native lifecycle: UNRUN** until independent review, a new
authentic ROOT, and exact-command admission. This PR does not close #23 and
does not claim #37 closure.
