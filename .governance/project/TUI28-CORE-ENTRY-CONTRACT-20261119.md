# TUI28 Core entry contract (proposed, 2026-11-19)

Status: source contract partially implemented. Core now owns the launch, image,
lease-failure, and handoff path; the TUI has a consumer binder and a private
`sl_core_initial_entry_*` source contract that refuses when authentic originals
are absent. Native product wiring, ROOT admission, and execution remain gated.
This document grants no native execution, ROOT, admission, or publication
authority.

## Decision

Core is the sole native entry owner and authority. CLI and TUI are consumers:

- Core acquires the real launch/startup occurrence, containing reservation,
  source/domain, issuer-to-operation relation, deadline root/sequence, and
  custody originals.
- Core creates a per-attempt opaque entry lease.
- Core launches the consumer and passes only that lease through an inherited
  handle.
- CLI renders Core results; it does not construct native operands.
- TUI receives the Core-bound entry record and passes it to
  `sl_candidate_owned_entry`; it does not acquire or invent operands.

A consumer launched without a Core lease has no entry and must refuse before
effects.

## Non-goals

This contract does not:

- add a public C ABI, CLI flag, environment variable, JSON file, or path-based
  authority;
- let CLI or TUI create a containing charge, domain, issuer, deadline, or
  reservation;
- treat hashes, dimensions, pointer equality, process identity, or a lease
  shape as issuer authority;
- require or permit a fabricated `main` for link success;
- replace authentic inputs, separate ROOT admission, or matching-host native
  proof.

## Opaque lease

A lease is a Core-owned, non-transferable record for exactly one attempt.

It binds at least:

- lease magic/version/size and Core attempt identity;
- consumer PID, process start identity, executable identity, and cgroup/domain
  identity;
- exact `argc`/`argv`/environment bytes and the selected public entry identity;
- the Core reservation, containing source/domain, issuer, operation, and
  expected deadline;
- issue/expiry times and a single-use state.

The lease is not authority by itself. Core retains the original reservation,
issuer, source, operation, and custody objects. The lease only identifies which
Core originals the binder may materialize for this consumer.

## API surface

The following methods are conceptual and versioned. Exact transports, names,
and codecs are selected during Core implementation; callers must use the
versioned contract, never a fallback.

### `entry.launch.v1`

Core-only operation. CLI or another authorized view requests a launch; Core:

1. verifies the reservation, issuer, operation, and requested command;
2. creates the lease and retains every original;
3. performs the real exec/launch with the lease handle inherited;
4. retains the child and original launch observations.

There is no consumer-side equivalent that adopts an existing process.

### `entry.bind.v1`

Consumer-side call through the Core binder. Input: inherited lease handle plus
the consumer's own observed PID, process start identity, executable identity,
`argc`/`argv`/environment, and current domain. Core returns either:

- a bound entry record containing exact consumer-local views of the Core-owned
  charge/source/domain/actors/bases, real public inputs, deadline source/root/
  sequence, issuer relation, and custody handles; or
- a typed refusal, with no partial authority.

The binder is Core code. It may materialize consumer-local views from immutable
Core originals, but it must not copy authority. A copy, substituted mapping, or
relocated pointer that is not proven against the Core original is a refusal.

### `entry.observe.v1`

Read-only: returns Core's original launch/reservation/issuer observations for
diagnostics. It never returns a new charge, issuer, deadline, or permission.

### `entry.release.v1`

Releases the lease and consumer-local bound views after the last borrower. It
does not release Core's original reservation/custody until Core's own last
borrower and terminal checks complete.

## Consumer sequence

CLI:

1. Calls `entry.launch.v1` with the requested command and reservation.
2. Does not link native acquisition code or construct operands.
3. Displays the returned status/result; it never turns a missing lease into a
   local entry.

TUI:

1. Starts only with an inherited Core lease handle.
2. Calls `entry.bind.v1` before any native effect.
3. On refusal, exits nonzero without creating a plan, charge, deadline, or
   native operation.
4. On success, passes the bound record directly to
   `sl_candidate_owned_entry`:
   `charge`, actual public inputs, deadline source, constructed root, and
   enrolled sequence.
5. Preserves every borrowed original through public capture, parent supervision,
   and last use.
6. Calls `entry.release.v1` once after the last borrower; no fallback release,
   retry, or local custody claim.

No PID/argv/env/path lookup, `/proc` reconstruction, local clock construction,
or profile parsing may appear between bind and `sl_candidate_owned_entry`.

## Lease binding rules

`entry.bind.v1` must verify all of the following against Core's retained
originals:

