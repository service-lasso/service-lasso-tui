# Service Lasso TUI

Keyboard-first terminal operator interface for the Service Lasso runtime API.

## Run

```powershell
$env:SERVICE_LASSO_API_TOKEN = '<local operator token>'
go run ./cmd/service-lasso-tui --api http://127.0.0.1:17883
```

Use `j`/`k` or the arrow keys to select a service, `Enter` for details, `r` to
refresh, and `q` to quit. From a service detail, `i`, `c`, `s`, `x`, `R`, and
`l` request install, config, start, stop, restart, and reload respectively.
Every lifecycle request needs a visible `y` confirmation and is sent only once;
Core performs authorization, confirmation enforcement, auditing, and execution.

`SERVICE_LASSO_API_TOKEN` is sent only as Core's
`x-service-lasso-admin-token` request header and is never rendered or logged.
Use a process environment or an operator-managed secret launcher; do not put a
token in a command-line argument.

The executable is an attached terminal operator tool. It is not a Core managed
service and must not be autostarted by Core.

See [the runtime API contract](docs/runtime-api-contract.md), [the Core
integration and release contract](docs/core-integration-contract.md), and [the
framework decision](docs/framework-decision.md).
