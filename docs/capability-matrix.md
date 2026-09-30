# TUI capability matrix

## TUI-CONNECTION

| Requirement | Current contract | Status |
| --- | --- | --- |
| `TUI-CONNECTION-001` | One runtime URL from `SERVICE_LASSO_API_URL` or `--api` | Missing named profile selection and switch semantics. |
| `TUI-CONNECTION-002` | Core development candidate reads `SERVICE_LASSO_API_TOKEN` and sends `x-service-lasso-admin-token` | The documented Core candidate does not establish an interchangeable bearer scheme. |
| `TUI-CONNECTION-003` | Token-bearing non-loopback HTTP is rejected; redirects are not followed | Direct TLS verification proof is still required. |
| `TUI-CONNECTION-004` | Non-2xx bodies are not retained | Typed authentication and permission presentation needs contract-specific tests. |
| `TUI-CONNECTION-005` | Refresh retains a stale service snapshot | Connection switching and old-result rejection are not implemented. |
| `TUI-CONNECTION-006` | Core #1462 registration and Core #1465 lifecycle are separate dependencies | Real Core/package qualification remains deferred. |

The current `develop` candidate already contains dashboard and lifecycle work.
This matrix does not qualify it for operator coverage, a release, deployment, or
GA. The next implementation must start from its exact head and preserve its
existing acceptance harness.
