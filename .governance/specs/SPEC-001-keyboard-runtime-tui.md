# SPEC-001 Keyboard runtime TUI

Status: active

## Issue20 bounded Darwin private runtime trace

TUI-ACCEPTANCE-003 permits ONE observational secondary replay only after the
original mandatory Darwin producer returns an observed nonzero status, before
its unchanged failing assertion. Keep the production workflow, primary caller,
fed bytes, selected parser, environment, options, roots, adversaries, birth/image
checks, waits, fsync and deadlines unchanged. The replay uses a distinct phase
inside that already retained failed fixture. Only its caller and env-i parser
seams add -x/-v and fixed private PS4 role/line markers; fed bytes stay identical.
Raw stdout/stderr go directly to exclusive private files before spawn and are
fsynced/read back afterward. They never enter public output or uploaded artifacts.
A closed lexical projector emits only fixed command categories, bounded source
line/depth integers, caller/producer roles, exact hashes/sizes and observed sink
dispositions. A read category means only untrusted stderr text matched a marker and command,
with unknown origin, reached-read and dispatch. Repeated PS4 prefixes match text only,
not an independently observed command-substitution process or failing parser.
Unrecognized/truncated traces remain incomplete; no raw excerpt, argv or ENV is
projected. Secondary success cannot satisfy primary acceptance. Projector
privacy/adversarial controls and actual failure-path wiring remain UNEXECUTED
until DIFFERENT ENTIRE SOURCE GO and NEW complete-input ROOT admission. Preserve
the natural c74 line124 failure and all full native/publication/Core-byte gates.

## Issue20 final042 source repair contract

TUI-ACCEPTANCE-001/002/003 and TUI-DISTRIBUTION-002 jointly require all six
final042 findings to be repaired before new execution admission:
F1: actual fresh native phase retains explicit empty GOFLAGS and GOWORK=off
and admits effective Go environment/module immediately before native build.
F2: register a fully initialized terminal immediately after fork, before birth
observation; unknown birth remains owned/unproven until actual child wait.
F3: actual Windows reconnect caller supplies the pinned source SHA and digest
of the extracted executable; the Python held identity spans direct constructor
and ConPTY without reacquisition. F4: authentic reconnect success requires the
closed candidateIdentity and matching directConstructor, bound to caller input;
identity-free, foreign and unexpected fields deny success. F5: native writer
accepts the closed direct-constructor stage, and Node attempts independent exit
persistence even when helper persistence fails, retaining primary and both sink
failures. F6: q success in both Windows read helpers requires actual wait with
observed zero; nonzero/signaled/unknown are closed failure categories.
Actual phase/post-fork/entrypoint/producer-consumer/native-writer/q-outcome
regressions are required. All remain UNEXECUTED pending DIFFERENT fresh ENTIRE
SOURCE GO and NEW complete-input ROOT admission. Preserve all earlier 5+2+3
repairs and Issue18 SPEC002 publisher controls; no native/operator/Core1553,
published-byte, release or full-delivery acceptance is implied.

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
The executable bytes used by the PTY must be a platform-enforced immutable
object, not merely a readable descriptor. On Linux, the harness copies the
verified descriptor bytes into a `memfd`, verifies the copy digest, applies
the write, grow, shrink, and seal seals, verifies those seals, and executes
only that sealed object with `fexecve`. On macOS, it copies the verified bytes
to a leaf in an owned staging directory and requires the OS-reported
`SF_IMMUTABLE` system flag on both the leaf and its parent before execution.
The Darwin leaf is flagged and read back first; the harness retains a writable
leaf descriptor from before activation and requires its post-activation
in-place write to be denied, then hashes that immutable leaf against the held
candidate digest before flagging and reading back the parent.
The PTY child retains the staging-directory descriptor, changes directory
through that held descriptor, verifies the leaf identity relative to it, and
uses Darwin's pathname `execve` on that relative leaf; it does not execute
`/dev/fd/N`. Activation and later removal require a non-interactive privileged
helper. User immutable flags, chmod, ACLs, descriptor retention, and a
same-user promise are insufficient. The metadata-only primary receipt records
only the mechanism, flag readback, denied in-place-write probe, and existing
digest identity before teardown. If leaf activation succeeds but parent
activation or readback fails, its held cleanup descriptor is carried into the
primary-before-cleanup finalization; any unproven rollback retains recovery
material and records only that closed result. A separate closed cleanup receipt
records its outcome and whether recovery material remains; it cannot replace
the primary child outcome. A bounded `q` observation never kills an owned
child: an unresolved live child retains its PTY and execution object for
recovery, while an observed negative wait status is `terminal_signaled`.

