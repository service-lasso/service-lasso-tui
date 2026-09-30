# PR 15 exact CI failure receipts

- Repository: `service-lasso/service-lasso-tui`
- Pull request: [#15](https://github.com/service-lasso/service-lasso-tui/pull/15)
- Exact head: `c5400b72898f22f3c444e12752e9ab3e168998e9`
- Workflow run: [36710206343](https://github.com/service-lasso/service-lasso-tui/actions/runs/36710206343)

These are untransformed bytes retrieved from each failed GitHub job-log
endpoint before diagnosis. A secret-pattern scan found only GitHub Actions'
masked `AUTHORIZATION: basic ***` setup line in each receipt; it found no raw
credential-shaped value.

| Job | Receipt | SHA-256 | Observed result |
| --- | --- | --- | --- |
| [macOS 109869918833](https://github.com/service-lasso/service-lasso-tui/actions/runs/36710206343/job/109869918833) | `job109869918833.logs.raw` | `beba487fd6668abf91ca3f9d9ec70f77b488392cf98acb6e9fab6de1caa37300` | `go test` and build passed; the Node receipt-handle test asserted a Windows-only rename denial. |
| [Windows 109869919131](https://github.com/service-lasso/service-lasso-tui/actions/runs/36710206343/job/109869919131) | `job109869919131.logs.raw` | `d4175a771ec2e195efa72853cb4aafb865b82fde3e9122a36c3332b0dcbce550` | `go test` and build passed; the real unavailable-state ConPTY helper returned a closed startup timeout. |
| [Linux 109869919217](https://github.com/service-lasso/service-lasso-tui/actions/runs/36710206343/job/109869919217) | `job109869919217.logs.raw` | `12505914efdfb1f58be98c3026226926a9a9994c8e715007cf996e2719ae05b5` | `go test` and build passed; the same Windows-only rename assertion failed. |

The cross-compilation job passed. These receipts do not establish candidate
acceptance, release, deployment, or GA.
