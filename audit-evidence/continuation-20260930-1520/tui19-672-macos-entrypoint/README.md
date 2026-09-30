# TUI-19 failed macOS entrypoint custody

Provider retrieval was read-only on 2026-10-01. The failed CI run was `36783953490`, job `110120756504`, attempt `1`, for commit `672628173c29ef61855d5911f4f76bb2b61201d7` on `feature/18-protected-candidate-publication`.

The raw failed-step log retrieved with `gh run view 36783953490 --repo service-lasso/service-lasso-tui --job 110120756504 --log-failed`, UTF-8 encoded exactly as retrieved (including its final newline), has SHA-256 `27a5d8d9fbb7a78968da6b7adce2ec081d1be937bd1e7495ca6eb7725585e6b7`.

The raw provider job JSON retrieved with `gh api repos/service-lasso/service-lasso-tui/actions/jobs/110120756504`, UTF-8 encoded exactly as retrieved (including its final newline), has SHA-256 `07538e5713fc262f8491f328e2823d78a6bfbffdc9aa7f4264b64c9941aa68b8`. The corresponding raw provider run JSON from `repos/service-lasso/service-lasso-tui/actions/runs/36783953490` has SHA-256 `ec6c31a798803c3f686b60732dc91273d5621f92a9f94a812a712b19641a7889`.

The failed step invoked the actual Node test suite. It reported `SyntaxError: Unexpected end of JSON input` at `scripts/verify-candidate-publication-fresh-job.test.mjs:123:21`; the verifier subprocess succeeded without writing stdout. The source guard at this commit compared `process.argv[1]` directly to `fileURLToPath(import.meta.url)`. macOS path aliases can spell the same source path as `/var/...` and `/private/var/...`, so this comparison could evaluate false and skip the CLI body.

No dispatch, publication, provider settings mutation, merge, deployment, or `main` access occurred during this capture.
