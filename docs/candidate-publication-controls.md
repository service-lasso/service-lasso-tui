# Protected development-candidate publication controls

Issue #18 adds source controls only. The following GitHub settings are proposals for the repository owner; this change does not apply them.

1. Enable **immutable releases** for `service-lasso/service-lasso-tui`. The release API must subsequently report `immutable: true` for a published candidate. A mutable API response is a failed candidate publication, regardless of tag or uploaded files.
2. Protect `develop`: require pull-request review, require branches to be up to date, disallow force pushes, and require these four current checks: `Linux test and build`, `Windows test and build`, `macOS test and build`, and `Release asset cross-compilation`.
3. Create a protected `development-candidate` environment. Restrict deployment to protected branches, retain no custom branch-policy bypass, and configure a bounded 10-minute wait timer. The workflow's write job is also bounded to 30 minutes.

After an owner applies the settings, read them through GitHub before dispatching. The workflow independently reads `immutable-releases`, `environments/development-candidate`, and `branches/develop/protection` immediately before creating a draft release. Missing access or an unexpected response fails closed.

The workflow never sends an authorization header to a download URL. It uses the workflow token only through GitHub's upload/API clients, and its testable transport policy permits headerless retrieval only from `github.com`, `github-releases.githubusercontent.com`, `objects.githubusercontent.com`, and `release-assets.githubusercontent.com` over HTTPS without URL credentials.
