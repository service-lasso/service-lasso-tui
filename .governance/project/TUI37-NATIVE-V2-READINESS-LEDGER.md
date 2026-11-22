# TUI37 native v2 readiness ledger — refreshed 2026-11-16

Issue: https://github.com/service-lasso/service-lasso-tui/issues/37
Base: `origin/develop` `c82f690`.
Gate: `node scripts/native-v2-readiness.mjs --profile <file> --abi <file> --geometry <file> --custody <file>`

This ledger is a mutable locator, not an immutable ROOT, admission receipt, or native evidence. A `READY` group means only that the supplied value passed the closed structural checks. It does not authenticate an issuer and does not grant execution.

## Group state

| Group | State | Owner | Evidence | Validation |
|---|---|---|---|---|
| `slcprof1-profile` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | `decodeCandidateProfile` closed header, nine sections, fixed extents, non-zero quantities |
| `go-c-abi` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | exact `goIntBits`/`cSizeTBits` in {32,64} |
| `native-geometry` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | `decodeNativeGeometry` roster, array extents, canonical quantities, `body_bytes = representationBytes`, `requests = terminalCalls + 1` |
| `constructors` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | closed custody record |
| `windows-inputs` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | closed custody record |
| `matching-host-ownership` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | closed custody record |
| `admission` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | closed custody record |
| `core-integration` | `UNAVAILABLE` | `wildone` (Max Barrass) | none supplied | closed custody record |

Owner assignment: every role is assigned to `wildone` (Max Barrass,
`max.barrass@gmail.com`) by the local owner instruction. Assignment is not
evidence and does not change any group state or authorize execution.

Disposition: `NOT_READY_NOT_ADMITTED`. No target execution is authorized.

The last custody search found no authentic SLCPROF1 original; that search was not re-run in this slice. Absence remains the recorded state until an owner supplies a reference.

## Input request packet

Supply sensitive originals through an approved private channel. Do not paste contents into chat or commit them. Publish only hashes and custody references.

| Input | Required original | Custody record |
|---|---|---|
| SLCPROF1 | complete nine-section binary profile, version 1, closed header | file digest + issuer custody reference |
| Go/C ABI | JSON `{"goIntBits":32|64,"cSizeTBits":32|64}` with tool provenance | file digest + toolchain custody reference |
| Native geometry | complete scalar/array JSON matching `decodeNativeGeometry`, including all fixed array lengths | file digest + derivation command/version reference |
| Constructors | genuine source/domain/operation/family/provider constructors plus financing/custody prerequisites | owner + evidence digest + custody reference |
| Windows inputs | authentic reviewed query profile, environment/tool/source inputs | owner + evidence digest + custody reference |
| Matching hosts | named Linux and Windows attempt owners with availability evidence | owner + availability digest + custody reference |
| Admission | fresh DIFFERENT entire-source review, NEW authentic ROOT, exact-command approval | owner + review/ROOT digest + custody reference |
| Core integration | qualified Core receiver/context and same-published-byte disposition | owner + evidence digest + custody reference |

## Custody record shape

Each non-file group must be supplied in the custody JSON as:

```json
{
  "owner": "non-empty issuer or custodian",
  "evidenceSha256": "64 lowercase hex characters",
  "custodyRef": "non-empty private custody reference"
}
```

Extra fields, empty values, uppercase or short digests are rejected.

## Fail-closed semantics

- Missing group: `UNAVAILABLE`.
- Supplied but structurally invalid: `INVALID`.
- Geometry cannot be validated without a valid profile and ABI.
- Admission requires all eight groups `READY`; otherwise `NOT_READY_NOT_ADMITTED` and exit code 1.
- A synthetic fixture that passes this gate is still not native evidence. `produceNative` remains closed.
