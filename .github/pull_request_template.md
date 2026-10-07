## Scope

Link the issue and specification sections implemented.

## Evidence

- [ ] Formatting, focused tests, and build ran locally
- [ ] Every intentional commit is pushed
- [ ] Hosted CI status is recorded
- [ ] Core dependencies and unverified claims are stated

## Safety

- [ ] No credentials or raw sensitive values are included
- [ ] The pull request targets `develop`

## Issue1 complete client contract unit A (8 October 2026)
Active Development source unit, based on TUI effd6a69 and authentic current Core
ce56f592 / pinned Core2633. Issue1 remains OPEN; issue20 is the five-action dependency.
TUI-CLIENT-A1 (F1/F2): require versioned accepted/read/cancel envelopes, top-level
operation and cancellation result/terminal, own actor, exact operation/action,
closed status/outcome consistency, and bounded unique target IDs. Core initially
claims the requested singleton then replaces target IDs with authoritative
preflight/progress targets; admit the requested singleton or subsets of the
frozen preview containing the requested service, never unrelated targets.
Retain that frozen preview in memory only. Restoration has no persisted preview:
first authoritative own read establishes the immutable action/target tuple and
later reads bind it; never infer request identity from a list or time.
TUI-CLIENT-A2 (F3): typed closed stage errors; never render raw transport, decode,
filesystem, config or startup errors. HTTP diagnostic keeps only method, route
class and canonical status. Response ceiling 1 MiB, depth16, collection1024,
string4096 bytes, duplicate keys/trailing JSON/invalid UTF8 denied before typed
decode. Service list<=512, inbox<=20, history<=256, targets<=100.
TUI-CLIENT-A3 (F5/F6/F10): availability v1 requires exact selected service,
all five supported actions plus unavailable reload, complete typed authority,
unique known actions. Current update_check/update_download pair is admitted
only as unsupported metadata, never displayed as supported or submitted.
Safe reason projection uses fixed labels; unknown reason is unavailable without
rendering its value. Selected history/availability bind epoch, service, request
generation, reset/loading state. Refresh results bind generation, snapshot age
is visible, post-terminal refresh reads authoritative service state once.
Recheck availability at confirmation submission without replacing the preview.
TUI-CLIENT-A4 (F7): lost submit response before an ID is explicit uncertain,
no-ID state: refresh cannot read an empty ID or replay any mutation. No key,
request or confirmation persistence; server correlation/recovery remains Core
work. Known-ID reads and cancellation retain the original client and validate
server context when available. Context changes fail closed.
All actual-wire/model/storage/startup regression SOURCE remains LOCAL UNRUN
until DIFFERENT ENTIRE cumulative SOURCE GO and NEW complete-input ROOT plus
parent admission. F4 keyboard viewport/focus/Unicode, F8 full forms/workflows,
F9 shared native fixture, direct Windows/Linux five actions/cancel/recovery,
protected immutable four-file/two-archive publication and same-byte Core/CLI/
template integration remain subsequent required scope. Mac Deferred/N A never
PASS; B0/B2 UNKNOWN and full Node plus finite measured-budget human choice remain.