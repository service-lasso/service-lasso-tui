# TUI capability matrix

## TUI-CONNECTION

| Requirement | Current contract | Status |
| --- | --- | --- |
| `TUI-CONNECTION-001` | Named metadata-only profiles are selected with `--profile`, `SERVICE_LASSO_API_PROFILE`, or the configured default; `p` opens the keyboard selector. | Implemented; flags override environment, which overrides profile metadata. |
| `TUI-CONNECTION-002` | A `local-admin` profile declares `tokenEnv`; its environment value is sent only as `x-service-lasso-admin-token`. | Implemented; legacy profiles default to `local-admin` and are never silently converted to bearer auth. |
| `TUI-CONNECTION-003` | Token-bearing non-loopback HTTP is rejected; redirects are not followed and standard TLS verification remains enabled. | Implemented and locally tested. |
| `TUI-CONNECTION-004` | Non-2xx bodies are not retained; authentication and permission denials render only method, path, and HTTP status. | Implemented and locally tested. |
| `TUI-CONNECTION-005` | Reconnect retains a stale snapshot only for its current connection; switching clears it and discards old asynchronous results. | Implemented and locally tested. |
| `TUI-CONNECTION-006` | A non-loopback durable-operation connection explicitly declares `oauth-bearer` and `service-lasso:read` plus `service-lasso:lifecycle:write`. | Implemented client admission; Core remains authoritative for token and scope validation. |
| `TUI-LIFECYCLE-020` | Availability → Core preview → frozen one-shot durable submit for install/config/start/stop/restart. | Implemented against Core `55848ec`; reload is deliberately unavailable and the retired synchronous route is absent. |
| `TUI-OPERATIONS-020` | Actor/connection-bound operation readback with terminal and unknown-after-crash display; only advertised cancellation is offered. | In-memory readback is implemented. Persistent reconciliation is fail-closed pending Core #1553's reviewed server-issued opaque actor/client/instance context contract. |

Connection configuration contains only profile URLs and credential environment
variable names, for example `{"defaultProfile":"local","profiles":{"local":{"url":"http://127.0.0.1:17883","tokenEnv":"SERVICE_LASSO_API_TOKEN"}}}`.
Use `--connections <file>` to load it. Credentials never appear in that file or
on the command line.

The current `develop` candidate already contains dashboard and lifecycle work.
This matrix does not qualify it for operator coverage, a release, deployment, or
GA. The next implementation must start from its exact head and preserve its
existing acceptance harness.
