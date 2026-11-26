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
