Exact-head CI failure receipt for `d0b948ed60f9666d6d8478513b8ef6b036b8d65e`:

- macOS job [109851531497](https://github.com/service-lasso/service-lasso-tui/actions/runs/36704529108/job/109851531497) failed only in the Python receipt guard. `go test ./...` and `go build ./cmd/service-lasso-tui` passed before it.
- The failing test injects `FakePty` but `probe` still evaluates `Backend.ConPTY`. macOS has no `winpty`, so `Backend` is `None`; it writes a `launch/error/stage_failed` receipt instead of the expected `startup/timeout/timed_out` receipt.
- The real ConPTY probe is explicitly Windows-only. The Windows test/build job passed. This is a portable test-harness assumption, not direct macOS TUI/product regression evidence.
- Retained raw GitHub job-log bytes: `.governance/evidence/tui-pr15-d0b948ed-macos-job109851531497/job109851531497.logs.zip`, SHA-256 `240921589953f352b2086a3393e56e4932fec85136df31dfb0acf14146857252`; assigned secret scan found no matches.

Bounded repair: make the injected fake-PTY receipt-test path accept an absent/explicit backend while retaining the actual ConPTY probe as Windows-only. Do not skip the guard or weaken its receipt assertion. CI, direct candidate acceptance, release, and GA remain open.
