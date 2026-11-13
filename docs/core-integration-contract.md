# Core integration and release contract

Tracking dependency: [service-lasso/service-lasso#1461](https://github.com/service-lasso/service-lasso/issues/1461).

The TUI is launched by an operator after Service Lasso Core is already running.
Core does not register, start, stop, supervise, or health-check an interactive
TUI process. The only connection boundary is an operator-supplied API URL and
a token delivered through `SERVICE_LASSO_API_TOKEN`.

## Release assets

For release version `<version>`, this repository publishes the following
platform assets:

| Asset | Executable inside asset | Core bundle placement |
| --- | --- | --- |
| `service-lasso-tui-<version>-win32-amd64.zip` | `service-lasso-tui.exe` | `tools/service-lasso-tui/win32-amd64/service-lasso-tui.exe` |
| `service-lasso-tui-<version>-linux-amd64.tar.gz` | `service-lasso-tui` | `tools/service-lasso-tui/linux-amd64/service-lasso-tui` |
| `service-lasso-tui-<version>-darwin-amd64.tar.gz` | `service-lasso-tui` | `tools/service-lasso-tui/darwin-amd64/service-lasso-tui` |
| `service-lasso-tui-<version>-darwin-arm64.tar.gz` | `service-lasso-tui` | `tools/service-lasso-tui/darwin-arm64/service-lasso-tui` |

The embedding package copies the exact matching asset unchanged, records its
version and SHA-256 with the Core release evidence, and never puts it under a
`services/` manifest or `services/*/.state` archive path.

## Develop candidate handoff

Issue #6 creates retained candidate artifacts only through an explicit
`develop` workflow dispatch. The dispatch requires an exact version tied to
the checked-out source SHA, produces all four assets and `SHA256SUMS.txt`, and
uploads a candidate manifest that records the immutable source identity and
asset digests. Once its source and native platform smoke jobs pass, the manual
dispatch creates a prerelease candidate tag and release containing the exact
same assets. It is not a GA release. Core must reject a candidate that is
incomplete, mutable, or checksum-mismatched.

The candidate workflow runs a native `--help` terminal-entrypoint smoke on
hosted Windows, Linux, and macOS before packaging. It cross-builds both macOS
architectures; the architecture not represented by the hosted macOS runner
has extraction and binary-structure evidence only until native hardware
acceptance is recorded.

## Launch

Windows:

```powershell
$env:SERVICE_LASSO_API_TOKEN = '<operator token>'
.\tools\service-lasso-tui\win32-amd64\service-lasso-tui.exe --api http://127.0.0.1:17883
```

Linux and macOS use the corresponding executable path with
`SERVICE_LASSO_API_TOKEN` in the process environment. Operators may point the
client at a remote TLS URL with `--api`; remote validation must preserve normal
TLS verification.

## Required acceptance evidence

An integrated release needs the exact Core and TUI asset identities, checksum
verification, and a real Core runtime exercise of authenticated service listing
plus a confirmed lifecycle result. CI runs native Go test/build jobs on hosted
Windows and macOS, while the release-asset compile job checks declared target
architectures. Both are surrogate evidence: platform resize, reconnect, and
real packaged interactive-terminal acceptance remain separate validation work.
