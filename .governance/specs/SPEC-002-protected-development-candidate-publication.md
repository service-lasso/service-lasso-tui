# SPEC-002: Protected Development-Candidate Publication

## Intent

Publish a reviewable TUI development candidate only when GitHub can enforce immutable release and protected-environment authority. This spec binds the TUI portion of Core `SPEC-007` `AC-7F` and `AC-7G`; it makes no release, GA, deployment, or promotion claim.

## Requirements

- **AC-1 Source identity:** manual dispatch accepts only `refs/heads/develop`, binds the exact full SHA, and derives a tag from its short SHA.
- **AC-2 Provider preflight:** before every write, the workflow reads and requires immutable releases enabled, the protected `development-candidate` environment with a one-to-thirty minute wait and protected-branch restriction, and current protected `develop` CI/review controls. The `develop` protection readback must explicitly report `allow_force_pushes.enabled` as `false`; missing, enabled, or malformed values deny publication. A zero required-review count remains a legitimate repository policy and is not replaced by an invented reviewer-identity rule.
- **AC-3 Write authority:** only the `development-candidate` environment job has `contents: write`; it has a 30-minute bound.
- **AC-4 Candidate package:** one clean build creates Windows amd64, Linux amd64, macOS amd64, and macOS arm64 archives. Every extracted executable must retain the exact clean VCS metadata.
- **AC-5 Existing tags:** an existing tag is accepted only after a complete immutable exact-SHA receipt validates every expected asset name, provider digest, declared size, safe nonzero unique GitHub release-asset ID, and the SHA-256 and size of newly read public bytes. The fixed inventory is the four platform archives, `SHA256SUMS.txt`, and `candidate-manifest.json`, represented by exactly six records from the same release. Each record binds its ID, API record URL, browser download URL, name, digest, and size. All other collisions fail without overwrite, deletion, recreation, retagging, or a second write attempt.
- **AC-6 Publication receipt:** a new candidate is drafted once, receives the fixed inventory, then is published and read back as a non-draft immutable prerelease at the exact full SHA. The same headerless public-byte receipt is required after both a new publication and an existing-candidate recovery. The fresh publication job first checks out `needs.validate-source.outputs.sha`, reads back `HEAD`, and requires a clean tracked and untracked source tree before it downloads the SHA-named artifact, runs preflight, attempts recovery, or can write a release.
- **AC-7 Transport boundary:** release upload uses GitHub's Bearer-authenticated upload host. Public asset downloads use no `Authorization` header, follow only a bounded number of HTTPS redirects through exactly `github.com`, `github-releases.githubusercontent.com`, `objects.githubusercontent.com`, and `release-assets.githubusercontent.com`, and reject credentials, non-default ports, fragments, malformed paths, and unapproved query use. Downloaded body and temporary-storage bounds are derived from the provider-declared inventory; each streamed digest must equal both the local and provider SHA-256 values.

## Verification

Run behavior-focused Node controls for preflight, manifest schema, immutable receipt/inventory including malformed, missing, and duplicate asset IDs, public-body mismatch/truncation/missing/host-policy/redirect/bound failures, recovery refusal, and transport policy. Run the actual verifier's preflight and receipt modes from a fresh isolated job fixture containing only clean checked source and the downloaded artifact inputs; the controlled provider fetch must prove headerless public downloads, while a dirty source must fail before artifact retrieval. Run the Go suite and full build. Hosted workflow execution remains the required provider-side proof after review and settings application.

## Non-goals

Do not dispatch a candidate, mutate GitHub settings, modify existing releases, promote, deploy, or claim GA.
