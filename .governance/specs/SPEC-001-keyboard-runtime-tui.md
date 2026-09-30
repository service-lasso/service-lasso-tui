# SPEC-001 Keyboard runtime TUI

Status: active

## TUI-CONNECTION

The client validates an HTTP(S) runtime URL, retrieves `/api/health`, and clearly
shows connected, unavailable, and retry states without replaying mutations.
It never follows redirects: a redirect response is a failed runtime request, so
the operator token cannot reach another origin or a downgraded transport.
For every non-2xx response, the error retains only the request method, path,
and HTTP status. It never parses, retains, or renders the runtime response
body.

## TUI-DASHBOARD

The client retrieves `/api/services`, `/api/runtime/capabilities`, and
`/api/setup/status`, renders a keyboard-navigable dashboard, service list, and
detail view, and presents safe lifecycle and health summaries. It may show the
status and phase from authenticated `GET /api/runtime/instance`, but never
renders runtime paths, host identity, source identity, or generation IDs.

## TUI-LIFECYCLE

For Core APIs that are documented and tested, the client invokes
`POST /api/services/:serviceId/{install|config|start|stop|restart|reload}`.
It presents an explicit local confirmation before a request and renders the
runtime's returned action result. A rejected request must create no retry.

## TUI-KEYBOARD

Arrow keys and `j`/`k` navigate; Enter opens a detail; Escape returns; `r`
refreshes; `?` shows contextual help; `/` filters locally; `n` narrows the
layout; and resize preserves the current view. Lifecycle shortcuts are visible
in the detail screen and require `y` to confirm or Escape to cancel.

`TUI-ACCEPTANCE-001`: Direct Windows read acceptance builds the current TUI
binary and runs it through a real ConPTY against an exact pinned Core `develop`
source. The Core API starts only on a loopback ephemeral port with temporary
workspace and service roots. The probe verifies the connected dashboard, `d`
dashboard navigation, `?` help, `q` exit, and a separately launched unavailable
API state. It retains only closed-schema, metadata-only results and removes all
temporary state. It does not autostart a managed Core service, persist operator
credentials, perform a lifecycle mutation, or establish cross-platform, release,
deployment, or GA acceptance.
It labels the result `win32-amd64` only after the Windows host, Node process,
and Python/ConPTY helper report AMD64. This remains local direct-only evidence:
bounded Windows CI dependency installation changes the pinned Core checkout,
which prevents the exact clean-Core precondition from being truthfully met.

## TUI-OPERATIONS

The Core action-run API is available but exposes server-side completed action
runs rather than a stable asynchronous operation polling contract. The client
may show its returned result for lifecycle work but cannot claim long-running
operation progress or cancellation until Core publishes suitable contracts.
The dashboard may read authenticated `GET /api/operator/inbox` and
`GET /api/services/:serviceId/health/history`; it displays a bounded inbox
summary (title, severity, state, timestamp) and health transition count only.
It does not fetch inbox details or invoke any inbox mutation route.

## TUI-DISTRIBUTION

Release packaging emits an attached-terminal executable for Windows, Linux, and
macOS. It is distributed beside a Core archive or as a separate release asset;
it is never declared as a managed Core service or an autostarted daemon.

`TUI-DISTRIBUTION-001`: A Core-packaging candidate is created only by an
explicit workflow dispatch on `develop`, with a caller-supplied exact version
whose source SHA suffix matches the dispatched `develop` commit. It creates
`win32-amd64`, `linux-amd64`, `darwin-amd64`, and `darwin-arm64` archives plus
an exact `SHA256SUMS.txt` and versioned candidate manifest. The workflow must
test source, build all targets, verify every checksum, and structurally smoke
the extracted executable path before uploading the retained candidate
artifact. It never runs automatically and never creates a deployment or GA
claim.
After its source and native Windows, Linux, and macOS smoke jobs pass, the
manual dispatch creates a prerelease candidate tag and release for those exact
assets. It is not the latest release and does not establish GA, deployment, or
publication acceptance. The hosted macOS runner directly smokes only its host
architecture; the other macOS archive is cross-built and structurally checked
as surrogate evidence until native hardware acceptance is available.
Core #1461 may consume a candidate only after independent release review pins
the candidate version, full source SHA, manifest, assets, and digests; a
mutable release selector or incomplete asset set is rejected.