`TUI-ACCEPTANCE-003`: The retained native public artifact is a closed metadata
receipt. Before upload, the workflow validates every selected file against its
exact schema and rejects host paths, process identifiers, birth data, private
custody labels, images, tool records, and literal command data. The binary
receipt records only its digest and byte size; it never uses a pathname-bearing
checksum format. Each phase directory contains exactly one complete five-record
set; foreign public records and partial or duplicate sibling sets fail closed.
Within that set, the input-custody, build-output, and public-projection TUI
commits must agree; input-custody and Core binding commits must agree; and the
binary digest and byte size must agree with build-output and public-projection.
All GitHub Actions used by CI and candidate publication are
committed SHA references, with their human version labels retained only in
comments.
Issue #20 native input custody uses a quoted shell heredoc for the fresh
environment boundary so literal next commands retain dollar expressions and
semicolons without outer-shell execution. The actual tool producer emits tab
separated name/path/SHA256/size records; the Python consumer splits real tabs.
TUI and Core Git commits and trees are exactly 40 lowercase hexadecimal Git
object identities, independently of the 64 hexadecimal SHA256 digest contract
for dirty state, inventory and executable bytes. Actual producer/consumer and
real Git tree regressions are required. This source repair addresses all three
inherited pipeline findings together; execution remains prohibited until fresh
entire SOURCE GO and a new complete-input ROOT admission. It grants no native
five-action or published-byte acceptance and preserves Issue #18 publication.
The same producer must terminate ancestor traversal at `/` using real parent
paths and check every ancestor including root for links; stripping a final
slash component to an empty string is not a valid root traversal.
The host birth selection uses an ordinary shell case with per-platform command
substitutions, preserving Linux awk and Darwin ps identities under the native
macOS Bash parser. Natural d1beda9 macOS guard failure is retained separately;
it does not justify any local execution or a native acceptance claim.
Each native run also mutates the
source inode in place after the immutable execution object exists and proves
the launch still reaches the verified object; it restores the source byte
before proceeding. If a platform cannot establish and read back that mechanism,
the lifecycle result is Blocked.
When bounded terminal observation expires, the helper must not exit while it
still owns the PTY, immutable execution object, or Core/JWKS parent lifecycle.
It writes only a closed unresolved receipt and transfers no descriptor-based
claim across a process boundary. A live external recovery parent retains those
objects and the Core/JWKS parent until it directly observes the child exit,
then writes the final child and cleanup receipts. `waitpid` ownership loss
(`ECHILD`/`ChildProcessError`) is an unowned, reaped, unresolved outcome; it
is never a successful terminal exit and must not overwrite the primary
receipt. The native harness test guard and native workflow both exercise an
adverse live-child recovery and an externally reaped child. The volatile
 JavaScript acceptance controller must not itself host Core or JWKS. A distinct
 external POSIX resource owner starts the actual Core/JWKS runtime as its child
 and retains that process identity together with the PTY and immutable launch
 object. The native Linux and macOS workflows kill that actual controller only
 after a real TUI child is live, read back live Core and JWKS dependencies from
 the owner, observe normal `q` terminal exit, and only then permit owner-driven
 dependency cleanup. A source-only or Linux-only toy process exercise cannot
 substitute for this production-path proof.
