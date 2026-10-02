# SPEC-002: Protected Development-Candidate Publication

## Intent

Publish a reviewable TUI development candidate only when GitHub can enforce immutable release and protected-environment authority. This spec binds the TUI portion of Core `SPEC-007` `AC-7F` and `AC-7G`; it makes no release, GA, deployment, or promotion claim.

## Requirements

- **AC-1 Source identity:** manual dispatch accepts only `refs/heads/develop`, binds the exact full SHA, and derives a tag from its short SHA.
- **AC-2 Provider preflight:** before every write, the workflow reads and requires immutable releases enabled, the protected `development-candidate` environment with a one-to-thirty minute wait and protected-branch restriction, and current protected `develop` CI/review controls. The `develop` protection readback must explicitly report `allow_force_pushes.enabled` as `false`; missing, enabled, or malformed values deny publication. A zero required-review count remains a legitimate repository policy and is not replaced by an invented reviewer-identity rule.
- **AC-3 Write authority:** only the publisher step in the `development-candidate` environment job receives the scoped publication credential; the built-in workflow token retains `contents: read` and the job has a 30-minute bound.
- **AC-4 Candidate package:** one clean build creates Windows amd64, Linux amd64, macOS amd64, and macOS arm64 archives. The manifest uses the fixed names `service-lasso-tui-${version}-win32-amd64.zip`, `service-lasso-tui-${version}-linux-amd64.tar.gz`, `service-lasso-tui-${version}-darwin-amd64.tar.gz`, and `service-lasso-tui-${version}-darwin-arm64.tar.gz`, with `service-lasso-tui.exe` only for Windows and `service-lasso-tui` for every other platform. Every extracted executable must retain the exact clean VCS metadata.
- **AC-5 Existing tags:** an existing tag is accepted only after a complete immutable exact-SHA receipt first recomputes SHA-256 and byte size from the six fixed regular files in the downloaded candidate directory and binds them to the candidate-local inventory. The four manifest archive hashes and every `SHA256SUMS.txt` archive entry must equal those computed archive hashes; the manifest checksum hash must equal the computed checksum-file hash. Candidate manifest and inventory metadata must be bounded regular files at their fixed names; symlinks, path escape, extra or attacker-chosen names, and mismatches deny before provider reads, recovery, or writes. The publication directory must contain only the six fixed files and `candidate-local-assets.json`; verification of the six files itself does not adopt unrelated files. The candidate manifest has closed nested `source`, `release`, `checksumManifest`, and per-asset object shapes. It then validates every expected asset name, provider digest, declared size, safe nonzero unique GitHub release-asset ID, and the SHA-256 and size of newly read public bytes. The fixed inventory is the four platform archives, `SHA256SUMS.txt`, and `candidate-manifest.json`, represented by exactly six records from the same release. Each record binds its ID, API record URL, browser download URL, name, digest, and size. All other collisions fail without overwrite, deletion, recreation, retagging, or a second write attempt.
- **AC-6 Publication receipt:** a new candidate is drafted once, receives the fixed inventory, and is verified before it can be published. That draft gate reads the actual draft release by its numeric ID, requires its exact tag, full SHA, `draft: true`, prerelease state, and exactly six asset records, then reads each asset's actual binary bytes against the original six verified held byte sequences. Only then may the workflow patch `draft: false`; it finally reads back a non-draft immutable prerelease at the exact full SHA. New-candidate upload streams the original, verified six held byte sequences directly to GitHub's fixed upload host; it never reopens a mutable candidate pathname after the final identity, hash, and size check. The same headerless public-byte receipt is required after both a new publication and an existing-candidate recovery. The fresh publication job first checks out `needs.validate-source.outputs.sha`, reads back `HEAD`, and requires a clean tracked and untracked source tree before it downloads the SHA-named artifact, runs preflight, attempts recovery, or can write a release.
- **AC-7 Transport boundary:** release upload uses GitHub's Bearer-authenticated upload host. The pre-publication draft gate may use that token only in memory for `GET https://api.github.com/repos/service-lasso/service-lasso-tui/releases/<numeric-id>` and its six fixed asset-record endpoints with `Accept: application/octet-stream`; every API redirect immediately drops authentication. Final and recovery public asset downloads use no `Authorization` header. Redirects are bounded, HTTPS-only, and restricted to `github.com`, `github-releases.githubusercontent.com`, `objects.githubusercontent.com`, and `release-assets.githubusercontent.com`; initial GitHub paths are the exact release tag/name route. Signed object redirects accept only either the existing bounded AWS form (`github-production-release-asset-2e65be/<numeric>/<opaque>` plus exactly `X-Amz-Algorithm`, `X-Amz-Credential`, `X-Amz-Date`, `X-Amz-Expires`, `X-Amz-SignedHeaders`, and `X-Amz-Signature`) or the observed GitHub Azure form (`github-production-release-asset/<numeric>/<UUID>` plus required non-empty bounded `sp`, `sv`, `se`, `sig`, and `jwt`; optional, unique, non-empty bounded Azure keys are limited to `sr`, `spr`, `rscd`, `rsct`, `skoid`, `sktid`, `skt`, `ske`, `sks`, `skv`, `response-content-disposition`, and `response-content-type`). The Azure form was directly observed from one public GitHub release asset as an HTTPS `302` to the fixed release-assets host; only its host, path class, key names, uniqueness, and value bounds were recorded, never values or the signed URL. All credentials, non-default ports, fragments, malformed paths, duplicate or unexpected query keys, raw signed URLs, and response bodies are rejected or withheld from logs. Downloaded body and temporary-storage bounds are derived from the provider-declared inventory; each streamed digest must equal both the local and provider SHA-256 values.