`TUI-DISTRIBUTION-002`: Windows candidate-asset ConPTY acceptance must prepare
an isolated Core source checkout at the pinned commit with `npm ci` and
`npm run build` before starting the source runtime, and it must fail closed if
the required runtime dist is absent. Packaged-Core acceptance is unavailable
until an installed-package contract verifies and binds package identity and
digest before runtime startup. Source construction never establishes
packaged-Core qualification.
Before candidate acquisition, a failed Core preflight emits only a closed reason
category (`source_identity_*`, including dirty or attached supplied-source
identity, `source_clone_failed`, `isolated_checkout_failed`,
`isolated_identity_*`, `dependency_install_failed`, `source_build_failed`,
`runtime_dist_unavailable`, or `packaged_runtime_invalid`). It never includes
command output, paths, environment values, or credentials in that record.
When the reconnect helper has started, it atomically records only `stage`,
`outcome`, and `closedReason` in its attempt-owned temporary root. A startup
terminal-close receipt may additionally contain `startupBoundary`, from the
closed set `unclassified`, `api_url_invalid`, `api_url_scheme`,
`api_url_userinfo`, `api_url_query_or_fragment`, `api_token_transport`,
`api_client_error`, `program_run_error`,
`program_run_killed`, `program_run_panic`, and `program_run_interrupted`; no
other receipt may contain that field. EOF is not an exit result: while its owned
PTY remains live the helper retains its existing bounded wait, and after exit it
records only one of `terminal_exited_zero`, `terminal_exit_code_1`,
`terminal_exit_code_2`, `terminal_exited_nonzero`, `terminal_signaled`, or
`terminal_unknown`. The two numeric categories mean only that the owned PTY
reported that exit code; neither implies that the TUI reached `main` or
`Program.Run`. The helper accepts a non-`unclassified` startup boundary only
from exactly one complete framed marker emitted by the checked TUI. Its nonce
must match the fresh attempt nonce, and its source commit and binary SHA-256
must match the source and held executable identities observed by the helper
before spawn. Malformed, partial, duplicate, foreign, or otherwise extra frames
produce `unclassified`. The helper opens the executable through an owned
Windows handle that permits read sharing only, hashes those held bytes, and
keeps the handle open through child spawn and probe completion so replacement
or deletion cannot change the spawned candidate. The marker is processed in
memory and its nonce, terminal text, paths, errors, and environment values are
never emitted or stored. Closed receipts may record only the source commit and
binary SHA-256 as `candidateIdentity`; they never record a path, nonce,
terminal text, URL, token, or error data. The TUI obtains the source commit
from its own clean Go build information (`vcs.revision` and
`vcs.modified=false`) and the binary SHA-256 by reading its own executable; it
does not accept a caller-supplied source or binary identity for this marker.
The `api_*` values are emitted only after the application receives a typed
configuration error from its API client; `api_client_error` is the fallback for
an unclassified API-client return. The `program_run_*` values are emitted only
after the application receives a non-nil result from Bubble Tea `Program.Run`;
typed Bubble Tea sentinel errors select the killed, panic, or interrupted
values, and any other returned error selects `program_run_error`. On Windows,
a native receipt-writer reaches the requested temporary base from a held volume
root one directory component at a time, keeps that base and every ancestor
handle live, and creates that root and both receipt files relative to held
directory handles. It rejects reparse points during construction, applies an
owner-only DACL, and keeps the owner non-delete- and non-write-shareable while
the receipt is live. Native fixtures must prove replacement of the attempt
root, parent, and grandparent is refused during acquisition. Node asks that writer to replace
the helper record with its bounded helper-exit state before cleanup. Every
receipt enum and response schema is closed; a receipt sink failure preserves
the original probe failure. Until a handle-relative owned deletion operation is
implemented, Windows native attempt roots are retained as evidence rather than
closed and recursively removed by pathname.
