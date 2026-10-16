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
