# SPEC-007: issue #114 operator theme

Active additive implementation scope for SPEC-001 TUI-DASHBOARD, TUI-KEYBOARD,
TUI-LIFECYCLE and selected operator/v2 B04/E01. Depends on #112 / PR #113.

The approved reference uses navy canvas and padded surfaces, teal focus, green
healthy, amber degraded, coral failure, compact header, actual view tabs, four
summary tiles, service list, selection-linked inspector and attention feed.
No decorative destinations, synthetic health strip, uptime, latency, restart
count, incident or freshness claim may be rendered as observed data.

The Stack view uses only current admitted overview observations and exactly
the same filtered service order/index as Enter. Four tiles show healthy, needs
attention, operation state and unavailable Core uptime. The inspector displays
observed summary fields and explicit unavailable telemetry; the attention feed
derives stopped/unhealthy conditions only, labelled as observations without
invented incident times. Paging and failed/loading reads remain visible.

Actual operatorViews drive the tabs; left/right and Tab retain existing focus
semantics. Narrow terminals retain compact semantic content. Monochrome keeps
headings, state words and selection markers. Every other view, form, preview,
operation, error and recovery state uses the same theme while preserving the
existing viewport priority and full authoritative safety/readback messages.

Acceptance proof: render/cell bounds at 80/120 columns and short heights;
selected service matches inspector/Enter; loading/error/empty/stale readings;
no fabricated telemetry; existing focus, preview, redaction and mutation tests;
Go formatting/tests/build plus disposable Core PTY captures with root owner.
These source/fixture proofs do not establish release/native qualification.

Missing measured latency, Core uptime, restarts, historical health and incident
projection are owned by service-lasso/service-lasso#1973. No legacy/raw API
substitute is admitted here. Empty operation state says None known; retained
operations use the full safety/result view with authoritative targets/readback.
At 80 columns list and inspector stack at full width; shared read detail keeps
1/2/3 overview/dependency/endpoint tabs and existing keyboard/read admission.

Local freshness is the elapsed time since a successful current-ticket overview
read passed applyLoaded. Failed/obsolete reads never renew it; a changed
selection hides it until its own read succeeds. The separate snapshot status
still reports stale/incomplete state. Refresh remains manual, with no pause or
auto-refresh claim. At 80x24 retain at least four service rows, full-width
compact inspector, untruncated short tiles and explicit telemetry provenance.

Issue #114 monitoring refinement: the existing one-second timer may issue one
read-only first-page overview refresh per five seconds only while the admitted
current overview is complete, untruncated and never accumulated continuation
pages, row focus is active, and no read/bootstrap/draft/operation/safety state
is pending. Space toggles monitoring pause/resume without dispatching a mutation.
Retain failed-read rows/last successful age; retry no faster than five seconds.
Preserve selected service identity on replacement/reorder; obsolete responses
remain rejected. Mode/shortcut labels must reflect the actual scheduler state.

Auto refresh is additionally suspended in local favorites-only mode: load(nil)
clears read-bound metadata and must not perturb filtered selection mid-read.
Completed continuation pages are excluded by their retained seenCursors set.

Show the cancellation shortcut only for an admitted pending operation whose
cancellation is supported and has no recorded or in-flight attempt.

Issue #115 responsive refinement: preserve Stack hierarchy from 60x18 through
160x45 and wide short viewports. Four aligned metrics use concise compact
labels; service and inspector stack below 110 columns. Budget every pane before
rendering so selection, inspector identity, attention provenance and keyboard
recovery remain visible. At tall sizes cap empty body space to observed content,
keeping the attention feed adjacent to the data. Pane headings, column labels,
selection and muted footer distinguish the list from its inspector. Never
introduce fabricated telemetry or reorder runtime observations for appearance.

The shared Stack/monitor minimum is 60x18; monitoring admission keeps all
existing data and safety requirements at that size. Recovery keys stay at the
terminal bottom with extra canvas after the attention feed. Stopped, degraded,
healthy and unknown service states retain semantic colors and explicit words.

