# TUI PR #15 credential-precondition repair

Scope: service-lasso-tui Issue #6 / PR #15, after the normal merge of current
`origin/develop` `0fa84ce38630e7f5b0066d2aaa103c55b0485c06` into
`feature/6-asset-acceptance`.

The preserved hosted-binary audit at
`D:\projects\service-lasso\tui-15-hosted-binary-audit-20261001` inspected
job `110050733902`, its exact executable SHA-256
`3dfcc15663352191f00c0dad792256768030b6cb48956e20a187d9643866e346`, and
candidate source `a468daf8270f6e93e6168f50b76aa2611a421739`. The candidate
source contains `ResolveConnections`; it requires a non-empty credential
before calling `NewClient`. The existing direct constructor and unavailable
ConPTY harnesses supplied an invalid URL but no credential, so the observed
generic `api_client_error` was an unmet harness precondition rather than a
claim about ConPTY, an overlay, or the Go compiler.

The repair makes both child paths use a constrained allowlist environment plus
one fixed inert child-only `SERVICE_LASSO_API_TOKEN`. It preserves the
deliberately invalid direct URL and unavailable loopback endpoint, requires the
same held executable identity and marker binding, and keeps token values out
of command lines, receipts, logs, and test results. Empty-token versus inert-
token negative tests establish the required precondition without changing the
product's credential-required policy.

This evidence records diagnosis and local repair intent only. It does not
rerun the old failed job, qualify a release asset, establish real-runtime
acceptance, publish a candidate, or establish deployment or GA.
