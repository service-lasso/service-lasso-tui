# Service Lasso TUI project intent

Provide a standalone, keyboard-first terminal client for a running Service
Lasso Core runtime. The TUI is an attached operator process: Core does not
supervise, start, or manage an interactive terminal session.

The client consumes documented HTTP APIs and keeps Service Lasso authoritative
for authentication, permissions, confirmation, lifecycle execution, auditing,
and durable result state. It must never expose credentials or raw sensitive
server error bodies.

Distribution candidates are immutable, explicitly dispatched from `develop`,
and checksum-bound to their source commit. Candidate artifacts support later
Core packaging review; they do not publish a release or establish GA.
