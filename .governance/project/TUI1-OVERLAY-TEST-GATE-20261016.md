# Issue 1: owned toolchain test execution

Pre-code mapping to SPEC001 native source ownership and SPEC004 build gates.
The Go1.26.0 overlay sources are standard-library replacements, not packages
in the product module. Give that source tree its own module boundary. Ordinary
product `go test ./...` must be accompanied by a mandatory Windows/amd64 gate
that generates the query backing from a held genuine schema3 source profile,
installs the complete existing overlay map and actually runs the matching-host
GOROOT package tests. A plan, skipped step or ordinary product pass cannot
substitute for this execution. Missing authentic profile remains a failed gate.

The executor retains exact command arrays, environment, source input hashes,
original output streams and exit results. Go must be exactly go1.26.0 on the
native Windows/amd64 host; cross-compilation cannot qualify these tests. Profile
sourceCommit must match the actual checked-out commit. Profile hash and schema
checks establish association only: the external attempt owner must admit its
genuine issuer/provider/source and containing reservation before invocation.
No fixture is eligible as CI input. No new profile issuer, public interface,
native admission or numerical reservation is created by this gate.

Both scoped and historical publisher preflights require the new check name;
their source fixtures retain all old requirements and add missing-check denial.
Provider settings are not changed. Raw profile, environment, paths, source
inputs and process output remain private in the runner attempt. Only a closed
state/source summary is uploaded; runner-local retention is not durable external
custody and must be resolved by the authentic attempt owner before qualification.

Exact1a8fca3 natural CI additionally shows legacy Windows first-save failures
without native numeric cause. Preserve the existing failure and original guard
semantics. Test-only diagnostics project existing private original numeric codes,
boundary labels and effect/release state to identify the actual failed native
boundary on the next natural run. No speculative sharing/identity change.
Linux harness fixtures must assert the genuine propagated primary exception and
its retained secondary originals. Their upload privacy check must inspect both
actual five-file public allowlists, not the entire later workflow source.

Parent coordinates the genuine source producer and ROOT admission. Source
implementation and fixtures remain UNRUN until different entire cumulative
review and new exact-command admission. Original positive native entry, complete
Windows/Linux operator acceptance, protected DEV publication and identical-byte
Core integration remain required. Existing PR34 is continued at clean 1a8fca3
under the explicit parent ownership transfer; PR33 and its indexes stay retained.
## Entire4fd8 completion and capture repair

The different full55-file review found two P2 actual gate lifetime failures.
Parent selected a bounded aggregate repair: a strongly rooted private command
owner and the same actual child/transport completion algorithm now await real
child close even after errors. Both raw captures are attempted independently;
all error originals retain occurrence order and the first remains primary.
Opaque originals stay in commandOwners, separate from serialized command
metadata. Actual child close is not native family/issuer authority.

The event-driven owned-overlay-command.test.mjs uses actual held files and
children, deliberate actual transport destruction while real stdin keeps the
child live, actual input release/close, failed spawn and real persistence
failures. It checks held inputs remain open through close, both captures run,
stderr bytes survive stdout failure and exact original identities persist.
Tests are UNRUN locally; fresh exact-head CI/review/admission remain distinct.
No timer/kill/retry/disposition, public authority, authentic profile or entry
producer is introduced by this fix.
