# Issue1 selected pair-wiring pre-code proposal

Parent selected this concrete proposal for durable cross-repository intent
review under SPEC004's existing scoped native-owner packaging B. Independent
INTENT_GO at exact1f83 selected the archive/producer/assembler/verifier source
unit; its body is currently uncommitted WIP. Committed product source remains at
the independently reviewed d720250521b456c82d3923d4cf160beec3794845 baseline.
Its ENTIRE60 SOURCE_GO and hosted source-test results qualify only those exact
bytes and their bounded checks. The historical two failed capture iterations
remain recorded; this proposal creates no new execution admission.

## Actual receiving Core source, not historical worker attribution

Observed remote Core `develop` is
`caee004cd93741f617acf65b1a45b9808af686d0`. Current open Core PR1775 is
`fix/1562-client-release-core-oct16` at
`aeb850677c6fd4ac71f60ff598fc81acd6f02ebd`. The following files have identical
Git blobs in both exact pins; no Core file was changed by the TUI author:

| Current consumer | Git blob | Required source change before activation |
| --- | --- | --- |
| `scripts/operator-tool-packaging-lib.mjs` | `980b625cd9605b8cb27e3187ed08499b064bc190` | Lines75/239/315/420-477 require closed scoped asset metadata, scoped schema3, staging and retained-byte verification. Add strict schema4 member-aware validation and verify the actual pair archive bytes. |
| `scripts/verify-operator-tui-pty-scoped.mjs` | `f2951cc0b5fd26b1e1f351b8375a7f215c3a3352` | Lines40-45 extract an archive and select the public root executable. Require complete validated pair extraction/readback and retain both members through actual public-owner execution. |
| `scripts/verify-operator-tui-conpty-scoped.mjs` | `0156636bfa5b0bc8bd80fb0ffa71a069a42e7935` | Lines57-63 extract a ZIP and select the public root executable. Require the same complete pair and actual member-byte joins before ConPTY use. |
| `scripts/scoped-provider-readback-lib.mjs` | `4a3c8d37287d88cc005967de889347bc162b2577` | Default provider body limit is256MiB; preserve it for actual receiving downloads. |

The current Core schema3-only receiving contract rejects schema4; it has no
native-owner/private-payload member validator. Archive SHA256 alone cannot
replace that missing implementation. Parent coordinates the active Core1562
owner's bounded consumer unit after its current fixture protocol bundle.
Read-only discovery does not transfer or modify that owner's dirty source.

TUI native acceptance still separately pins Core
`2633c07be25512d0a84f9bfa28de6be5edff35e8` in
`scripts/scoped-native-contract.mjs`. That acceptance pin is distinct from the
current receiving consumer above. This proposal neither changes it nor claims
same-byte integration against a different Core. An explicitly selected exact
consumer pin and its actual acceptance must precede later integration claims.

## Fixed existing representation and limits

Use the existing `scripts/native-owner-package.mjs` contract unchanged:

| Platform | First member / role | Second member / role |
| --- | --- | --- |
| linux-amd64 | `service-lasso-tui` / native-owner | `.service-lasso-tui/service-lasso-tui-go` / go-payload |
| win32-amd64 | `service-lasso-tui.exe` / native-owner | `.service-lasso-tui/service-lasso-tui-go.exe` / go-payload |

Exactly two members in that order; no aliases, extras, links, alternate names,
trailing/concatenated archive data or partial inventory. Retain existing exact
ELF/PE framing, canonical archive reconstruction and both member byte digests
and sizes. Each body and the combined bodies stay within256MiB; archive cap is
256MiB+65536, with existing Linux decompression bound256MiB+4096. Existing outer
publication limits stay1GiB per asset,2GiB total and1MiB metadata; this proposal
does not increase them or weaken any Core receiving bound.
Core scoped downloads currently use boundedProviderBody's256MiB default.
Therefore a Core-consumable archive must additionally fit that actual256MiB
bound even when the TUI archive-format cap permits65536 more bytes. The
effective receiving bound is the intersection, not permission to enlarge Core's
limit or infer that every maximum-sized pair is consumable.

The same four public release filenames remain: the two versioned platform
archives, `SHA256SUMS.txt` and `candidate-manifest.json`. Keep existing ASCII
archive/checksum order, Windows/Linux scope and immutable develop candidate
identity. macOS remains N/A for this scope; historical support stays separate.

## Selected proposed TUI data flow

1. **Actual producer.** Only after genuine readiness is established, consume
   the SAME held completed public C owner and private Go payload from the real
   selected producer. Use existing createNativeOwnerArchive and
   nativeOwnerMemberInventory; rejoin both actual originals and full bytes.
   A new closed native wrapper `service-lasso.tui-native-candidate.v2` retains
   exact keys `schema,scope,source,run,platform,jobId,archive,binary,members,receipts,outcome`,
   existing source/run/platform/job/archive/binary/receipts/outcome bindings and
   adds exact ordered `members` refs `{path,role,sha256,size}`. `binary` remains
   the public-owner ref, not the private payload. This wrapper is a proposal for
   review, not an already implemented producer contract.
2. **Native evidence.** Extend strict dispatch in scoped-native-contract:
   existing v1 validates its existing single-body archive; v2 validates the
   selected ordered pair through existing assertNativeOwnerAsset, both member
   identities/digests/sizes, and the public binary receipt. Preserve exactly
   the five public receipt siblings and their privacy/source/Core/provider/
   run-attempt/job bindings. Existing Go receipt bytes cannot be relabeled as
   native-owner proof. Actual five actions, keyboard, cancellation, error,
   custody and runtime closure predicates remain required.
