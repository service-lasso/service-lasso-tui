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

## Direct Windows Core read acceptance

Issue #13 provides a Windows-only direct read acceptance probe for the current
source binary. It pins Core `develop` to
`d9e2ae799244317940c862fe1261dfd22b7bdda1`, builds the TUI in a disposable
directory, starts that Core source on a loopback port chosen by the OS, and
drives the binary through ConPTY. It checks the connected dashboard, `d`, `?`,
`q`, and a separate unavailable API launch. It creates no managed service,
permanent runtime state, or credentials.

From a clean worktree at that Core revision, install the pinned ConPTY helper
and its Core dependencies, then run:

```powershell
python -m pip install --require-hashes --only-binary=:all: --no-deps -r scripts/requirements-conpty.txt
npm ci --prefix <path-to-pinned-core>
node scripts/verify-real-core-conpty.mjs --core-root <path-to-pinned-core>
```

The probe refuses to label evidence `win32-amd64` unless the Windows host, Node
process, and Python/ConPTY helper each report AMD64. This remains a local
direct-only check: bounded Windows CI can prepare the pinned source but its
dependency installation changes that checkout, so it cannot satisfy the
probe's exact clean-Core precondition without weakening the evidence boundary.
The result is a closed-schema, direct-read record. It is not evidence of
release asset qualification, lifecycle behavior, cross-platform support,
deployment, or GA.

See [the runtime API contract](docs/runtime-api-contract.md), [the Core
integration and release contract](docs/core-integration-contract.md), and [the
framework decision](docs/framework-decision.md).