Before its first Core fetch, dependency install, build, or import, each native
phase fsyncs a fresh input-custody record that binds the requested Core commit,
clean TUI source identity, all three distinct owned Core paths, their non-link
parent chains, initially absent registries, process identity, actual invoked
tool identities, and literal next commands. Host paths, process identifiers,
birth values, executable locations, and tool locations are owner-private
custody only. The uploaded public receipt has a separate closed schema with
only public source identities, ownership predicates, and verified-tool names;
it never carries owner-private fields. Core source binding is independently
fsync-persisted after checkout and before `npm ci`, while binary output is a
separate post-build receipt. Raw PTY chunks remain in process memory solely for
bounded interaction assertions and are never written or uploaded. Existing
private failure evidence remains preserved under its existing authority; this
workflow creates no new private artifact channel or permission claim.
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
root, parent, and grandparent is refused during acquisition. Node asks that
writer to persist the helper result and independently persist its bounded
helper-exit state in the separate Node sink before cleanup. Both attempts are
required even if the helper sink rejects. Every
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

Issue #20 final9f entire-review findings require two coherent source repairs:
record OS-observed Bash producer and Python writer PID/PPID/birth/image tuples,
with the live parent directly observing and binding its actual writer child;
and retain mandatory external-owner finalization outcome for the durable
observer to combine with its actual held Core child shutdown/wait/absence.
Missing, malformed or crashed ownership receipts fail closed. Actual producer
and controller-to-observer-to-external-owner regressions are required.
All prior five producer repairs and Issue #18 nine-mutation/six-held-asset
publisher safeguards remain required. Source and regressions stay UNEXECUTED
until fresh ENTIRE independent SOURCE GO and NEW complete-input ROOT admission.
Issue #20 final87ad entire SOURCE NO-GO requires all three coherent repairs:
explicit source inode tuple and candidate digest inputs to the actual mutation
probe with restored-byte regression; observer ownership from runtime spawn
through every acquisition/owner launch/birth/persistence/handoff failure, actual
child waits and birth-absence proof before closed failure projection; private
FIFO peer-aware rendezvous with truthful parent-owned writer waits on death
before ready and shell failure before observation. Retain all earlier producer,
finalization, immutable execution and Issue #18 publisher protections. New
regressions remain UNEXECUTED until fresh ENTIRE SOURCE GO and NEW ROOT admission.

## PR25 final9d five-finding source successor (issues18 and20)
Issue18 / SPEC002 AC5 and AC6: retain the first admitted manifest bytes and
require exact equality with the acquired held manifest before ANY provider
access. Regress coherent manifest plus inventory replacement at acquisition.
Issue20 / SPEC001 TUI-ACCEPTANCE-001/002/003 and TUI-DISTRIBUTION-002:
F2 retains the actual direct-owned runtime immediately after spawn, before
birth/select/read/parse, and closes/waits with unknown birth remaining unknown.
F3 preserves primary reconnect spawn failure plus independent Node/native sink
rejection without inventing a helper receipt. F4 identity-guards restoration
and removal of only the positively bound redundant original hardlink alias;
repeat and interrupted rename/symlink/mutation regressions remain required.
F5 targets the actual observer owner command for launch/handoff faults and
records the reached boundary; Darwin /bin/ps remains outside those faults.
All five form one SOURCE-ONLY bundle. Prior6+5+2+3 assertions, provider policy,
private held-byte gates, Core authority, TLS and secrecy stay mandatory.
All new regressions are UNEXECUTED until DIFFERENT fresh ENTIRE SOURCE GO plus
NEW complete-input ROOT admission. No native or full-delivery claim is made.
## Issue20 PR25 current1102 two repairs and parser evidence preparation

