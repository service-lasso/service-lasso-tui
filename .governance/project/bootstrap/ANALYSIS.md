# Bootstrap analysis

Issue #1 requires full keyboard operator coverage but only the TUI foundation
was present. Core `develop` supplies a documented lifecycle HTTP contract and
server-enforced permission/confirmation handling. Core has no stable TUI-owned
asynchronous operation polling, cancellation, inbox, or history contract,
which is recorded in `INIT-TODO.md` and SPEC-001.
