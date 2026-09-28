# TUI framework decision

## Decision

Use **Go with Bubble Tea** for the first Service Lasso TUI foundation.

## Evaluation

| Option | Keyboard/event model | Single-binary distribution | Runtime API client | Decision |
| --- | --- | --- | --- | --- |
| Bubble Tea (Go) | Native message/update loop; testable state transitions | Strong cross-platform static-binary path | Go standard library HTTP client | Selected |
| Ratatui (Rust) | Strong event model | Strong cross-platform path | Good HTTP ecosystem | Not selected: introduces a new Rust toolchain where Service Lasso already ships Go harness tooling |
| JavaScript terminal UI | Familiar API work | Requires Node runtime distribution | Good HTTP ecosystem | Not selected: adds a runtime dependency to a standalone operator tool |

Bubble Tea does not decide the final mutating-operation policy. This foundation only
reads `GET /api/health` and `GET /api/services`; start, stop, install, update and
other mutations need endpoint-specific confirmation and audit rules before being
added.
