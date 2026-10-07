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

## Issue1 complete client contract unit A (8 October 2026)
Active Development source unit, based on TUI effd6a69 and authentic current Core
ce56f592 / pinned Core2633. Issue1 remains OPEN; issue20 is the five-action dependency.
TUI-CLIENT-A1 (F1/F2): require versioned accepted/read/cancel envelopes, top-level
operation and cancellation result/terminal, own actor, exact operation/action,
closed status/outcome consistency, and bounded unique target IDs. Core initially
claims the requested singleton then replaces target IDs with authoritative
preflight/progress targets; admit the requested singleton or subsets of the
frozen preview containing the requested service, never unrelated targets.
Retain that frozen preview in memory only. Restoration has no persisted preview:
first authoritative own read establishes the immutable action/target tuple and
later reads bind it; never infer request identity from a list or time.
TUI-CLIENT-A2 (F3): typed closed stage errors; never render raw transport, decode,
filesystem, config or startup errors. HTTP diagnostic keeps only method, route
class and canonical status. Response ceiling 1 MiB, depth16, collection1024,
string4096 bytes, duplicate keys/trailing JSON/invalid UTF8 denied before typed
decode. Service list<=512, inbox<=20, history<=256, targets<=100.
TUI-CLIENT-A3 (F5/F6/F10): availability v1 requires exact selected service,
all five supported actions plus unavailable reload, complete typed authority,
unique known actions. Current update_check/update_download pair is admitted
only as unsupported metadata, never displayed as supported or submitted.
Safe reason projection uses fixed labels; unknown reason is unavailable without
rendering its value. Selected history/availability bind epoch, service, request
generation, reset/loading state. Refresh results bind generation, snapshot age
is visible, post-terminal refresh reads authoritative service state once.
Recheck availability at confirmation submission without replacing the preview.
TUI-CLIENT-A4 (F7): lost submit response before an ID is explicit uncertain,
no-ID state: refresh cannot read an empty ID or replay any mutation. No key,
request or confirmation persistence; server correlation/recovery remains Core
work. Known-ID reads and cancellation retain the original client and validate
server context when available. Context changes fail closed.
All actual-wire/model/storage/startup regression SOURCE remains LOCAL UNRUN
until DIFFERENT ENTIRE cumulative SOURCE GO and NEW complete-input ROOT plus
parent admission. F4 keyboard viewport/focus/Unicode, F8 full forms/workflows,
F9 shared native fixture, direct Windows/Linux five actions/cancel/recovery,
protected immutable four-file/two-archive publication and same-byte Core/CLI/
template integration remain subsequent required scope. Mac Deferred/N A never
PASS; B0/B2 UNKNOWN and full Node plus finite measured-budget human choice remain.