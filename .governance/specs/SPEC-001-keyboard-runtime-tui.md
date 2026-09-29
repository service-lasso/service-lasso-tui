# SPEC-001 Keyboard runtime TUI

Status: active

## TUI-CONNECTION

The client validates a runtime URL, retrieves `/api/health`, and clearly shows
connected, unavailable, and retry states without replaying mutations.

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
