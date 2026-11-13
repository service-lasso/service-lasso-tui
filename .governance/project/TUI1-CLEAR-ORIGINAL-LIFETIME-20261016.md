# Issue1 Windows clear original lifetime

Pre-code mapping to SPEC001 storage/SPEC004 and selected storage effect/release
amendment. Exactd5 whole41-file independent review is bounded SOURCE_GO; no
native qualification/execution admission follows. Exactd5 source CI establishes
first Save now proceeds; three failures moved to Clear/retry. It is not full
Windows pass. Share0 and same-directory prepared-parent guards remain unchanged.

Parent selected the following internal original-lifetime refinement after
checking Microsoft's FILE_DISPOSITION_INFORMATION_EX remarks: with POSIX
semantics the visible link is removed only when the POSIX delete handle closes;
other independently opened handles can retain the original data streams.
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_file_disposition_information_ex

1. Under SAME held exclusive parent/current target and existing arbiter/journal,
   acquire a distinct read-only original via actual NtCreateFile before deletion.
   Do not duplicate the delete handle's file object. Attach the acquired owner
   before any later failure. Independently rejoin native identity, size/time,
   bounded full raw bytes and original target/parent before the delete request.
2. Retain the independent reader. Issue the existing native POSIX disposition
   on the original delete owner, then close that exact owner once. Cache its
   original close result and never query/reclose the stale numeric handle.
   A failure stays unresolved with all observations; no retry or success grant.
3. Only after successful delete-owner close, perform native absence observation
   under SAME parent and complete independent retained-reader raw/identity
   readback. The original reader survives both. Only this conjunction may settle
   clear_applied; pending/errors/presence/mismatch remain unresolved.
4. Finalization joins the cached delete close and separately closes the retained
   independent reader once, after its actual synchronous borrowers complete.
   Preserve all close/error/effect/release outcomes independently, with the
   original parent/arbiter last-release order unchanged.

Actual constructor-based source tests must bind distinct NtCreateFile versus
delete source owners, readonly access, native delete/once-close/absence/readback
order, unchanged raw original and no second stale-handle query/close. Existing
real saved-record/app retry tests remain mandatory. Fault cases use genuine
existing abstractions; do not fabricate native observations or reset production
once-state to manufacture a pass. All new cases locally UNRUN; full native
failure/custody/lastborrower qualification remains required.

The concrete new fixture also calls remove again on the consumed owner and
requires refusal with unchanged native query/close counts and SAME still-live
independent reader. Existing actual close-failure/original-cause and public
clear-error recovery cases remain unchanged. No new synthetic native success or
claimed complete native fault matrix is introduced by these source cases.

The same unit repairs the workflow source guard to interpret LF/CRLF/mixed
line endings identically and mirrors the new source-only cases into deferred
diagnostics so original step-equivalence assertions remain exact. Required
Windows/Linux+owned overlay+scoped gates remain mandatory; macOS remains N/A.