SPEC001 TUI-ACCEPTANCE-002/003 and TUI-DISTRIBUTION-002 require identity-bound
cleanup of an actual staged reparse link after failed rename, with actual
post-symlink failure, repeat restoration and foreign-substitution regressions;
and bounded cleanup of the real build's positively owned private HOME module
cache without changing persisted GOFLAGS or foreign GOWORK assertions.
The mandatory actual Darwin producer failure remains UNRESOLVED. Prepare
private caller/parser path/version/hash, exact source bytes, argv and environment
binding before parsing through the actual producer test seam. Preserve primary
syntax diagnostics, real tuple/tool/Git/FIFO assertions and mandatory Darwin
execution. This is diagnostic preparation, not a causal source repair or pass.
Entire scope remains SOURCE-ONLY and regressions UNEXECUTED until DIFFERENT
fresh ENTIRE SOURCE GO and NEW complete-input ROOT admission. Preserve all
prior cumulative protections, Issue18 SPEC002 AC1..7 and every full-delivery gate.
## Issue20 PR25 exact2e5a two-finding successor
SPEC001 TUI-ACCEPTANCE-002/003 and TUI-DISTRIBUTION-002 require a closed
safe BEFORE-invocation parser/caller/workflow/full-producer/fed-source tuple
and AFTER-invocation stage/result/diagnostic-sink disposition through existing
test output. Full environment, argv, paths and raw version/results stay private;
no upload authority is created. Select the existing parser at both seams and
preserve fed bytes. Darwin line99 tools cause remains UNRESOLVED and the actual
producer is mandatory. Returned natural Darwin cleanup failure must reach the
mandatory external-owner finalization and observer aggregate gate, preserving
primary success and retained recovery. Actual natural-return/main/observer gate
regression is required alongside the existing exception vector. Source-only,
UNEXECUTED until DIFFERENT fresh ENTIRE review and NEW complete-input ROOT
admission; all earlier repairs, Core authority and Issue18 protections remain.
## Issue20 PR25 exact67b5 bounded parser differential preparation
SPEC001 TUI-ACCEPTANCE-002/003 and TUI-DISTRIBUTION-002 permit a source-only
paired parse diagnostic after the unchanged mandatory actual producer. Bind the
same selected parser image, exact fed bytes, fresh environment and existing
-euo pipefail options; add only -n, comparing stdin with -c input routes.
Hypothesis: the fed bytes are rejected independently of parser input route.
Unequal results falsify route independence; paired zero results falsify intrinsic
parse rejection only. Neither proves expansion/runtime success, original caller
equivalence or a Darwin cause. Keep private argv/environment/raw output fsynced
before each route and emit only closed outcomes/hashes. Original invocation and
assertions remain primary. All prior repairs and full delivery gates remain;
DIFFERENT entire source review and NEW complete-input ROOT admission precede
execution. No diagnostic has run locally and no source causal repair is claimed.
## Issue20 PR25 da17 mandatory cleanup receipt persistence successor
SPEC001 TUI-ACCEPTANCE-002/003 requires mandatory cleanup receipt durability
independently of the primary outcome and actual OS cleanup result. A natural
open/write/flush/fsync OSError at the production cleanup writer must propagate
as cleanup_receipt_persistence_failed through the real owner finalization and
observer denial. Preserve the successful primary, true cleanup disposition and
retained Darwin recovery; receipt failure must not relabel OS cleanup failed.
Actual main/finalize/production writer and observer regressions cover successful
cleanup and natural retained Darwin cleanup failure with later receipts live.
Source-only; all regressions UNEXECUTED pending DIFFERENT ENTIRE source review
and NEW complete-input ROOT admission. F3 actual Darwin producer is unresolved;
paired parse-only zero results identify no runtime cause. Prior F1/F2/P2 and all
5+6+5+2+3, Issue18 AC1..7/Core authority/native/full-delivery gates remain.
## Issue20 PR25 F3 accepted explicit-FIFO descriptor source proposal
SPEC001 TUI-ACCEPTANCE-003 retains the mandatory actual native producer and
its selected Bash/parser/quoted heredoc/options. The parent accepted replacing
read -r -t 1 writer_ready <&9 with read -r -t 1 -u 9 writer_ready so peer-ready
FIFO consumption does not redirect script-input fd0. GNU/Apple published text
supports descriptor isolation, not an exact source-to-Darwin-image binding or
causal defect. Seekback/undo counterevidence is retained. Preserve status/variable,
actual jobs-alive policy/$! identity/reciprocal FIFO birth-image-PPID checks,
failed-frame/EOF/overflow denial/trap actual wait/private fsync/success wait.
Existing actual producer plus writer-death/producer-failure adversaries remain
primary and bind the new operation. Original da17 runtime failure remains FAILED;
paired -n stdin/-c zero results do not prove runtime success or causal repair.
Proposal remains UNQUALIFIED until exact new candidate natural mandatory Darwin
producer succeeds. DIFFERENT ENTIRE review and NEW ROOT admission precede local
execution; all R1 and prior native/publisher/full-delivery requirements remain.

