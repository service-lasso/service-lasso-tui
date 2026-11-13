# TUI capability matrix

Current issue1 source unit: fix/1-complete-client-contract, PR33 targeting develop. The default command now selects operator/v2; --compat-v1 explicitly selects the historical adapter. The current dirty source is incomplete and UNRUN. No current compile, source approval, native acceptance, protected publication or GA claim is made. Earlier “implemented/locally tested” wording below describes historical compatibility work only; it is not evidence for this current head.

The normative current contract is .governance/specs/operator-v2-selected/BLUEPRINT.md, CODECS-AND-STATE.md and REQUIREMENT-OWNER-MATRIX.md, with the exact adopted catalog, lifecycle availability, family revision, confirmation phrase, error/HTTP/retry, action union/topology/scalar and entry replacement amendments. All46 requirements remain required. Private storage outcome adoption supplies truthful source observations only; original failed-save A/legacy2 preservation and native custody guarantees remain unqualified.

| Current journey | Authored source behavior | Remaining acceptance |
| --- | --- | --- |
| Overview/detail | Closed service/read DTOs, actor-bound pages, selected service epochs, safe provider text and availability states. | Genuine current Core issuer and whole-source/native validation; no stale response may retarget a draft. |
| Graph | Explicit dependency-to-dependent nodes/edges/groups, split completeness, snapshot/expiry/cursor ownership and no empty continuation. | Actual backend privacy/incomplete graph owner and native rendering/failure cases. |
| Variables/endpoints | Typed scopes/transports and bounded pages; logical data is escaped at the terminal boundary. | Full current issuer and actual bounded paging/expiry/privacy evidence. |
| Lifecycle | Service-scoped availability plus eight family bindings; original immutable preview, immediate current availability and final fresh trusted context before one POST. | Genuine durable registrations, correlated outcomes and five direct compiled native actions; reload remains unavailable. |
| Config | Typed scalar/structured fields; useful files/templates/roots/logical handles; issued catalogs, insertion slots and separate own-entry replacement rosters; exact revision, batch retention and backup policy. | Actual Core issue/materialize/all-writer lock ownership; full positive/failure/editor/native validation. |
| Setup/declared actions | Declaration-bound input and family revision; full flat schema table preserving recursive union/object/array/enum semantics, original Unicode payload scalars and finite draft budgets. | Genuine backend payload-reference issuer/held authority, actual durable action owner and full integration validation. Policy booleans do not issue reference authority. |
| Updates/removal | Explicit check/download/install controls with exact family tokens; removal retains data and never invents dependent/member authority. | Actual candidate/update/removal owner, every-member absence, allocation generation CAS and retained-data native custody. |
| Health/history/inbox/queue/audit/logs | Closed kind/status tuples and typed paged data; stable snapshot/source/expiry/cursor/row identity; explicit stale/error states; safe control rendering. Inbox/queue actions retain source revision and exact context. | Genuine current backend owner, permission and whole paging/mutation/native cases. |
| Metadata | Dedicated GET/CAS favorite/layout; omission preserves the other field, explicit null layout reset; fresh read after publication. | Actual metadata revision/all-writer owner, restart and conflict evidence. |
| Operation restoration | Exact saved2 four keys/original canonical-v1 digest, readonly own-ID readback and authoritative root/action before proposed3; exact old bytes/context guards; no lost-ID guessing/replay. | Genuine two-platform atomic publication, all cleanup/identity failures, durable interprocess/restart exclusion and original failed-save predicates. Architecture alternatives are not approved. |
| Keyboard/rendering | Request/selection/draft/mutation identities, readonly navigation during uncertainty, grapheme editing including CRLF, cell/viewport bounds and escaped rendering without value copyback. | Fresh independent entire source review and meaningful admitted compiled/native cases. Saved source cases are UNRUN. |
| Native/archive integration | Existing historical producer/driver and archive code retained for provenance; current protected Core catalogs deliberately remain empty. | Complete genuine protected same-public-byte Core/CLI/TUI consumer and driver with real principal and managed service/template custody. Historical JWKS/fixture/list-difference receipts cannot qualify v2. |

A failed retained-record read is not proof of record absence. Legacy/unreadable/foreign-context records and unresolved native custody block new mutation/edit/cancel/clear while retaining useful read navigation and exact known-ID reconciliation. No warning or ready-to-release marker proves original bytes unchanged or native owner released.

Production B0/B2 finite measurements remain UNKNOWN; Q32GiB is qualification-only, with the fixed service1MiB/store4MiB/Core128MiB limits unchanged. The human Mac waiver is N/A, never PASS. Only the release owner can authorize publication, promotion or deployment.

## Historical compatibility record

The following retained material describes earlier operator/v1 compatibility source and historical candidate dependencies. Its old source hashes, bounds, unavailable recovery adapters and test claims are not current operator/v2 authority.
## TUI-CONNECTION