- the lease is live, single-use, unexpired, and issued for this attempt;
- the consumer PID/start/executable/domain match the launched occurrence;
- the observed public `argc`/`argv`/environment bytes match exactly;
- the bound charge/source/domain/actors/bases are the same Core originals;
- the issuer-to-operation relation is the same Core relation;
- the deadline source/root/sequence are the same constructed/enrolled
  originals, not storage bindings or fresh clocks;
- every borrowed handle is the same original, with a complete lifetime.

A lease cannot be renewed, rebound to a second consumer, reused for a second
attempt, or elevated by a later CLI/TUI call.

## Required Core-held operands

Core must hold through the consumer's last borrower:

- real initial stack, `argc`/`argv`/environment, auxv, `execfn`, loader/CRT and
  pre-main costs;
- containing source graph/domain/items and every actor/basis original;
- real reservation/charge rows, available bytes, page size, copies, and
  capacity;
- issuer, operation, domain, and provider/family relation;
- deadline source/operation/duration, constructed root, enrolled sequence, and
  observations;
- installed/public image, held files, memfd/custody, readbacks, and failure
  prefixes.

Observations from `public_startup_linux.c`, `local_installation_linux.c`,
`creator_file_linux.c`, `creator_elf_linux.c`, or the CLI initial-installer
stack are building blocks. They do not by themselves establish the reservation
or issuer relation.

## Refusal matrix

`entry.bind.v1` and `entry.launch.v1` must fail closed on:

- absent, malformed, expired, replayed, or already-bound lease;
- consumer launched outside Core or by a different attempt;
- PID reuse, wrong start identity, wrong executable, or wrong domain;
- any `argc`/`argv`/environment byte change or alias;
- missing/mismatched reservation, source, domain, actor, basis, or issuer;
- substituted charge/copies/capacity, foreign deadline root/sequence, fresh or
  unenrolled clock, or storage binding presented as a constructor;
- borrowed-handle substitution, duplicate ownership, or use after release;
- any attempt to derive authority from a path, flag, env string, JSON field,
  hash, digest, Boolean, or caller-supplied numeric value.

Refusals preserve the original failure and produce no partial entry.

## Implementation order

1. Core defines the versioned contract types, lease state machine, refusal
   codes, and source tests.
2. Core implements real launch/exec acquisition, retained originals, and lease
   issuance.
3. Core provides the private binder that materializes exact consumer views and
   proves every binding.
4. CLI becomes a view over `entry.launch`/`entry.observe`; no native
   constructors.
5. TUI adds a product `main` that only binds and calls
   `sl_candidate_owned_entry`.
6. Focused refusal tests run first; positive native execution waits for
   authentic inputs, separate ROOT admission, and matching-host qualification.

## Non-claims

This contract is not fully implemented. It is not a closed native ABI, a native
entry, or evidence of issuer authority. The current generated C still has no
product `main`; `nativeEntrySupplied:false` and `authenticatedIssuer:false`
remain truthful until the product path and its genuine inputs exist.

## Source implementation status (2026-11-20)

- Core image/lease/launcher: `service-lasso/src/contracts/candidate-entry-binder.ts`,
  `service-lasso/src/contracts/candidate-entry.ts`, and
  `service-lasso/src/runtime/entry/candidate-entry-launcher.ts`.
- TUI consumer: `internal/coreentry/` validates the Core image, seals it into a
  read-only memfd, builds offset views, and acknowledges the exact digest.
- Core-bound product entry: `cmd/service-lasso-tui/core_main_linux.go` under the
  `service_lasso_core_entry` build tag requires the inherited Core handoff,
  refuses an absent channel, and records the Core entry in local app/API
  lifetime. The manual entry remains the default.
- Private link-in contract: `native/entry/sl_core_initial_entry.h` and
  `native/entry/sl_core_initial_entry.c`. Acquisition returns
  `SL_CORE_ENTRY_RETAINED_UNAVAILABLE` unless a non-null authentic Core
  originals handle is supplied. No synthetic originals are accepted.
- Native build: `scripts/native-owner-build-plan.mjs` includes the private
  contract source and `native/entry` include path.
- Go bridge: `internal/coreentry/native_cgo_linux.go` passes the sealed views to
  the private contract under cgo; `native_nocgo_linux.go` refuses when cgo is
  unavailable. The Core-bound entry requires `NativeAcquired` before startup.
- Refusal fixture: `native/entry/sl_core_initial_entry_test.c`, run by
  `scripts/core-entry-c-contract.test.mjs`.

Still not implemented or admitted: authentic Core originals, the production
Core binder adapter, native execution, ROOT admission, qualification, and
publication.