## PR27 F1/F2/F3 coherent source successor
TUI-ACCEPTANCE-003: combined stderr is untrusted lexical input. Public fields
name marker matches and command-text categories only; origin, reached read,
dispatch and child parser identity remain unknown even for exact forged PS4,
multiline argv/ENV, verbose source or command-stderr frames. No authentication
or private-artifact grant is inferred.

Secondary raw descriptor files remain private and retained in full on failure.
Readback uses the original held read/write descriptor, fsync and regular-file
fstat before allocation. Fixed source quotas: 1,048,576 bytes per stream,
8,192 bytes per physical line, 32,768 physical lines. Oversize skips readback
before allocation; no full hash or complete projection is claimed. Admitted
readback allocates at most 1,048,577 bytes, reads positionally in at most 65,536
byte requests plus a one-byte EOF probe, then repeats held fstat. Changed size,
identity, metadata, short read, extra bytes or IO failure close incomplete or
unavailable with no hash. The full hash describes only the bounded observed
held bytes with EOF and unchanged fstat; it is not an immutable seal.
Projection scans bounded Buffer offsets, decodes at most one 8,192-byte line
(no whole UTF8 conversion/split), and rejects exceeded byte/line/count quotas
with incomplete output and no partial match set. Counts are at most 32,768;
source-line numbers 1..999999 and prefix depths 1..64, fixed enums, two roles.
Maximum simultaneous readback input is one quota plus one byte; decoded line
is at most 16,384 UTF16 bytes (with bounded regex captures), four fixed records.
Raw writing has no invented capture cap: disk/descriptor persistence failure
remains honestly failed; readback quota does not truncate or delete raw files.
All secondary failures and closure preserve the unchanged primary result,
mandatory assertion, fed 8331 bytes/hash, private roots and waits/fsync.

Issue18 SPEC002 AC2/AC3/AC7: normalize CRLF to LF inside the source contract
before every selector/split/regex; both line endings exercise EVERY existing
scoped credential, read permission, publisher-only and no-fallback assertion
and all negative mutations. No workflow permission or credential change.

Author meaningful forged-frame, held-private IO/oversize/line/count/growth/error
and LF/CRLF regressions but do not execute them before DIFFERENT ENTIRE SOURCE
GO and NEW complete-input ROOT admission. c585 natural Darwin and Windows
failures stay preserved; native cause, protected scoped credential, publication
and same-published-byte Core/full delivery remain incomplete.
## Issue #28 source implementation ready for entire review
SPEC-004 TUI28-CANDIDATE/NATIVE/AGGREGATE/PUBLISH/DENIALS/CLOSURE is implemented coherently in the separate scoped workflow, native producer/owner/observer, current-attempt aggregate, archive/public byte verifiers and protected publisher. Existing v2 workflow and publisher source stay unchanged. The canonical policy copy is byte-identical. All new local product/compiler/parser/test/native execution remains UNEXECUTED; source-only review and NEW complete-input ROOT are mandatory. Preserve natural pre-code CI37150082745: macOS failure and native jobs skipped, never scoped acceptance. Persistent/pending reconciliation against2633 remains blocked despite source support for the reviewed future endpoint. Full programme ACTIVE; no protected catalog entry, real publication, Core qualification or GA is claimed.
