# TUI30 pinned BubbleTea source ownership selection

State: PARENT_INTERNAL_IMPLEMENTATION_SELECTION; source work only; not SOURCE_GO, execution admission, native qualification or release readiness.

## Decision and evidence

Select a repository-owned, pinned BubbleTea v1.3.10 source fork for the complete TUI D2 implementation. Preserve the library's current public API and useful keyboard, renderer, input, signal, resize, terminal and command semantics across Windows/Linux; macOS remains owner-deferred N/A. Use a local module replacement within the sole TUI issue checkout, with exact source provenance and explicit modifications. Do not edit the shared Go module cache, publish a new module or update the upstream dependency version.

Parent directly read pinned tea.go default/explicit TTY opening and shutdown/panic paths. The actual source has deferred f.Close with discarded error, cancelReader.Close ignored, restoreTerminalState ignored, and panic diagnostic output with stack/error projection. Caller-level wrappers cannot recover those same discarded originals. Author30 identified these paths independently. Current TUI go.mod pins v1.3.10; go.sum records h1:otUDHWMMzQSB0Pkc87rm691KZ3SWa4KUlvF9nRvCICw= and go.mod h1:ORQfo0fk8U+po9VaNvnV95UPWA1BitP1E0N6xJPlHr4=. Reading these pins and local bytes is not authentication of the installed source or permission to execute it.

## Options and scope

A caller-only wrapper cannot fulfill original-error and actual internal-worker lifetime requirements where the dependency already discards those values. Replacing the terminal driver is viable but would materially recreate input/render/terminal platform behavior. The selected pinned-source fork keeps the existing library behavior and permits ownership at the actual effect sites. This is an internal implementation choice under GOV-14, within the selected complete D2 method; it does not amend public product scope, trust/persistence boundaries, deadlines or the native eligibility predicate.

## Required propagation before dependency source changes

Sole author30 must first update project intent, active spec/backlog mappings, INIT and affected strict workflow/blueprint/acceptance/lifetime ownership artifacts. Identify the repository-owned module path and exact upstream copied-source inventory, preserve the MIT license/copyright notice, record every modified source path and rationale, and carry these files into the cumulative source manifest/review closure. All supported platform source and essential module/build inputs belong to the inventory; source copying alone does not establish loaded-byte authenticity. Parent read the upstream MIT license: copyright (c)2020-2025 Charmbracelet, Inc; retain its actual notice/license in the fork.

## Actual ownership and behavior obligations

Preown the actual library input/resource constructor, partial acquisition, reader, renderer, signal/resize handlers, command/control message, terminal state, diagnostic and shutdown observations before their effects. Retain SAME actual originals/errors and actual worker/consumer joins. Do not infer absence, disposal, final archive acceptance or successful shutdown from cancellation, Kill, Run return, type/package matches or a projected joined error. Preserve selected local versus genuinely separate outer originals, failed/uncertain lifetime retention, independent caller deadline failure, native root/family/EOF and archive chronology. No raw panic/error/stack output may bypass the required diagnostic boundary. Every actual disposition must be once-only with independent eligible cleanup continuing after a different failure.

Source-owned library instrumentation must preserve command sequence/batch ordering and current application surfaces. Any remaining downstream library that discards a required actual original remains an explicit unresolved source boundary; do not claim this single fork closes all transitively loaded code by implication. Expand only where concrete source paths require it, with the same precode propagation and full review coverage.

## Rollback, review and proof

No migration or provider change is needed. The source patch can be reviewed/reverted together with the local module replacement, subject to retaining owned work/evidence. Final admission requires a DIFFERENT fresh entire cumulative independent source review including fork, application, full native/storage/diagnostic/terminal controls and known consumers, plus a NEW authentic complete input root and exact-command admission. Then actual Windows/Linux native/operator/product acceptance and exact-head natural CI must prove behavior on the resulting bytes. Protected development publication and identical published-byte integration remain separate full requirements. No current execution, signing retry, merge, publication, GA, promotion or deployment is granted by this decision.
