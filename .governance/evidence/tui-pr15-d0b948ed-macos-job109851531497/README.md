# PR 15 exact macOS failure receipt

- Repository: `service-lasso/service-lasso-tui`
- Pull request: [#15](https://github.com/service-lasso/service-lasso-tui/pull/15)
- Exact head: `d0b948ed60f9666d6d8478513b8ef6b036b8d65e`
- Workflow run: [36704529108](https://github.com/service-lasso/service-lasso-tui/actions/runs/36704529108)
- Failed job: [109851531497](https://github.com/service-lasso/service-lasso-tui/actions/runs/36704529108/job/109851531497), `macos-latest`, completed `2026-09-30T10:48:10Z`

## Preserved bytes

`job109851531497.logs.zip` is the exact GitHub log response bytes captured
from the job log endpoint. The endpoint returned UTF-8 text despite the local
filename selected for the archive request; the file was retained without
transformation.

- SHA-256: `240921589953f352b2086a3393e56e4932fec85136df31dfb0acf14146857252`
- `job109851531497.log-failed.txt` is the rendered `gh run view --log-failed`
  extraction for quick review.
- Rendered extraction SHA-256: `65cd862951011ac6f1d027de4e1c1abc2a16318b8ec44b4e4219b08035dd1419`
- Secret scan of both retained files found no GitHub token, PAT, AWS access
  key, private-key PEM, or assigned-secret-pattern match.

## Failure boundary

`go test ./...` and `go build ./cmd/service-lasso-tui` passed. The failure is
in `Test direct-probe evidence guards`, before the Node guard tests run:
`test_reconnect_timeout_uses_the_helper_path_and_writes_no_terminal_text`
expected a `startup/timed_out` receipt and received `launch/stage_failed`.

The receipt test injects `FakePty`, but `probe` still evaluates
`Backend.ConPTY` in its `spawn` call. On macOS `winpty` is unavailable, so the
module sets `Backend = None`; the dereference fails at launch. The real
ConPTY probe is explicitly Windows-only in `.github/workflows/ci.yml`, and
the Windows test/build job passed. This is a cross-platform unit-test harness
assumption, not direct evidence of a macOS TUI/product regression.

The bounded repair is to make the fake-PTY receipt-test path accept an
explicit/absent backend while keeping the actual ConPTY probe Windows-only.
Do not skip the guard step or relax the receipt assertion. This receipt does
not establish release, deployment, or GA acceptance.

## Matching Linux failure receipt

Linux job [109851531608](https://github.com/service-lasso/service-lasso-tui/actions/runs/36704529108/job/109851531608), also for exact head
`d0b948ed60f9666d6d8478513b8ef6b036b8d65e`, failed in the same
`test_reconnect_timeout_uses_the_helper_path_and_writes_no_terminal_text`
Python guard. Its Go tests and build passed first; the Windows-only real
ConPTY step was skipped as intended. It adds no different product or platform
failure boundary.

`job109851531608.logs.raw` contains the untransformed GitHub job-log endpoint
response, retained for this audit. SHA-256:
`6ac054bb3718d953219e3ead11987fa867693c4099126b79576573d70095c853`.
