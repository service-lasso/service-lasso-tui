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
$env:GOFLAGS = ""
$env:GOWORK = "off"
node scripts/verify-real-core-conpty.mjs --core-root <path-to-pinned-core>
```

The probe refuses to label evidence `win32-amd64` unless the Windows host, Node
process, and Python/ConPTY helper each report AMD64. This remains a local
direct-only check: bounded Windows CI can prepare the pinned source but its
dependency installation changes that checkout, so it cannot satisfy the
probe's exact clean-Core precondition without weakening the evidence boundary.
The probe refuses ambient Go build flags and requires `GOWORK=off`; it runs the
same clean-source admission gate as CI, builds with readonly module resolution
and VCS metadata enabled, then verifies the executable's revision and clean
VCS flag before ConPTY starts it.
The result is a closed-schema, direct-read record. It is not evidence of
release asset qualification, lifecycle behavior, cross-platform support,
deployment, or GA.

## Exact Windows candidate-asset acceptance

Issue #6 also has a bounded Windows-only probe for the published prerelease
candidate `2026.9.30-97fafb0`. It downloads only that candidate's manifest and
Windows archive, verifies their pinned SHA-256 values, checks the extracted
`service-lasso-tui.exe` path, and drives the extracted executable through
ConPTY. The probe starts a disposable loopback Core read surface only after an
unavailable launch, then proves the connected dashboard, `d`, `?`, `q`, and a
50-column ConPTY resize. It does not provide a token or issue lifecycle,
authentication, staging, release, or deployment mutations.

From a Core checkout at `10e4d72b75c66977ad1dd629991a27443ffc0fd3`:

```powershell
node scripts/verify-release-asset-conpty.mjs --core-root <path-to-pinned-core>
```

The preflight clones the supplied exact Core source into a temporary isolated
checkout, runs `npm ci` and `npm run build` there, and starts that source-built
runtime only after its required `dist/server/index.js` exists. Its record is
labelled `direct-release-asset-source-built-core-conpty-read`; it is not a
packaged-Core claim. Packaged-Core mode is unavailable until a separate
installed-package contract binds package identity and digest before runtime
startup.

If that preflight fails, its closed record includes `stage:
"core-runtime-preflight"` and a safe `reason` category only. The categories
identify the failed boundary (source identity, clone, isolated checkout or
identity, dependency install, build, runtime dist, or packaged runtime) without
including command output, paths, environment values, or credentials.

On Windows, the probe resolves the candidate executable to an absolute path
before giving it to the constrained ConPTY child environment. Its native
receipt writer walks the configured temporary-base directory from a held volume
root, retaining each ancestor handle through the attempt; it does not admit a
complete mutable DOS base path in one operation. The root, parent, and
grandparent replacement checks are native fixtures. Attempt roots remain
retained evidence because owned, handle-relative removal is not implemented.

The output is a closed-schema direct release-asset read record. Linux and
macOS assets, authenticated reads, lifecycle operations, and independent
release review remain outside this evidence.

When the owned Windows terminal closes during startup, the helper records its
observed exit category. Numeric codes one and two are retained as
`terminal_exit_code_1` and `terminal_exit_code_2`, neither of which infers an
application failure location.
For the CI-only unavailable-state probe, the TUI receives a fresh private
nonce and may return a framed marker after its API-client setup or Bubble Tea
`Program.Run` error path. The helper accepts only a matching nonce and one of
the documented boundary categories, discards terminal text in memory, and
records no nonce or error text. A missing or untrusted marker remains
`unclassified`. Typed API configuration errors distinguish invalid URL,
scheme, userinfo, query or fragment, and insecure token transport without
recording the supplied URL or token.

When the Windows unavailable-state ConPTY probe is narrowed to constructor
classification, it first launches the newly built, held executable directly
with a deliberately invalid URL. It requires exit code 2 and an exact
`api_url_invalid` marker whose commit and SHA-256 match the held executable,
then carries that same held identity into ConPTY. The closed result records
only the direct outcome and candidate identity; it never records the nonce,
URL, terminal text, or raw error.

See [the runtime API contract](docs/runtime-api-contract.md), [the Core
integration and release contract](docs/core-integration-contract.md), and [the
framework decision](docs/framework-decision.md).
