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
- `.github/workflows/ci.yml` — `windows-native-lifecycle` job on
  `windows-latest`, pinned Core `2633c07be25512d0a84f9bfa28de6be5edff35e8`,
  Node 22.23.2, Go 1.26.0, Python 3.12 x64, pywinpty 3.0.5.

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

## Evidence

A local Windows x64 run at TUI `714e5a51d7a9da9e0d388bc2139dcf48456d9307`
and Core `2633c07be25512d0a84f9bfa28de6be5edff35e8` passed: five operations,
five Core audit entries, actual terminal exits zero, unrelated service
unchanged, reconnect with no replay, resize, and frozen preview. The hosted CI
run and artifact are attached to the pull request.
