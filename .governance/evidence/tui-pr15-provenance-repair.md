# TUI PR #15 provenance and probe-privacy repair

Working baseline: `346983112413eddc95c7598b9244de5ef5ad3231` on
`feature/6-asset-acceptance`; PR #15 targets `develop`.

The Windows CI result at that baseline reported `terminal_exit_code_2` and the
closed `api_client_error` startup boundary. The source's only `NewClient`
failure exits are typed `ConfigurationError` values, so this result is retained
as a build-provenance contradiction. It is not evidence that an overlay caused
the hosted result, or that Go miscompiled the source.

`scripts/assert-go-source-provenance.test.mjs` creates a clean, detached Git
fixture, overlays the imported API client, and actually builds and runs the
binary. It demonstrates that an ambient Go overlay can produce
`api_client_error` while the binary still reports the fixture's clean VCS
revision and hashes itself. The source gate rejects that ambient `GOFLAGS`
input before any source test or build, requires `GOWORK=off`, verifies the
effective module, and requires readonly module resolution. CI and release
builds also require `-buildvcs=true`; release checks each staged binary's clean
revision metadata before archiving.

The probe-output unit test covers a valid nonce with unavailable binary
identity: it emits neither a marker nor the raw synthetic secret. Ordinary
operator startup failures still display their error outside probe mode.

Local evidence is surrogate only: Go unit tests, the adversarial overlay test,
the Node receipt tests, and the Python receipt tests. Fresh natural exact-head
hosted CI remains required. No candidate release, runtime acceptance,
deployment, publication, or GA claim is made here.
