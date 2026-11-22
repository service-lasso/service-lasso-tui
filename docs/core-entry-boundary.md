# Core-owned entry boundary: use cases and status

This document explains who uses the Core-owned candidate entry boundary, when
it matters, and what remains before it can be considered production-ready. It
is not a release declaration.

## What it is

The boundary makes Core the only issuer of the initial entry for a launched
native consumer. Core launches the process, retains the attempt record, verifies
the consumer identity, sends a binding image, and binds the lease only after the
consumer acknowledges the exact image digest. The TUI validates and seals that
image, passes the views to the private native entry contract, and releases the
lease after the last borrower.

## When people use it

- **Operators launching the native TUI through Core.** The TUI must prove it was
  started by Core for this attempt, not launched manually or by another process.
- **Hosts where local spoofing matters.** PID reuse, a substituted executable,
  altered argv/environment, wrong namespace/domain, a stale launch, or a
  replayed lease must fail before effects.
- **Privileged or audited installations.** The single-use lease, exact image
  handshake, acknowledgement, and release provide an auditable startup boundary.
- **Native candidate entry.** A native owner receives Core-bound views instead
  of trusting a path, environment variable, JSON file, `/proc` reconstruction,
  or a caller-supplied constructor.
- **Release and qualification.** Reviewers can distinguish the reviewed
  production entry path from a local/manual entry.

## When it is not used

- Normal local development and the default manual TUI entry.
- A compromised TUI binary: this is not attestation or signed execution.
- General user authentication, public web access control, or service health.
- A substitute for real source/evidence ownership, ROOT admission, or
  qualification.

## Current code status

`node scripts/native-v2-readiness.mjs` declares:

```json
{
  "status": "NOT_GA",
  "ga": false,
  "declared": false,
  "authority": "release-owner-only"
}
```

Every TUI37 input group is `UNAVAILABLE` until authentic originals are
supplied. `fixtures/dev-synthetic/` is for local tests only and is rejected by
the production producer/binder unless explicitly allowed.

The local synthetic chain is implemented and tested:

```text
Core launcher -> synthetic producer -> TUI binder -> native contract -> release
```

## What remains

1. Supply the authentic authorization bundle: nine-section SLCPROF1, ABI,
   native geometry, custody originals, and toolchain originals.
2. Run the readiness gate with the real files. `ADMITTED_FOR_REVIEW` is still
   `NOT_GA`.
3. Produce the authorization bundle and entry image, then run the real Core
   launch on the matching host.
4. Complete fresh independent review, ROOT admission, exact-command approval,
   matching-host qualification, and release-owner declaration.

Until those exist, the only honest status is `NOT_GA`.
