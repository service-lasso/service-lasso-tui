# Service Lasso TUI

Keyboard-first terminal operator interface for the Service Lasso runtime API.

## Run

```powershell
go run ./cmd/service-lasso-tui --api http://127.0.0.1:17883
```

Use `j`/`k` or the arrow keys to select a service, `Enter` for details, `r` to
refresh, and `q` to quit. The initial foundation is read-only: it makes runtime
connectivity and service state visible without issuing lifecycle mutations.

See [the runtime API contract](docs/runtime-api-contract.md) and
[the framework decision](docs/framework-decision.md).
