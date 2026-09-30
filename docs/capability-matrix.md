# TUI capability matrix

## TUI-CONNECTION

| Requirement | Current contract | Status |
| --- | --- | --- |
| `TUI-CONNECTION-001` | Named metadata-only profiles are selected with `--profile`, `SERVICE_LASSO_API_PROFILE`, or the configured default; `p` opens the keyboard selector. | Implemented; flags override environment, which overrides profile metadata. |
| `TUI-CONNECTION-002` | A profile declares `tokenEnv`; its environment value is sent only as `x-service-lasso-admin-token`. | Implemented against Core `develop` `387726b`; no bearer scheme is required or selected by the TUI. |
| `TUI-CONNECTION-003` | Token-bearing non-loopback HTTP is rejected; redirects are not followed and standard TLS verification remains enabled. | Implemented and locally tested. |
| `TUI-CONNECTION-004` | Non-2xx bodies are not retained; authentication and permission denials render only method, path, and HTTP status. | Implemented and locally tested. |
| `TUI-CONNECTION-005` | Reconnect retains a stale snapshot only for its current connection; switching clears it and discards old asynchronous results. | Implemented and locally tested. |
| `TUI-CONNECTION-006` | Core #1462 registration and Core #1465 lifecycle are separate dependencies | Real Core/package qualification remains deferred. |

Connection configuration contains only profile URLs and credential environment
variable names, for example `{"defaultProfile":"local","profiles":{"local":{"url":"http://127.0.0.1:17883","tokenEnv":"SERVICE_LASSO_API_TOKEN"}}}`.
Use `--connections <file>` to load it. Credentials never appear in that file or
on the command line.

The current `develop` candidate already contains dashboard and lifecycle work.
This matrix does not qualify it for operator coverage, a release, deployment, or
GA. The next implementation must start from its exact head and preserve its
existing acceptance harness.
