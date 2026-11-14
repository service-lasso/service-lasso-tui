# Ordinary source TUI end-to-end checks

Run from a clean TUI checkout with Go1.26.0, Node22.23.2 and Python3 on Linux:

```sh
node scripts/run-local-e2e.mjs --core-dir /absolute/core-checkout --expected-core-revision FULL_40_HEX_SHA --suite compat
```

The exact Core checkout must provide `scripts/operator-e2e-fixture.mjs`. The helper builds that revision, creates private isolated workspace/registry/auth state, starts a real loopback API on port0, and announces private connection and public provenance receipt paths. Override its path with `--core-fixture-helper`; specify an absolute Go binary with `--go`. The client runner owns the helper and sends `stop\n` in `finally`, waits for its actual exit, and checks that its credential file disappeared. The existing user runtime is never selected automatically. Output defaults to a new private temporary directory; `--output-dir` must name a fresh path.

The runner builds the current clean TUI revision, checks embedded VCS metadata, verifies actual invalid-token rejection, and drives its real POSIX terminal. Keyboard input waits for rendered UI and noncanonical terminal mode, including the initial terminal-color query delay. Compatibility coverage includes healthy dashboard, distinct help, Escape return, narrow/navigation/reconnect, resize and normal q with actual child wait. Raw terminal/build streams remain private; public `summary.json` contains closed stage results, source/artifact hashes, monotonic timings and token-leak Booleans. Retain evidence until review, then remove only your own output directory.

`--suite all` is the default. Compatibility success does not pass the v2 operator suite: the v2 stage observes unavailable/error UI and quit resilience but remains BLOCKED until full useful v2 journeys can run against the authoritative Core API. Both `all` and `v2` return nonzero for that missing qualification. This is separate from native profile/custody, installed-byte integration, publication and release acceptance.

Windows requires a matching-host ConPTY adapter and is explicitly BLOCKED by this POSIX runner. macOS POSIX execution can be observed separately but cannot borrow Linux or native acceptance; the current platform overlay's deferred applicability remains unchanged. Source fixture tests run with `node --test scripts/local-e2e.test.mjs`; their synthetic terminal proves observer timing/EOF behavior, never real backend or native authority.
