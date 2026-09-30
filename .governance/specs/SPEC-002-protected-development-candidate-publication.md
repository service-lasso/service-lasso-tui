# SPEC-002: Protected Development-Candidate Publication

## Intent

Publish a reviewable TUI development candidate only when GitHub can enforce immutable release and protected-environment authority. This spec binds the TUI portion of Core `SPEC-007` `AC-7F` and `AC-7G`; it makes no release, GA, deployment, or promotion claim.

## Requirements

- **AC-1 Source identity:** manual dispatch accepts only `refs/heads/develop`, binds the exact full SHA, and derives a tag from its short SHA.
- **AC-2 Provider preflight:** before every write, the workflow reads and requires immutable releases enabled, the protected `development-candidate` environment with a one-to-thirty minute wait and protected-branch restriction, and current protected `develop` CI/review controls.
- **AC-3 Write authority:** only the `development-candidate` environment job has `contents: write`; it has a 30-minute bound.
- **AC-4 Candidate package:** one clean build creates Windows amd64, Linux amd64, macOS amd64, and macOS arm64 archives. Every extracted executable must retain the exact clean VCS metadata.
- **AC-5 Existing tags:** an existing tag is accepted only after a complete immutable exact-SHA receipt validates every expected asset name, provider digest, declared size, and the SHA-256 and size of newly read public bytes. The fixed inventory is the four platform archives, `SHA256SUMS.txt`, and `candidate-manifest.json`. All other collisions fail without overwrite, deletion, recreation, retagging, or a second write attempt.
- **AC-6 Publication receipt:** a new candidate is drafted once, receives the fixed inventory, then is published and read back as a non-draft immutable prerelease at the exact full SHA. The same headerless public-byte receipt is required after both a new publication and an existing-candidate recovery.
- **AC-7 Transport boundary:** release upload uses GitHub's Bearer-authenticated upload host. Public asset downloads use no `Authorization` header, follow only a bounded number of HTTPS redirects through the approved GitHub release hosts, and reject credentials, non-default ports, fragments, malformed paths, and unapproved query use. Downloaded body and temporary-storage bounds are derived from the provider-declared inventory; each streamed digest must equal both the local and provider SHA-256 values.

## Verification

Run behavior-focused Node controls for preflight, manifest schema, immutable receipt/inventory, public-body mismatch/truncation/missing/host-policy/redirect/bound failures, recovery refusal, and transport policy; run the Go suite and full build. Hosted workflow execution remains the required provider-side proof after review and settings application.

## Non-goals

Do not dispatch a candidate, mutate GitHub settings, modify existing releases, promote, deploy, or claim GA.
