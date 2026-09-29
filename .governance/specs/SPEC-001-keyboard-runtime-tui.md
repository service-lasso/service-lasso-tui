# SPEC-001 Keyboard runtime TUI

Status: active

## TUI-CONNECTION

The client validates an HTTP(S) runtime URL, retrieves `/api/health`, and clearly
shows connected, unavailable, and retry states without replaying mutations.
It never follows redirects: a redirect response is a failed runtime request, so
the operator token cannot reach another origin or a downgraded transport.

## TUI-DASHBOARD

The client retrieves `/api/services`, renders a keyboard-navigable service list
and detail view, and presents safe lifecycle and health summaries.

## TUI-LIFECYCLE

For Core APIs that are documented and tested, the client invokes
`POST /api/services/:serviceId/{install|config|start|stop|restart|reload}`.
It presents an explicit local confirmation before a request and renders the
runtime's returned action result. A rejected request must create no retry.

## TUI-KEYBOARD

Arrow keys and `j`/`k` navigate; Enter opens a detail; Escape returns; `r`
refreshes; `?` shows contextual help; lifecycle shortcuts are visible in the
detail screen and require `y` to confirm or Escape to cancel.

## TUI-OPERATIONS

The Core action-run API is available but exposes server-side completed action
runs rather than a stable asynchronous operation polling contract. The client
may show its returned result for lifecycle work but cannot claim long-running
operation progress, cancellation, inbox, or history coverage until Core
publishes suitable contracts.

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
artifact. It never creates a GitHub release, tag, deployment, or GA claim.
Core #1461 may consume a candidate only after independent release review pins
the candidate version, full source SHA, manifest, assets, and digests; a
mutable release selector or incomplete asset set is rejected.
