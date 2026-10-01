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

Issue #20 consumes Core `develop` `55848ec178b5fb05826192c8a2db576eaa8848dc`'s
durable lifecycle HTTP contract. For the selected service it first reads
`GET /api/operator/lifecycle/services/:id/availability`, then asks Core for a
non-mutating preview at `POST /api/operator/lifecycle/operations`. The preview
is the source of the target, effects, and server-issued confirmation context.
Only `install`, `config`, `start`, `stop`, and `restart` may be submitted; Core
currently reports `reload` unavailable and the TUI must never substitute the
older synchronous endpoint.

After a visible keyboard confirmation, the TUI submits exactly one frozen
request with a fresh opaque idempotency key and the Core confirmation context.
It reads `GET /api/operator/lifecycle/operations/:operationId` until Core
reports a terminal result. Refresh, reconnect, navigation, and connection
switching never replay a mutation. The TUI persists only an opaque operation
ID plus a Core-issued validated opaque actor/client/instance context needed for
later readback; it never persists credentials, confirmation phrases, previews,
request bodies, idempotency keys, URL-derived identifiers, or token hashes.
Until Core #1553's reviewed context contract is integrated, persistence fails
closed while in-process accepted and uncertain operations remain retained and
cannot be resubmitted. A context mismatch or an uncertain result is rendered
as reconciliation-required rather than submitted again.

## TUI-KEYBOARD

Arrow keys and `j`/`k` navigate; Enter opens a detail; Escape returns; `r`
refreshes; `?` shows contextual help; `/` filters locally; `n` narrows the
layout; and resize preserves the current view. Lifecycle shortcuts are visible
in the detail screen and require `y` to confirm the Core preview or Escape to
cancel. While confirmation is shown, only `y` and Escape are accepted;
refresh, navigation, profile switching, search, layout, and quit input cannot
alter the frozen context. Availability, preview, accepted, running, terminal,
unknown-after-crash, denied, and unavailable states remain distinct.

`TUI-ACCEPTANCE-001`: Direct Windows read acceptance builds the current TUI
binary and runs it through a real ConPTY against an exact pinned Core `develop`
source. The Core API starts only on a loopback ephemeral port with temporary
workspace and service roots. The probe verifies the connected dashboard, `d`
dashboard navigation, `?` help, `q` exit, and a separately launched unavailable
API state. It retains only closed-schema, metadata-only results and removes all
temporary state. It does not autostart a managed Core service, persist operator
credentials, perform a lifecycle mutation, or establish cross-platform, release,
deployment, or GA acceptance.

`TUI-ACCEPTANCE-002`: Linux and macOS lifecycle acceptance builds the TUI in a
clean native checkout and builds the exact Core source
`2633c07be25512d0a84f9bfa28de6be5edff35e8` with `npm ci` and `npm run build`.
Before every Core Node invocation it sets distinct owned workspace, instance
registry, and host-port registry paths. A real POSIX PTY drives install,
config, start, stop, and restart through the guarded Core API, confirms the
frozen preview once, and reconnects only to read a completed operation. It
retains true terminal exit classifications, exact binary VCS provenance,
Core-owned path readback, zero-operation adverse receipts and direct audit
counts, and an unchanged named unrelated-service state. It does not replace
Core with a mock, cross-compile a binary, derive persistent identity from a
profile or credential, or claim pending reconciliation while Core #1553 lacks
its reviewed adapter. Hosted runner availability remains direct native evidence
only when the complete workflow succeeds; any unavailable or failed receipt is
classified Blocked.
It labels the result `win32-amd64` only after the Windows host, Node process,
and Python/ConPTY helper report AMD64. This remains local direct-only evidence:
bounded Windows CI dependency installation changes the pinned Core checkout,
which prevents the exact clean-Core precondition from being truthfully met.

## TUI-OPERATIONS

Core's durable operation contract supplies server-authoritative accepted,
progress, terminal, and recovery readback. Cancellation is displayed and sent
to `POST /api/operator/lifecycle/operations/:operationId/cancel` only when the
readback explicitly advertises `cancellationSupported: true`; no cancellation
request is sent for an unsupported action. Update/update-cancellation remains a
Core #1538 dependency; removal and source-safe admission are separate
dependencies.

The local-admin profile remains explicit and sends its token only as
`x-service-lasso-admin-token`; an existing token is never silently converted
to a bearer token. A non-loopback durable-operation profile must explicitly
declare guarded `oauth-bearer` mode and the required read/lifecycle scopes.
The client rejects every non-loopback profile that omits that declaration,
uses `local-admin`, or lacks either required scope before reading its named
credential or creating an HTTP request. This admission applies while loading
configured profiles, resolving flag/environment overrides, selecting a
profile, and constructing a client from a literal profile. Omitted auth mode
continues to mean `local-admin` only for a loopback URL. A valid remote profile
uses HTTPS and sends only `Authorization: Bearer`; it never sends the
local-admin header. These local guards do not validate OAuth tokens or grant
authority: Core remains the server-side authority for both.
Core remains authoritative for loopback authentication, OAuth validation,
scope enforcement, permission profiles, confirmation, idempotency, execution,
auditing, and recovery.
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
Every source test and build path first requires a clean checkout, explicitly
clears `GOFLAGS`, sets `GOWORK=off`, verifies the effective module and Go
environment, and uses `-mod=readonly`. It builds with `-buildvcs=true`; the
release workflow verifies each staged binary's `vcs.revision` and
`vcs.modified=false` before archiving, then extracts every candidate archive
and repeats those checks against the candidate SHA before upload. The direct
real-Core ConPTY script applies the same admission and post-build checks before
it starts the executable. This rejects ambient overlays and workspaces: a clean
VCS stamp and self-hash alone do not prove that imported Go source was not
replaced during compilation.

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
The harness constructs a constrained child environment from an allowlist and
sets a fixed inert `SERVICE_LASSO_API_TOKEN` only for its direct invalid-URL
and unavailable ConPTY children. It must not inherit a default token, read a
real credential, place a token in an argument or receipt, or log it. This
precondition exists because connection resolution requires a non-empty
credential before URL construction; it does not relax that product policy.
All pull-request source builds and Windows constructor artifacts bind to the
pull request's exact head SHA, not the provider-created merge ref. Push builds
bind to `github.sha`. The workflow verifies its checked-out commit equals that
selected immutable source identity before building, uses it for VCS admission
and provenance, and uses it in the retained failed-artifact name. Integration
with `develop` remains a distinct branch-protection decision.
When a hosted Windows unavailable-state ConPTY probe reports `api_client_error`
for a deliberately invalid API URL, the harness must first run the newly built
held executable directly with a fresh nonce and that invalid URL. It accepts
only exit code 2 and exactly one complete `api_url_invalid` marker bound to the
same held source commit and executable SHA-256. It consumes the marker in
process and neither logs nor stores terminal text, nonce, URL, token, or raw
error data. Only after that direct constructor assertion passes may the same
still-held executable enter ConPTY. The closed result records the direct
constructor outcome and candidate identity so it proves both paths used the
same binary. A direct assertion failure is a bounded build-domain suspect; it
does not rerun, replace the failed candidate, or relabel a generic failure as
the typed API boundary.