## Verification

Run behavior-focused Node controls for preflight, manifest schema, recomputed local-byte custody (including substituted archive, checksum, manifest, and inventory failures before provider access), immutable receipt/inventory including malformed, missing, and duplicate asset IDs, draft membership and original-held-byte verification before patch, public-body mismatch/truncation/missing/host-policy/redirect/bound failures, recovery refusal, and transport policy. Prove that upload keeps tokens out of child-process arguments and that every redirect is headerless. Run the actual verifier's preflight and receipt modes from a fresh isolated job fixture containing only clean checked source and the downloaded artifact inputs; the controlled provider fetch must prove headerless public downloads, while a dirty source must fail before artifact retrieval. Run the Go suite and full build. Hosted workflow execution remains the required provider-side proof after review and settings application.

## Non-goals

Do not dispatch a candidate, mutate GitHub settings, modify existing releases, promote, deploy, or claim GA.

Issue #18 source repair binds actual Git tag refs and recursively resolved annotated objects to the exact candidate SHA. Orphan, malformed, cyclic and mismatched tags deny all writes. The publisher retains validated provider policy and re-reads identical policy immediately before every create, asset upload and publish mutation; changed or unavailable policy denies further writes. Source-only implementation and regressions remain UNEXECUTED pending entire independent SOURCE GO and new complete-input ROOT admission. Full native five-action/keyboard/error/cancel and three-OS same-byte Core acceptance remain pending.

AC-5/AC-6: After proving no existing release or tag, create one lightweight ref at the full source SHA, resolve it through the actual Git ref API, then create the private release. Each mutation has its own immediate retained-policy read. Failure retains any newly created ref/draft as an unrecoverable collision unless a later read-only complete immutable receipt succeeds; never delete, retag or retry writes. Annotated existing refs are dereferenced recursively with bounded depth, identity and cycle checks. The public receipt checks the same tag chain before and after all six bytes.

## PR25 final9d five-finding source successor (issues18 and20)
Issue18 / SPEC002 AC5 and AC6: retain the first admitted manifest bytes and
require exact equality with the acquired held manifest before ANY provider
access. Regress coherent manifest plus inventory replacement at acquisition.
Issue20 / SPEC001 TUI-ACCEPTANCE-001/002/003 and TUI-DISTRIBUTION-002:
F2 retains the actual direct-owned runtime immediately after spawn, before
birth/select/read/parse, and closes/waits with unknown birth remaining unknown.
F3 preserves primary reconnect spawn failure plus independent Node/native sink
rejection without inventing a helper receipt. F4 identity-guards restoration
and removal of only the positively bound redundant original hardlink alias;
repeat and interrupted rename/symlink/mutation regressions remain required.
F5 targets the actual observer owner command for launch/handoff faults and
records the reached boundary; Darwin /bin/ps remains outside those faults.
All five form one SOURCE-ONLY bundle. Prior6+5+2+3 assertions, provider policy,
private held-byte gates, Core authority, TLS and secrecy stay mandatory.
All new regressions are UNEXECUTED until DIFFERENT fresh ENTIRE SOURCE GO plus
NEW complete-input ROOT admission. No native or full-delivery claim is made.
## Issue18 scoped development publisher credential wiring
SPEC-002 AC-2/AC-3/AC-7 requires DEVELOPMENT_CANDIDATE_TOKEN from a genuinely
repository-scoped credential with Contents write, Actions read and Administration
read. Bind GH_TOKEN only to the actual publisher step; no github.token fallback,
credential in argv/logs, or inheritance by checkout/build/download actions.
The built-in workflow token retains Contents read. Missing authority fails closed
before the publisher. All nine mutation gates, repeated policy reads, exact tags,
six original held assets, private draft and headerless public byte checks remain.
Source regression is UNEXECUTED pending DIFFERENT fresh ENTIRE SOURCE GO and NEW
complete-input ROOT admission. No secret/settings/identity is provisioned here;
mandatory native/Darwin/Core and same-byte publication gates remain unmet.