# TUI29 cross-repo checksum verification (2026-11-17)

## Purpose and authority

This record verifies the current TUI source against the normative scoped
candidate checksum contract in `SPEC-003` and the pinned Core `SPEC-008`
inventory. It is a focused source verification, not the independent review
required to close #29.

It does not claim any of the following:

- a fresh different ENTIRE cumulative four-repo source review;
- `SOURCE_GO`, issue closure, or owner acceptance;
- native, product, parser, compiler, build, npm, ACL, provider, or publication
  execution;
- packaged same-byte, release, deployment, or GA qualification.

#29 remains open until an independent reviewer and the owner accept the
required whole-source evidence.

## Exact inputs

| Input | Identity |
| --- | --- |
| TUI source | `develop` `c82f69048f992ea3635f6147e6401023cefc62c3` |
| Normative TUI contract | `.governance/specs/SPEC-003-scoped-candidate-checksum-correction.md` |
| Pinned Core contract commit | `0087aa4b3c9870ac6e36769ef5632785c9411fd9` |
| Canonical policy commit | `a0384e676c1b2dbf66b915563ea70c176c09d598` |
| Policy file | `.governance/project/ga-platform-scope.json` |
| Policy Git blob | `e694c3e314c1bf11e03dc4656730a96467a765b8` |
| Policy raw SHA-256 | `159d644c161cf532c94d3bfe17ed55e32bf94c5d2843928945c450f6d8140c12` |

The policy blob and raw SHA-256 are identical in the TUI working tree and at
both pinned Core commits. The TUI verification helper refuses a source policy
that is not the canonical byte-identical reference.

The pinned Core `SPEC-008-two-os-release-evidence.md` states the relevant TUI3
inventory at line 49 and the construction/cross-repo invariants at lines 52 and
56:

- TUI3: four public files, exactly two archive-only checksum entries;
  `candidate-local-assets.json` is local transport only.
- Construction: archives, then exactly two sums records, then the manifest,
  whose unchanged `checksumManifest(name,sha256)` binds the completed sums.
- Sums: Linux archive then Windows archive in ascending ASCII filename order,
  lowercase 64-character digest, two ASCII spaces, canonical basename, LF; no
  manifest row and no sums self-row.
- Historical TUIv2: six public files and four archive-only sums rows, with its
  original Map-based reader that did not enforce the v3 ASCII row order.
- CLI7, Template3, and Core candidate5 checksum contracts remain unchanged.

## Commands executed

```sh
node --test \
  scripts/scoped-candidate.test.mjs \
  scripts/native-owner-package.test.mjs \
  scripts/native-pair-wiring.test.mjs \
  scripts/verify-candidate-publication.test.mjs \
  scripts/verify-candidate-publication-fresh-job.test.mjs
```

Result: `117` tests passed, `0` failed, `0` skipped.

Static review inputs:

- `scripts/assemble-scoped-candidate.mjs`
- `scripts/verify-scoped-candidate-publication.mjs`
- `scripts/verify-candidate-publication.mjs`
- `.github/workflows/release.yml`
- `.github/workflows/release-scoped.yml`

## Contract review

| Invariant | Evidence | Result |
| --- | --- | --- |
| Exactly four public files | `assembleScopedCandidate` writes the two scoped archives, `SHA256SUMS.txt`, and `candidate-manifest.json`; `assertPublicationDirectory` admits only those four plus the local transport inventory. The test at `scripts/scoped-candidate.test.mjs:82` asserts four public publication assets. | Conforms. |
| Exactly two archive-only sums rows | `assembleScopedCandidate` builds sums from the sorted archive assets only; `assertChecksumManifestBytes` rejects any row count other than two, any manifest row, any duplicate, any missing row, any reordering, and any digest mismatch. Tests cover all five variants at `scripts/scoped-candidate.test.mjs:123`. | Conforms. |
| Linux row before Windows row | `assembleScopedCandidate` sorts archive assets by ascending filename; the two scoped names order `linux-amd64` before `win32-amd64`; the test at `scripts/scoped-candidate.test.mjs:83` asserts the first row is the Linux archive. | Conforms. |
| Canonical sums bytes | `assertChecksumManifestBytes` requires UTF-8, trailing LF, no CR, lowercase 64-character digest, two ASCII spaces, and a canonical basename; it rejects a manifest row and a sums self-row. | Conforms. |
| Manifest binds completed sums | `assembleScopedCandidate` computes sums first and then writes `checksumManifest.sha256` from those bytes; the manifest is written after the sums. `assertActualLocalAssets` requires the manifest digest to equal the held sums bytes. | Conforms. |
| Held, private, and public byte identity | `holdVerifiedLocalAssets` holds every required local buffer before publication; `assertActualLocalAssets` compares held bytes to the manifest; `assertReleaseAssetMetadata` and the public readback compare the release inventory to the held digests and sizes. The publisher test asserts four public byte receipts and the immutable outcome. | Conforms at source level. |
| Substitution and corruption denial | Tests deny a third manifest row, duplicate/missing/reordered rows, a wrong digest, a coherent manifest and inventory replacement, a corrupt native ZIP, a substituted public manifest body, and a failed aggregate before provider access. | Conforms at source level. |
| Historical v2 remains six/four | `scripts/verify-candidate-publication.mjs` retains four targets, six public files, and four archive-only rows. The test at `scripts/scoped-candidate.test.mjs:138` reads a v2 inventory with rows in reverse order and does not apply the v3 order rule. `release.yml` still builds four archives, then `SHA256SUMS.txt`, then `candidate-manifest.json`. | Conforms. |
| Cross-repo sums contracts unchanged | The pinned Core `SPEC-008` states CLI7, Template3, and Core candidate5 sums remain unchanged. This TUI change does not modify those repositories or their checksum contracts. | No TUI-side divergence found. |
| Canonical policy is source-fixed | `assertSourcePolicy` in the TUI publication helpers reads the committed policy rather than caller input. The policy blob and raw SHA-256 match both pinned Core commits and the TUI tree. | Conforms. |

## Findings

No discrepancy was found in the TUI #29 checksum scope. The v3 path produces
four public files and two ordered archive-only sums rows, the manifest binds the
completed sums, and the historical v2 six-file/four-row path remains separately
readable with its original Map-based semantics.

Residual limitations:

- This review covers the TUI implementation and the pinned Core normative text.
  It is not the required fresh different ENTIRE cumulative CLI/TUI/Template/Core
  source review.
- CLI and Template source were not independently reviewed here; their unchanged
  status is taken from the pinned Core `SPEC-008` contract, not from a new
  whole-source review.
- No native, packaged, provider, or publication execution was performed.
- The `release.yml` v2 shell glob writes rows in filesystem/locale order. That
  is compatible with the v2 Map-based verifier, which does not require the
  prospective v3 ASCII order; it is not evidence for the v3 path.

## Verdict

`PASS` for the scoped TUI source verification of the #29 checksum correction.
This is not `SOURCE_GO` and does not close #29. The issue still requires the
independent whole-source review and governed develop acceptance described in
`SPEC-003` and the issue acceptance criteria.