Issue #117 mockup refinement: at wide sizes use a separate brand and truthful
freshness/status header, primary numbered Stack shortcuts, semantic tile rails,
striped service rows with a selected-row rail, a severity-led inspector, a
tinted attention feed, and individually rendered keyboard keycaps. Spend
available reference-height rows on the panels while keeping tall sparse
observations together. Compact and monochrome layouts retain explicit text and
recovery hints. Health coverage counts checked summaries within the observed
selection. Preserve runtime service order and the exact selected service used
by Enter and shortcuts. Stack numbers focus services or open selected service
detail, operation history, or logs. All navigation continues through normal
read admission and does not control operations. Core #1973 remains the
dependency for real telemetry and stack-wide operation counts.

Issue #119 Stack health graphs (TUI-DASHBOARD): display the selected service's
primary HTTP-check latency over the last fifteen minutes and a pass/fail strip
per listed service from actual admitted operator/v2 observations (Core #1973).
Read the additive GET /health-check-samples endpoint in one bounded batch for
each admitted overview page. Its closed frame contains observation time,
900-second window, and only requested service IDs with configured state and
actual chronological timestamp, pass/fail, and integer duration samples.
Optional reads never delay publication of the overview or change its readback
and navigation semantics. Drop obsolete responses, scope graphs to the current
page selection, and expire readings. An older Core, denied/failed reads, no HTTP
check, no recent samples, and stale readings retain explicit text states.

At reference size plot timestamp-binned latency with labeled millisecond scale
and the latest observed duration, plus timestamp-binned service strips. Empty
buckets have no bar; a failed check has distinct color and shape. Compact and
monochrome windows preserve pass/fail meaning, selected service identity, and
keyboard recovery. Graph rendering never substitutes samples for missing data or
implies real runtime uptime, restart counts or incident history from probes.
The latest HTTP result may label a service whose aggregate health is unknown.
An observed failed HTTP check joins the attention feed once per service with its
recorded UTC timestamp, while known stopped and degraded summaries retain
precedence. When present, HTTP observations supply explicitly scoped HTTP
check coverage and pass/fail totals without promoting aggregate health. Pause
retains the admitted frame while its overview remains valid and labels the
chart endpoint as its last observation.

Issue #121 completes Stack runtime telemetry (TUI-DASHBOARD). Read the
additive, bounded operator/v2 GET /stack-runtime view from Core #1973 for
the current admitted overview page. Its observed values supply Core uptime,
current-run service uptime, durable service restart counts, workspace-scoped
active durable operation count and recent UTC incidents. Nullable metrics
and histories are unavailable; known empty history and zero counts are
explicit. Display only safe fixed incident labels and server-supplied
timestamps. A nonempty timeline contains only newest-first durable Core
incidents. Current service and HTTP summaries remain the attention fallback
when incident history is empty or unavailable. Keep check strips and the fifteen-minute latency chart.

Read and publish runtime telemetry independently of overview and check
samples. Confirm its existing reconciliation context and selector before
retaining it; reject obsolete responses, server mismatches and expired reads.
When paused, the display retains only the admitted observation while the
overview remains valid. Partial, denied, failed and older-Core responses do
not promote unavailable values. Wide layouts add RUN UPTIME to the service
table and retain selected uptime/restarts in the inspector. Compact and
monochrome layouts retain selection, telemetry status and keyboard recovery.

Issue #123 (TUI-DASHBOARD) keeps graphical space present for every check state.
No configured HTTP check uses a full-width muted neutral strip; configured
checks without samples use a flat muted guide. Pending, stale, unavailable and
denied observations retain distinct safe labels alongside neutral graphics.
Wide latency panels retain axes and neutral guides without numeric sample claims;
compact panels retain a graphical guide or timestamp-binned sample strip. Real
observations retain durations, failures, gaps and existing freshness rules.
Validate no-check, empty and real states from 60x18 through 200x20.

Issue #123 missing-credential diagnosis retains the fixed generic observer
prefix and adds a fixed hint to load the local admin token in this terminal.
No environment variable names, private profile names, URLs or values are exposed;
profile/transport admission still precedes credential lookup.


Issue #125 (TUI-DASHBOARD) classifies Stack service intent using authoritative
Core metadata, separately from observed running and health state. Stopped
expected daemons require attention; prepared providers, optional services,
disabled services, unsupported services and first-run gated services retain
neutral explicit states. Never infer intent from IDs, names, check presence or
a missing metadata record. Missing, denied, failed, stale or older-Core metadata
retains an explicit unknown intent and its safe status. Running/unhealthy and
actual failed checks remain observed failures, and durable timestamped incident
history remains history. Preserve the existing dark hierarchy, keyboard
selection and inspector, uptime/restarts/operations, neutral no-probe/no-data
graphics and real timestamped latency samples at 120x32 and 80x24. Metadata
contracts must be agreed with Core before implementation and use the existing
authenticated connection, context and freshness safety boundaries.

Agreed Core #1985 contract: optional scalar `stackIntent` on each authenticated
operator/v2 stack-runtime row: expected, prepared, optional, disabled,
unsupported, gated, or unprepared. Unprepared means a supported provider lacks
actual installed/configured preparation. Omitted or unknown values carry no
intent claim. Reuse the runtime frame's reconciliation context, requested IDs,
revision/generation binding, observation receipt and expiry.


Issue #126 TUI-CONNECT: --connect opens a dark keyboard connection screen
before any default target or credential lookup. Resolve repo-local runtime in
the Core wrapper first. Display/edit URL, validate and confirm exact target,
then accept a masked local-admin token for loopback or explicitly selected OAuth
bearer with general-read admission for remote HTTPS. Never persist credentials
or follow redirects. Authenticate through real operator/v2 discovery and context
before admitting the overview. Safe rejection allows retry or target editing.
Missing credentials on a real terminal open setup for the selected profile;
nonterminal resolution failures remain noninteractive. Preserve configured
profiles, lifecycle safety, existing input custody and explicit CLI precedence.

Target confirmation and credential submission require the entire admitted URL
to fit visibly, including after resize. Otherwise block the transition and give
resize guidance; never confirm or send a credential to a hidden origin suffix.

Issue #145 (TUI-DASHBOARD): Stack selected-service actions. Show `a Actions`
in the keyboard footer. The shortcut opens the existing Lifecycle journey for
the admitted selected service; Enter remains Inspect. Missing, stale, expired,
loading, foreign-selection and invalid-row observations cannot supply a target.
Navigation only reads lifecycle availability and never sends a mutation. Existing
authentication, capability availability, revision binding, preview and exact
confirmation gates remain authoritative. Verify selection and read-only HTTP
routing plus wide/compact footer layout and action-admission regressions.

Issue #143 (TUI-DASHBOARD): RUN UPTIME is blank in the Stack service table
unless the service observation confirms `running: yes`. This includes stopped,
prepared, disabled, unsupported, and unknown observations, even if telemetry
contains an uptime sample. Running rows retain measured uptime and explicit
Pending/unavailable telemetry status. Preserve STATE, selected inspector uptime
and restart status, column alignment, and check graphics at 120x32 and 80x24.

Issue #143 owner clarification: show canonical `prepared` intent as green
`Ready` in Stack STATE and selected inspector status. Ready denotes prepared
capability, never a running observation; non-running Ready rows have blank
RUN UPTIME. Health/check failures retain their existing precedence.

Issue #149 (TUI-PINNED-RELEASE): For selected @tui and @cli, existing Stack
Actions exposes Check new releases and installation of the checked exact tag.
Use Core #2015 POST /api/services/:id/pinned-release/check with {} and
POST /api/services/:id/pinned-release/install with {tag,confirm:true}. Show
current and candidate tags, require explicit confirmation, bind async responses
and install requests to the selected connection/service/context, and show safe
unavailable/error results from older Core. Installation takes effect on the next
client launch. Existing lifecycle actions remain available. Verify HTTP/auth,
confirmation/cancellation, stale selection, error redaction and compact layout.
