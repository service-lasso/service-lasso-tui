# Protected development-candidate publication controls

The current scoped Windows/Linux candidate path retains the source controls
introduced by issue #18. The following GitHub settings are proposals for the
repository owner; this document does not apply them.

1. Enable **immutable releases** for `service-lasso/service-lasso-tui`. The release API must subsequently report `immutable: true` for a published candidate. A mutable API response is a failed candidate publication, regardless of tag or uploaded files.
2. Protect `develop`: require pull-request review, require branches to be up to date, disallow force pushes, and require the exact four checks used by the current scoped publisher: `Linux test and build`, `Windows test and build`, `Windows owned GOROOT overlay tests`, and `Scoped release asset cross-compilation`.
3. Create a protected `development-candidate` environment. Restrict deployment to protected branches, retain no custom branch-policy bypass, and configure a bounded 10-minute wait timer. The workflow's write job is also bounded to 30 minutes.

After an owner applies the settings, read them through GitHub before dispatching. The workflow independently reads `immutable-releases`, `environments/development-candidate`, and `branches/develop/protection` immediately before creating a draft release. Missing access or an unexpected response fails closed.

This current roster is enforced by
`scripts/verify-scoped-candidate-publication.mjs` and the scoped release path
in `.github/workflows/release-scoped.yml`. The historical three-OS publisher
retains its separate five-check policy in
`scripts/verify-candidate-publication.mjs`: Linux, Windows, Windows owned
GOROOT overlay, macOS, and legacy release asset cross-compilation. That
historical roster does not redefine the selected current Windows/Linux scope.
macOS is N/A for this scoped delivery, never a claimed pass. Required native
acceptance and authentic generated profile inputs remain separate mandatory
qualification gates; these four branch contexts alone do not establish them.

The retained provider readback returned `404` for `branches/develop/protection`;
that observation is a blocked owner-control prerequisite, not evidence that protection is in
place. The reviewable owner action is to apply the controls above and provide
an authenticated readback of the exact branch-protection, environment, and
immutable-release responses. No source workflow may apply or infer those
provider settings.

The workflow never sends an authorization header to a download URL. It uses the scoped publisher token only through GitHub's upload/API clients, and its testable transport policy permits headerless retrieval only from `github.com`, `github-releases.githubusercontent.com`, `objects.githubusercontent.com`, and `release-assets.githubusercontent.com` over HTTPS without URL credentials. The signed-object policy is deliberately closed: it accepts the retained bounded AWS release-asset grammar and the directly observed GitHub Azure grammar (`github-production-release-asset/<numeric>/<UUID>` with required bounded `sp`, `sv`, `se`, `sig`, and `jwt`, plus only named bounded Azure SAS/response keys). It does not admit arbitrary signed domains, paths, or query parameters.

The publisher step alone binds `GH_TOKEN` to the repository/environment secret
`DEVELOPMENT_CANDIDATE_TOKEN`. Provision a genuinely repository-scoped credential
with Contents write, Actions read and Administration read; the built-in Actions
token cannot perform mandatory Administration policy reads. Checkout, packaging,
verification and artifact actions receive no scoped publisher credential, and
built-in workflow/job Contents permissions remain read. An empty secret denies
before publication, with no fallback to `github.token`. Never copy broad operator
OAuth access into this secret or put credentials in arguments, logs or receipts.
An empty repository secret listing alone does not establish whether an
organization-level credential is available to this repository. Credential
provisioning and tag identity restrictions remain external prerequisites;
source wiring alone proves no live permission, policy, native acceptance or release.
