# Issue #125 delivery evidence

Development-only SPEC-007 TUI-DASHBOARD classification consumes Core #1985's
optional `stackIntent` scalar in authenticated operator/v2 stack-runtime rows.
Missing/unknown intent remains neutral and explicit; no service-name heuristics.
Fresh revision/generation/context-bound metadata distinguishes expected stopped
services from prepared, optional, disabled, unsupported, gated and unprepared.
Actual unhealthy summaries and real failed probes retain attention; existing
Core incident timelines remain durable timestamped observations.

Local source validation on 2026-11-27: `go test ./...`, `go vet ./...`, and
`go build -o /tmp/service-lasso-tui-125 ./cmd/service-lasso-tui` passed with
Go 1.26.0 from `/home/maxbarrass/.cache/tui37-tools/go/bin`. Tests exercise all
seven intents, omitted/unknown scalar compatibility, deny/failure/pending/stale
metadata, generation mismatch, exact 120x32 and 80x24 viewport bounds, selected
identity and recovery controls. Existing graph, metrics, keyboard and incident
regressions pass. This is fixture/source proof, not live Core or native acceptance.
Parent owns combined live Core PTY/recording proof and metadata integration.

Primary checkout is untouched. Owned managed worktree:
`/home/maxbarrass/.codex/worktrees/tui-125-stack-intent/service-lasso-tui`,
branch `feature/125-stack-service-intent`, based on develop `90ef4b2`.
Review, hosted gates, exact merged SHA and postmerge/worktree disposition are
recorded in the issue/PR delivery comments after actual completion.