3. **Assembler.** Keep existing v1 to schema3 path. Only homogeneous Linux and
   Windows v2 wrappers can produce schema4; mixed wrappers refuse. Use existing
   assertNativeOwnerManifest and exact ordered member inventories. Provider
   artifact original-byte checks, complete actual input inventory and aggregate
   qualification remain mandatory; no synthetic or locally substituted body
   becomes a provider artifact. Public file count and checksum order stay fixed.
4. **Verifier and publisher.** Strictly dispatch schema3 to its existing single
   archive validators and schema4 to the existing owner-pair validators. Check
   the pair on held local, upload, draft/public original-byte readbacks and
   retained qualification paths; manifest members alone are insufficient.
   Preserve source policy, exact source/run/job provenance, native acceptance,
   four required current contexts, protected environment/branch/immutable-release
   readbacks, credential scope and public same-byte checks. No settings,
   credential, tag, release or publication action is selected here.

No automatic fallback, single-Go renaming or mixed-version downgrade is
selected. Legacy historical schema1, unscoped schema2 and scoped schema3 remain
closed under their existing separate contracts. New v2/schema4 acceptance must
not weaken any old rejection or reinterpret historical receipts as native-owner
acceptance. Tests must cover old valid/invalid schemas, mixed wrapper refusal,
member order/swap/substitution/omission/alias/trailing data, limits, exact public
binary receipt, provider artifact substitution and all local/public byte joins.

## Actual readiness and last-borrower boundary

Current TUI Windows executor is source implementation with NO connected
genuine native compiler-family/fixed-operation ownership or actual generated
entry producer. Genuine profile/provider acquisition remains CLI-owned and
unavailable here. The existing installer/compiler owner must provide its actual
entry/CRT contract and child-local original issuer/operation binding; no Cmain,
WinMain/_start, JSON/hash/dimension, path, current UID, boolean or generic
callback may substitute. The actual enclosing owner must strongly retain each
attempt and genuine family/source/profile/tool originals through all borrowers
and actual once-close outcomes. JS child exit/close, weak associations and GC
provide no native family closure or release authority.

The real Core consumer above is also not ready. Therefore no production
producer switch, schema4 publication, pair extraction activation or positive
native release qualification is authorized by this proposal. Implementation
must first receive independent cross-repository intent review and parent
selection; actual activation additionally requires the genuine entry/profile/
family route, both-host native qualification and actual Core receiving source.
No target parser/import/generator/compiler/build/test/native execution occurs
as part of this documentation change. No disposable output was created.

## Selected receipt delta awaiting separate intent review

Actual schema1 build-output.json contains only tuiCommit and nativeBinary;
it does not bind the go-payload bytes. A new wrapper inventory cannot turn
those old originals into genuine pair evidence. Parent selected this necessary
additive schema2 proposal for fresh bounded intent review BEFORE any schema2
writer/parser body or activation.

The same five filenames and privacy allowlist remain unchanged. The proposed
schema2 build-output.json is closed to these fields:

```
{
  schemaVersion: 2,
  kind: "tui20-native-build-output",
  tuiCommit: <full candidate source commit>,
  run: {id: <positive safe integer>, attempt: <positive safe integer>,
        workflowSha: <same full candidate source commit>},
  nativeBinary: {sha256: <public owner digest>, size: <public owner byte size>},
  members: [
    {role: "native-owner", sha256: <public owner digest>, size: <public owner byte size>},
    {role: "go-payload", sha256: <private payload digest>, size: <payload byte size>}
  ]
}
```

The explicit run tuple makes prior-attempt/cross-run substitution directly
rejectable by v2 validation; it is public provider provenance, not process/native
identity. No paths, process/birth identity, private actor/handle/source, host,
credential or runtime authority field is added. Every digest is lowercase
64hex, source/workflow commit40hex; size is a positive safe integer with each
member and aggregate within existing256MiB. nativeBinary equals the first
member's digest/size. Exactly two members in source-selected role order, no
extra fields, alternate roles, omission, duplication or mixed versions.

V2 wrapper MUST require this actual schema2 build original, join BOTH members
to the actual pair archive bytes and ordered wrapper/manifest inventory, and
require its tuiCommit/run to match the SAME candidate source, wrapper run,
workflow source and actual provider job/run attempt. V1 MUST require schema1;
schema1/2 receipts and v1/v2 wrappers cannot mix or downgrade. Legacy schema1
receipts remain valid for their existing closed legacy paths only. The other
four public records retain their existing exact schemas and cumulative joins.

The actual writer must observe the SAME retained public-owner/private-payload
originals under the same genuine enclosing operation/run and preserve them
through all later borrowers. It cannot derive this record by upgrading old
Go-only receipts, copying wrapper hashes or accepting declared JSON members.
Schema2 shape/digests alone establish no genuine acquisition, issuer, family
closure, finance, compiled entry or qualification. The new-production path
stays explicitly refused without its real constructor and accepted Core
receiver; no v1 fallback, public flag, boolean or callback opens it.

Required delta negatives include old schema1 receipts under v2, schema2 under
v1, missing/extra fields, reversed/duplicate/foreign roles, either member
substitution, nativeBinary/first-member mismatch, aggregate/member limits,
prior run/attempt/workflow/source, and coherent raw wrapper/provider/receipt
rebinding that must still fail the actual member/source/run bridge. No huge
allocation or fake native issuer test is selected. Receipt parser/writer body
remains deferred until this exact delta's independent intent disposition.
