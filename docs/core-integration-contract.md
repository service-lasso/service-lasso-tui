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

The embedding package copies the exact matching asset unchanged, records its
version and SHA-256 with the Core release evidence, and never puts it under a
`services/` manifest or `services/*/.state` archive path.

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
plus a confirmed lifecycle result. TUI unit tests and mocked HTTP contracts are
surrogate evidence only. Platform resize, reconnect, and all workflow coverage
remain separate validation work.
