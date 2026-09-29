# Runtime API contract

The client targets Service Lasso Core `develop` HTTP contracts. It sends
`SERVICE_LASSO_API_TOKEN` only as `x-service-lasso-admin-token`; no token is
accepted on the command line, rendered, or included in errors.

| Workflow | Core route | TUI status |
| --- | --- | --- |
| Connectivity | `GET /api/health` | Implemented |
| Service list and detail | `GET /api/services` | Implemented |
| Install/config/start/stop/restart/reload | `POST /api/services/:serviceId/:action` with `{"confirm":true}` | Implemented after an explicit `y` confirmation |
| API capability discovery | `GET /api/runtime/capabilities` | Implemented; public Core metadata route |
| Setup status | `GET /api/setup/status` | Implemented; public Core metadata route, bounded display only |
| Runtime identity | `GET /api/runtime/instance` | Implemented; regular runtime authentication, status and phase only |
| Completed declared action runs | `GET /api/services/:serviceId/actions` | Core contract available; TUI implementation pending |
| Action run | `POST /api/services/:serviceId/actions/:actionId/runs` | Core contract available; TUI implementation pending |
| Durable operation progress/cancellation | No stable TUI-ready contract | Blocked on Core |
| Permission-aware action availability | No per-service action-availability contract | Blocked on Core |
| Operator inbox | `GET /api/operator/inbox?limit=20` | Implemented; regular runtime authentication, title/severity/state/timestamp only; no detail or mutation route |
| Service health history | `GET /api/services/:serviceId/health/history` | Implemented; regular runtime authentication, transition count only |

Set `SERVICE_LASSO_API_URL` or pass `--api` to target a runtime. The default is
`http://127.0.0.1:17883`. Connection errors are displayed in the TUI and can be
retried with `r`; they are never presented as healthy runtime state. On a
non-2xx response the client retains only the method, path, and HTTP status; it
does not parse, render, or retain the server response body. Redirect responses
also fail closed and are never followed, so an operator token cannot be sent to
another origin or a downgraded transport.

Core is authoritative for authentication, permissions, confirmation, auditing,
idempotency, and lifecycle execution. The TUI sends no automatic lifecycle
retry and does not substitute direct host control when a Core capability is
missing.
