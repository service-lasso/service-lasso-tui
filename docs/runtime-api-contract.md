# Runtime API contract

The initial TUI reads Service Lasso Core's documented runtime API:

- `GET /api/health` for connectivity and API status.
- `GET /api/services` for the keyboard-navigable service list.

Set `SERVICE_LASSO_API_URL` or pass `--api` to target a runtime. The default is
`http://127.0.0.1:17883`. Connection errors are displayed in the TUI and can be
retried with `r`; they are never presented as healthy runtime state.

The first slice does not call mutation endpoints. Future keyboard actions must
model the runtime's confirmation, permission, audit, and error responses before
they can invoke lifecycle APIs.