| Requirement | Current contract | Status |
| --- | --- | --- |
| `TUI-CONNECTION-001` | Named metadata-only profiles are selected with `--profile`, `SERVICE_LASSO_API_PROFILE`, or the configured default; `p` opens the keyboard selector. | Implemented; flags override environment, which overrides profile metadata. |
| `TUI-CONNECTION-002` | A `local-admin` profile declares `tokenEnv`; its environment value is sent only as `x-service-lasso-admin-token`. | Implemented; legacy profiles default to `local-admin` and are never silently converted to bearer auth. |
| `TUI-CONNECTION-003` | Token-bearing non-loopback HTTP is rejected; redirects are not followed and standard TLS verification remains enabled. | Implemented and locally tested. |
| `TUI-CONNECTION-004` | Non-2xx bodies are not retained; authentication and permission denials render only method, path, and HTTP status. | Implemented and locally tested. |
| `TUI-CONNECTION-005` | Reconnect retains a stale snapshot only for its current connection; switching clears it and discards old asynchronous results. | Implemented and locally tested. |
| `TUI-CONNECTION-006` | A non-loopback durable-operation connection explicitly declares `oauth-bearer` and `service-lasso:read` plus `service-lasso:lifecycle:write`. | Implemented client admission; Core remains authoritative for token and scope validation. |
| `TUI-LIFECYCLE-020` | Availability → Core preview → frozen one-shot durable submit for install/config/start/stop/restart. | Implemented against Core `55848ec`; reload is deliberately unavailable and the retired synchronous route is absent. |
| `TUI-OPERATIONS-020` | Actor/connection-bound operation readback with terminal and unknown-after-crash display; only advertised cancellation is offered. | In-memory readback is implemented. Persistent reconciliation is fail-closed pending Core #1553's reviewed server-issued opaque actor/client/instance context contract. |

Connection configuration contains only profile URLs and credential environment
variable names, for example `{"defaultProfile":"local","profiles":{"local":{"url":"http://127.0.0.1:17883","tokenEnv":"SERVICE_LASSO_API_TOKEN"}}}`.
Use `--connections <file>` to load it. Credentials never appear in that file or
on the command line.

The current `develop` candidate already contains dashboard and lifecycle work.
This matrix does not qualify it for operator coverage, a release, deployment, or
GA. The next implementation must start from its exact head and preserve its
existing acceptance harness.

## Issue1 complete client contract unit A (8 October 2026)
Active Development source unit, based on TUI effd6a69 and authentic current Core
ce56f592 / pinned Core2633. Issue1 remains OPEN; issue20 is the five-action dependency.
TUI-CLIENT-A1 (F1/F2): require versioned accepted/read/cancel envelopes, top-level
operation and cancellation result/terminal, own actor, exact operation/action,
closed status/outcome consistency, and bounded unique target IDs. Core initially
claims the requested singleton then replaces target IDs with authoritative
preflight/progress targets. Initial accepted submission contains the requested
service unless skipped; later reads admit any subset of the frozen preview,
including genuine shrink/empty scope, never unrelated targets.
Retain that frozen preview in memory only. Restoration has no persisted preview:
first authoritative own read establishes action and correlation identity; later
reads bind those identities while admitting genuine server-reported target replacement; never infer request identity from a list or time.
TUI-CLIENT-A2 (F3): typed closed stage errors; never render raw transport, decode,
filesystem, config or startup errors. HTTP diagnostic keeps only method, route
class and canonical status. Response ceiling 1 MiB, depth16, collection1024,
string4096 bytes, duplicate keys/trailing JSON/invalid UTF8 denied before typed
decode. Service list<=512, inbox<=20, history<=256, targets<=100.
TUI-CLIENT-A3 (F5/F6/F10): availability v1 requires exact selected service,
all five supported actions plus unavailable reload, complete typed authority,
unique known actions. Current update_check/update_download pair is admitted
only as unsupported metadata, never displayed as supported or submitted.
Safe reason projection uses fixed labels; unknown reason is unavailable without
rendering its value. Selected history/availability bind epoch, service, request
generation, reset/loading state. Refresh results bind generation, snapshot age
is visible, post-terminal refresh reads authoritative service state once.
Recheck availability at confirmation submission without replacing the preview.
TUI-CLIENT-A4 (F7): lost submit response before an ID is explicit uncertain,
no-ID state: refresh cannot read an empty ID or replay any mutation. No key,
request or confirmation persistence; server correlation/recovery remains Core
work. Known-ID reads and cancellation retain the original client and validate
server context when available. Context changes fail closed.
All actual-wire/model/storage/startup regression SOURCE remains LOCAL UNRUN
until DIFFERENT ENTIRE cumulative SOURCE GO and NEW complete-input ROOT plus
parent admission. F4 keyboard viewport/focus/Unicode, F8 full forms/workflows,
F9 shared native fixture, direct Windows/Linux five actions/cancel/recovery,
protected immutable four-file/two-archive publication and same-byte Core/CLI/
template integration remain subsequent required scope. Mac Deferred/N A never
PASS. Human selection is full Node with measured finite B0/B2; architecture05 is SOURCEIMPLEMENTATIONONLY. Production B0/B2 measurements remain UNKNOWN; Q32 is qualificationONLY.
### TUI-CLIENT-A1 restored read limitation (parent disposition, 8 October)
The persisted record has no preview or request tuple. For restored reads, bind
exact operation ID, authenticated own ownership, action, version/status/outcome
and server context, and render targets explicitly as server-reported. Permit
Core's genuine later replacement/expansion/shrink, including empty skipped
preflight targets; do not invent monotonic membership authority. Request/preview
correlation remains UNPROVED and owning Core recovery work. Restored cancellation
is unavailable until stronger request/preview correlation exists; reads never
create a preview, key, mutation, inferred ID, or request persistence. The current
in-memory submission retains the frozen preview and rejects foreign targets.
Authentic cancel requested can be cancelling/nonterminal or cancelled/terminal;
unsupported and too_late retain their actual terminal flag. Lifecycle actions
currently advertise false; update_check/download wire support is not TUI update
workflow availability or supported-action native acceptance.
### Successor06 source acceptance clarification (8 October 2026)
TUI-CLIENT-A1/A4 binds server operation correlation after the first authoritative
read and rejects later ID/action/ownership/context/correlation changes. Server
target replacement and shrink remain valid; restored scope is server-reported
and does not establish original request membership or cancellation authority.
Nonterminal progress is below100 with null outcome; finalizing may precede
authoritative terminal outcome. Failed/interrupted expiry does not prove mutation
failure. Terminal retained storage failure still triggers one authoritative
refresh, without mutation replay or discarding retained recovery evidence.
TUI-CLIENT-A2 rejects control characters in confirmation phrases and bounds
canonical error status. Closed durable authority envelopes use exact wire keys;
Core-generated timestamps/safety metadata are validated without retaining raw
private content. Existing genuine-invalid fixture corrections must strengthen
authentic Core wire admission; no protected assertion is weakened or skipped.
TUI-CLIENT-A3 requires a context read immediately before the confirmed POST
after availability recheck and preserves the frozen preview. Actual new regression
sources stay UNEXECUTED pending different ENTIRE review/NEW ROOT admission.

