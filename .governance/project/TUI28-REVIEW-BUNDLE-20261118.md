# TUI28 review bundle manifest (provisional, 2026-11-18)

Status: provisional owner-side manifest. It is not independent review, SOURCE_GO,
ROOT admission, native proof, qualification, or publication.

## Source pins

| Role | Repository | Commit | Tree |
| --- | --- | --- | --- |
| TUI base | `service-lasso/service-lasso-tui` | `f0337e315506ae154f02f8874bce7fc3cafc1cc5` | `45ee624104fb658ddf38fc87ae8d5d93f37c7efa` |
| Core SPEC-008 authority | `service-lasso/service-lasso` | `0087aa4b3c9870ac6e36769ef5632785c9411fd9` | `33d5b7f94b61958562b4725086443caf6b5174e4` |
| Core native runtime pin | `service-lasso/service-lasso` | `2633c07be25512d0a84f9bfa28de6be5edff35e8` | `2178666c6ea0fde729a1499a59d74db881e02eb7` |
| CLI reference only | `service-lasso/service-lasso-cli` | `f8042bf0d325d01c9b4389446c16774a2ca76ac1` | `fb2ac9f44cadfb738a0c86a3ecd306d31cc6b895` |

The CLI row is a reference, not a frozen candidate constructor. No closed
Go/C ABI constructor or genuine entry/provider/family/finance custody source is
currently pinned.

## Canonical policy

- Source: `service-lasso/service-lasso` commit `a0384e676c1b2dbf66b915563ea70c176c09d598`
- Path: `.governance/project/ga-platform-scope.json`
- Blob: `e694c3e314c1bf11e03dc4656730a96467a765b8`
- Raw SHA-256: `159d644c161cf532c94d3bfe17ed55e32bf94c5d2843928945c450f6d8140c12`

## New source boundary in this bundle

- `scripts/native-input-bundle.mjs`
  - Working-tree SHA-256: `7bb6d1b5ba66aa48a64f1c8b8b62aea44afb89534b894bb13065a98fbabac456`
- `scripts/native-input-bundle.test.mjs`
  - Working-tree SHA-256: `1f4a73a2619a178b2cd9ba3a225314c809b107c71e3140c918a635df6bc4d6cf`
- `scripts/native-owner-build-plan.mjs`
  - SHA-256 at `edb0831`: `2f15a15a2d3713894c9e5cbb3ad3bda8a10da26ed0675e511f6bdcfeb4721348`
- `scripts/linux-candidate-executor.mjs`
  - SHA-256 at `edb0831`: `affa7e7719cbf21752842edf2fccdefa327023175d1f3c610ad7ed57244a1c60`
- `scripts/linux-candidate-build-plan.test.mjs`
  - SHA-256 at `edb0831`: `3d6d7efc75a022e5eb4f5dc2949e832c9b15f02ea3448900756a6635401771de`
- `scripts/candidate-profile-source.mjs`
  - SHA-256 at `063b7d8`: `19f32f2baed0265de068b43d1b1cd89dca053ad094548376c115d5ea2ecfb0b8`
- `native/owner/tests/generated_owned_entry_linux_test.c`
  - SHA-256 at `063b7d8`: `ffbfb822761f2eaaf15d82c952974d861971a5f848b2b4db525a4ffec31332b9`

These hashes are provisional until the final TUI commit is created. The module
defines a closed `service-lasso.tui-native-input-bundle.v1` record, denies
unknown fields, duplicate JSON keys, path traversal, symlinks, aliases, foreign
hosts, and foreign source commits, and derives profile/geometry quantities from
the actual `SLCPROF1` and geometry originals rather than trusting manifest
numbers. It also requires an explicit `classification` (`authorization` or
`development-synthetic`), exposes `requireAuthorization` so a production caller
cannot consume a development bundle, and provides a writer that verifies every
original before writing a non-overwriting bundle.

## Existing authorities

- `.governance/specs/SPEC-004-scoped-tui-candidate.md`
  - SHA-256: `654b22921b5e69dce76f0ae09fae852996305b94956ce0fe1442969c1d402dd0`
- `.governance/project/TUI37-LINUX-ACQUISITION.md`
  - SHA-256: `614911984564248eb8e1c037c859ef60bacf47a9ba7d5d0e00cb0beff95050dd`
- `.governance/project/TUI28-CORE-ENTRY-CONTRACT-20261119.md`
  - SHA-256 at `f0337e3`: `4df988405ca2cd76d982b33f78b5518be7e0b939c6a31bb11c2dfdfddfec2120`
- `.governance/project/ga-platform-scope.json`
  - SHA-256: `159d644c161cf532c94d3bfe17ed55e32bf94c5d2843928945c450f6d8140c12`

## Durable input producers

Three source-only producer CLIs now exist:

```sh
# Nine section files -> canonical SLCPROF1. The spec is a closed JSON object
# with {"sections":[{"id":1,"path":"sections/01.bin"}, ...]}.
node scripts/produce-slcprof1.mjs /abs/sections.json /abs/profile.slcprof1

# Profile + ABI + geometry source -> canonical geometry JSON.
node scripts/produce-native-geometry.mjs \
  /abs/profile.slcprof1 /abs/abi.json /abs/geometry-source.json /abs/geometry.json

# Record + original files -> non-overwriting native input bundle.
node scripts/produce-native-input-bundle.mjs \
  /abs/native-inputs.json /abs/originals /abs/bundle --authorization
```

