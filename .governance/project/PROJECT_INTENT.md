# Service Lasso TUI project intent

Provide a standalone, keyboard-first terminal client for a running Service
Lasso Core runtime. The TUI is an attached operator process: Core does not
supervise, start, or manage an interactive terminal session.

The client consumes documented HTTP APIs and keeps Service Lasso authoritative
for authentication, permissions, confirmation, lifecycle execution, auditing,
and durable result state. It must never expose credentials or raw sensitive
server error bodies.

Distribution candidates are immutable, explicitly dispatched from `develop`,
and checksum-bound to their source commit. After all platform smoke jobs pass,
the manual dispatch creates a clearly labelled prerelease candidate with the
same retained assets for later Core packaging review; it does not establish GA.

Issue #18 refines that contract: a candidate publication must fail closed until
GitHub reports immutable releases, a protected `development-candidate`
environment, and protected `develop` controls including an explicit disabled
force-push setting. The publication job must bind its checked-out `HEAD` to the
validated full SHA before consuming the matching candidate artifact or running
any verifier. Candidate binaries remain development evidence, never a claim of
GA, promotion, deployment, or a successful Core operation.
