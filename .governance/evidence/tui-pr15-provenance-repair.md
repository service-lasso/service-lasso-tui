# TUI PR #15 provenance and probe-privacy repair

Working baseline: `346983112413eddc95c7598b9244de5ef5ad3231` on
`feature/6-asset-acceptance`; PR #15 targets `develop`.

Run `36758877647` is the preserved exact-head CI failure. Its Windows job
`110036016339` passed the source admission and VCS-metadata checks, then
reported `terminal_exit_code_2` with the closed `api_client_error` startup
boundary; Linux, macOS, and cross-compilation passed. The full failed-job log
was retrieved without retry and has SHA-256
`4c72a395488b6d72e74fba8149001393b9d10f97a0db1e065109aa6dc2ce0826`.

The executable reported source identity `a1e768`, which is the permitted
GitHub pull-request merge candidate, not an unexpected branch stamp. It has
parents `0fa84ce` (the PR base at scheduling) and `781583a` (the exact PR
head); its merged tree `27830e` equals the locally computed merge tree for
those parents. The source's `NewClient` path with loopback HTTP and an empty
token has no generic error return: its only constructor failures are the five
typed configuration errors. The observed category therefore remains a
source/runtime contradiction. It is not evidence that an overlay caused the
hosted result, or that Go miscompiled the source.

`scripts/assert-go-source-provenance.test.mjs` creates a clean, detached Git
fixture, overlays the imported API client, and actually builds and runs the
binary. It demonstrates that an ambient Go overlay can produce
`api_client_error` while the binary still reports the fixture's clean VCS
revision and hashes itself. The source gate rejects that ambient `GOFLAGS`
input before any source test or build, requires `GOWORK=off`, verifies the
effective module, and requires readonly module resolution. CI and release
builds also require `-buildvcs=true`; release checks each staged binary's clean
revision metadata before archiving.

The direct real-Core script now invokes that same admission gate, refuses an
ambient `GOFLAGS` or non-off `GOWORK`, builds with readonly module resolution
and VCS metadata, then verifies its produced executable revision and clean
flag before the ConPTY helper starts it. The candidate workflow repeats the
revision and clean-flag checks after extracting each of its four archives; the
Windows candidate probe also applies them to its extracted executable. An
actual clean detached Windows build at `781583a` had empty effective flags,
the expected module, Windows AMD64 toolchain, matching clean VCS metadata, and
completed the bounded ConPTY unavailable-state probe. Direct typed invalid-URL
validation of that same executable exits with the expected configuration exit
code. These controlled observations exclude a general local ConPTY or
unchecked-source-build explanation but do not reproduce or explain the hosted
merge-candidate failure.

The probe-output unit test covers a valid nonce with unavailable binary
identity: it emits neither a marker nor the raw synthetic secret. Ordinary
operator startup failures still display their error outside probe mode.

Local evidence is surrogate only: Go unit tests, the adversarial overlay test,
the Node receipt tests, and the Python receipt tests. Fresh natural exact-head
hosted CI remains required. No candidate release, runtime acceptance,
deployment, publication, or GA claim is made here.

The first natural run for this repair (`36758552763`) reached the new clean
source gate. Its cross-compilation job passed; Linux, macOS, and Windows
completed their source tests then failed only because PowerShell evaluated the
multi-line `go version -m` result as an array. The repair joins that result
before requiring the two VCS fields. This is a workflow assertion correction,
not evidence of a failed provenance condition; no failed run is retried.