Recovery metadata admits only version2, canonical opaque operation ID, bounded
safe profile identifier and closed slrc1 server-context digest. Missing default
store reports typed storage unavailability. JSON authority keys are exact and
case-sensitive; wrong case cannot alias an admitted key. Time/safety metadata
validation is syntax/closed shape, not invented clock monotonicity. Core phase
identifiers follow its bounded grammar; no invented finite phase enum or
monotonic progress/cancellation/target constraint denies genuine transitions.

Preview effects admit Core's genuine bounded Unicode descriptions through its
300 UTF16-unit ceiling (at most1200 UTF8 bytes), rejecting terminal controls.
Do not impose the inherited ASCII-only240-byte bound on authentic Core output.
Keep original invalid-target/control regressions and add genuine Unicode/long
positive plus over-bound negative wire cases. This is F2 contract admission,
not F4 viewport/Unicode-editing or native acceptance completion.

Terminal refresh must retain a fixed safe recovery-storage warning separately
from ordinary refresh error clearing until actual Save/Clear succeeds. Model
and store boundaries classify injected raw exceptions without retaining their
private strings. Existing original-A byte retention, failed-clear visibility
and read-only recovery assertions remain mandatory, with typed-stage checks.

Author17 current readonly declared inspection (source only, UNRUN): actual InputFields topology/types/required properties/supplied constraints and safe reference labels/availability are inspectable through existing review keys without payload draft allocation or action-row retargeting. Opaque reference tokens and enum bytes remain withheld. Successful current form selection and failed-read status guard inspection; schema absence/unavailable/unsupported remain distinct. [Keyboard and privacy contract](declared-action-inspection.md). Schedules/concurrency/failure-policy metadata absent from the current OperatorAction DTO remains an explicit Core coherent-successor dependency; no fields, source values or execution authority are invented. The complete original all46/native/capture/finite owner programme remains unfinished.

## OPV2-R31-LOG-SEARCH-1 source checkpoint (author18)
Exact selected CODECS109 search query<=256UTF8bytes is authored in actual client/load/reducer/view and bound to connection/service/stream/context/query tickets, current page selections and remembered positions. `/` opens a separate read editor; Enter starts a fresh first page, Escape retains the prior query, Ctrl+u plus Enter clears. Original OperatorLogs callers remain no-search. Invalid UTF8/over-byte requests deny before GET; escaped display never copies back. Actual synthetic HTTP/reducer/Unicode/custody source cases are UNRUN, not Core/native issuance. Core current v2 log producer, sourceHandle catalog and tail/source/run/rotation ownership remain UNSUPPLIED/UNPROVEN full R31 obligations; no v1q fallback or source/native/full-delivery GO.