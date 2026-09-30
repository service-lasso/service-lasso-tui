# PR 15 exact Windows CI failure receipt

- Repository: `service-lasso/service-lasso-tui`
- Pull request: [#15](https://github.com/service-lasso/service-lasso-tui/pull/15)
- Exact head: `dcce259bddd888c3e7b92731b4406d046aaad0f5`
- Workflow run: [36720265137](https://github.com/service-lasso/service-lasso-tui/actions/runs/36720265137)
- Job: [109903285182](https://github.com/service-lasso/service-lasso-tui/actions/runs/36720265137/job/109903285182)

`job109903285182.logs.raw` is the untransformed 20,365-byte job-log response
captured before this repair. Its SHA-256 is
`1cb86198621a29a333ff556f357d003bcd940775cf8c50ead19c2d9e548a56d6`.

The Windows job completed Go tests and its build, installed `pywinpty` 3.0.5,
then the unavailable-state ConPTY helper returned only a closed startup receipt
with `terminal_closed`. The raw log contains no child diagnostic sufficient to
attribute why EOF occurred. The EOF classifier repair is source evidence only:
it requires fresh exact-head hosted Windows helper CI and independent review.
This receipt does not establish candidate-archive acceptance, a native receipt
writer result, direct Core acceptance, publication, deployment, or GA.