`produce-slcprof1.mjs` rejects traversal, symlinks, duplicate/missing sections,
and existing output. `produce-native-geometry.mjs` rejects a geometry roster
that does not join to the profile quantities and ABI. `produce-native-input-bundle.mjs`
rejects non-leaf names, symlinks, missing originals, output collisions, and
non-authorization records when `--authorization` is set. All write mode-0600
outputs and preserve the existing closed validators. They produce durable
artifacts from supplied originals; they do not invent evidence.

## Exact source-review commands

Use the cached Node 22.23.2 binary already used by the source suite:

```sh
NODE=/home/maxbarrass/.cache/tui37-tools/node-v22.23.2-linux-x64/bin/node
$NODE --test scripts/native-input-bundle.test.mjs
$NODE --test scripts/scoped-candidate.test.mjs scripts/native-owner-package.test.mjs scripts/native-pair-wiring.test.mjs scripts/verify-candidate-publication.test.mjs scripts/verify-candidate-publication-fresh-job.test.mjs scripts/native-input-bundle.test.mjs
```

The current focused run is 162 tests: 161 pass, 1 Windows-only load test
skipped on Linux. This is bounded source validation, not native proof.

## Development-only Linux probe (2026-11-18)

A `/tmp` `development-synthetic` bundle was written from owner-authored evidence,
canonical policy bytes, TUI HEAD `c82f69048f992ea3635f6147e6401023cefc62c3`,
and actual Go 1.26.0 / Node 22.23.2 / GCC 15.2.0 originals. It is not
authorization and is not a durable review input.

The first Linux executor run passed source head/clean, matching Go/Node checks,
paired source emission, Go candidate tests, and the Go payload. Compilation then
found real source defects:

- `native/owner/final_capture_linux.c` used `sl_linux_fd_lift.original_close`;
  the actual member is `close_original`.
- The `avtab_original->htable` / `table_original->htable` conditional branches
  needed explicit `const void *` casts.
- `native/owner/failed_parent_directory_linux.c` used `INT_MAX` without
  `<limits.h>`.
- Vendored `third_party/libsepol/src/private.h` used `typeof` in a `-std=c11`
  build; `__typeof__` preserves the GCC/clang extension.

After those fixes were committed at `7a4ea99240048d732874e363c6516ba804b49921`,
a clean-tree executor rerun passed source head/clean, the matching toolchain
check, paired emission, Go candidate tests, the Go payload, all owner and libsepol
compiles, and the emitted `candidate-entry.c` compile. `link-native-owner` then
failed only with `undefined reference to main`, matching
`nativeEntrySupplied:false`. A temporary `/tmp` `int main(void){return 0;}`
link probe linked every non-test object with no other unresolved symbols; it is
not a native entry and was not added to the tree. The Linux plan now also
compiles and links the existing generated-owned-entry refusal fixture against
the SAME generated C and all non-test owner/libsepol objects. The fixture is a
compile/link-only gate and is not executed; its test `main` does not supply the
product's missing genuine installer entry. This is GCC source validation, not
the selected clang toolchain, native execution, ROOT admission, or PASS.

On 2026-11-19, commit `edb08312452c9988c510100f1082c580d3714d88` added that
compile/link-only gate to the Linux build plan and executor. A clean-tree rerun
using `/tmp/tui28-inputs-syDmz2` (version `2026.11.19-edb0831`) compiled and
linked `objects/generated-owned-entry-fixture`, then reached
`link-native-owner`; the product link again failed only with
`undefined reference to main`. Attempt root:
`/tmp/tui28-attempt-1795008333295251625`. The bundle and attempt remain
development-synthetic; neither is authorization, ROOT admission, or native proof.

Commit `063b7d8997f94e13687497fcae8f2bb546ad1fe6` adds the static
`sl_candidate_owned_entry` composition seam. It calls the existing
`sl_candidate_prepare_owned_entry` and `sl_candidate_enter_owned_outer` helpers
in order, refuses without operands, and adds no acquisition, public ABI, or
entry authority. A clean-tree rerun using `/tmp/tui28-inputs-LV4ewl` (version
`2026.11.19-063b7d8`) compiled and linked the fixture including this seam, then
failed product `link-native-owner` only with `undefined reference to main`.
Attempt root: `/tmp/tui28-attempt-1795010229681724780`.

Commit `f0337e315506ae154f02f8874bce7fc3cafc1cc5` adds the proposed Core-owned
entry contract. It fixes Core as the sole acquisition/authority owner and makes
CLI and TUI consumers of a per-attempt opaque lease plus a Core binder. TUI's
eventual `main` may only bind that record and call `sl_candidate_owned_entry`;
it may not acquire or invent operands. The contract is unimplemented, has no
closed ABI, and changes no native source or proof status.

## Still missing before issue #53 can begin

- Final TUI commit/tree for the reviewed source.
- Frozen CLI/ABI constructor commit and its exact file inventory.
- Authentic complete `SLCPROF1` original.
- Native geometry original whose `body_bytes` equals the profile
  `representationBytes`.
- Reviewed Windows query profile.
- Genuine entry/provider/family/finance custody originals.
- Named host/toolchain owners and Windows native acceptance.
- Independent ENTIRE source reviewer and separate ROOT admission authority.

macOS/Darwin remains deferred/N/A and is never PASS.
