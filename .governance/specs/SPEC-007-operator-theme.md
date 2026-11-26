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
