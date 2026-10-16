# Issue1 Windows quantitative consumer layout and binding — PRE-CODE

Owner: TUI1/PR34. Product baseline2a04f0755f1c2e1b84a2b8826f7cc53654340b0f.
This is a proposed new Windows native source contract, requiring bounded intent
review before body. Existing startup exclusion checks permit finite backing
binding, but no existing Windows complete quantitative plan/charge/deadline
contract covers this unit; Linux types will not be cast or renamed.

## Selected division and concrete types to implement

TUI proposes native/owner/candidate_layout_windows.h/.c with these pure types:

- sl_win_candidate_source_span: const void *original; SIZE_T extent; category;
  actor_index and occurrence_index. It denotes SAME live source backing, never
  an issuer certificate. Categories distinguish control, query, argument, IO,
  image/provider, stack/TLS, native/kernel, deadline/custody and failure overlap.
- sl_win_candidate_layout_item: source span original reference; SIZE_T count,
  element_extent, alignment, offset, extent; fixed role enum. Count/extent come
  from the complete source-selected operand roster, never live size queries.
- sl_win_candidate_cost_item: exact source/actor/occurrence original, category,
  uint64_t resident_bytes, reserved_bytes, kernel_bytes, retained_failure_bytes;
  exact overlap lifetime original. These are independently source-qualified
  physical cost operands, not payload offsets or native allocations. Checked
  sums separately retain committed/reserved/kernel/failed-overlap subtotals;
  total financing cannot be inferred from the layout payload byte count.
- sl_win_candidate_layout_source: original ordered item roster/count, original
  complete actor/source/profile roster spans and native ABI geometry originals.
- sl_win_candidate_layout: source_original; independently retained item copies
  and copy_capacity; checked total/alignment; attempted/returned/result and exact
  first failing item. Copies/control/source live outside the described payload.
- sl_win_candidate_binding: plan_original plus retained CLI entry/operation/
  reservation/source/family/deadline associations supplied by its actual typed
  header; actual payload backing and exact item views; failed/unresolved state.
  No CLI pointer field is finalized until the actual header exists. This binding
  validates correspondence only, cannot issue finance or family authority.

Proposed pure routine sl_win_plan_candidate_layout computes checked aligned
prefix sums: pad=(-offset)&(alignment-1), require nonzero power-of-two alignment,
count*element_extent representable, offset+pad+extent representable in SIZE_T,
every source span original/exact extent and role unique where singular. No
allocation, native query, clock, callback or process effect occurs. ABI geometry
uses actual sizeof/_Alignof selected Windows types with independent source/SDK
qualification, not copied Linux constants or a JSON hash as authority.

TUI's generated consumer owns typed views of all rows below and initializes
existing fields from them before startup. CLI owns actual child-local allocation,
complete physical fit and operation/source/family/deadline acquisition. The
actual memory-owner/arena/plan/control descriptors must remain outside their own
payload; exact-live transport and caller allocations stay separate/disjoint per
CLI4fc intent. Every relevant source/operand/descriptor span is excluded from all
output writes, including failure rows. No partial binding enables startup.

## Complete known startup backing roster

The canonical native source is public_startup_windows.c startup_backing: exactly
20 backing regions plus the argument turns (21). Each becomes a distinct item;
no alias, reuse or double charge of embedded members is permitted.

| Role | Existing actual field | Extent/alignment source |
|---|---|---|
| Startup control | sl_win_public_startup | sizeof/_Alignof type; embeds controller/birth/stream/channel/duplex/argument/sender/sequence observations |
| Pipe name | duplex.name | name_capacity*sizeof(WCHAR), WCHAR alignment |
| TokenUser | duplex.token_body | token_capacity, native TOKEN_USER/SID alignment |
| ACL | duplex.acl_body | acl_capacity, ACL alignment |
| Connection observations | duplex.completions | completion_capacity*sizeof(sl_win_duplex_completion) |
| Cancellation settlement | duplex.settlements | settlement_capacity*sizeof(sl_win_duplex_settlement) |
| Private argument output | arguments.output | output_capacity*sizeof(WCHAR) |
| Module query arena | arguments.module | module_capacity*sizeof(WCHAR), complete fresh-query prefix sum |
| Module query originals | arguments.module_queries | module_query_capacity*sizeof(sl_win_module_query) |
| Mutable birth command | controller.entry.birth.command_line | command_line_capacity*sizeof(WCHAR), >=output_capacity |
| Process/job service | controller.services | service_capacity*sizeof(sl_win_service_observation) |
| Attribute list | birth.attribute_backing | source-qualified attribute_capacity and native alignment; no query-grow retry |
| Six security originals | birth.security[0..5].backing | six distinct original capacities and security descriptor alignment |
| Two token access originals | birth.tokens[0..1].backing | two distinct capacities >=sizeof(TOKEN_ACCESS_INFORMATION) |
| Endpoint security | endpoint.security.backing | independent original capacity/alignment |
| Writer observations | argument_sequence.turns | turn_capacity*sizeof(sl_win_argument_send_turn) |

Embedded writes[4]/workers[4]/spans[4] are counted inside startup control, not
another free allocation. Existing argument_sender_factory_windows.c currently
owns static factory control + turns; selected implementation must expose their
exact static extent/association OR bind source-owned per-entry turns, with one
roster location and preserved factory/exclusion checks. It must not charge an
arena turn array while silently consuming the old uncharged static array.

All capacities above require actual source operands. Existing schema3 supplies
only query capacities, moduleUnitsIncludingNUL and writer stack/turn capacity,
not the missing pipe/ACL/token-name/attribute/service/cancellation/source bounds.
Missing operands refuse; guessed defaults and successful probe sizes cannot
retroactively finance the first query.

## Additional consumer, child query and native actor roster

| Actor/region | Exact source and required coverage |
|---|---|
| Debug root | debug_root_windows.h: separate sl_win_debug_event_observation, sl_win_mapped_root_observation, sl_win_root_write (embedded readback), sl_win_debug_release; finite source-selected fresh event/observation rosters |
| Root argument and cancellation | sl_win_argument_root_source and sl_win_argument_reader_cancel (embedded writer cancellation); separate retained invocation rows and source root bytes |
| Generic profile parser | candidate_profile.h sl_candidate_profile plus original encoded bytes/section spans and five sl_candidate_record arrays; structural parser remains separate from genuine quantitative/issuer profile |
| Query regions | generated C sl_child_endpointSecurity_region, sl_child_processSecurity_region, sl_child_tokenAccess_region with exact schema3 original capacities/anchors/alignment; independent from Go backing |
| Child argument Go storage | generate-windows-query-regions.mjs: M module units; K=ceil(M/1024); fresh module arena1024*K*(K+1)/2 UTF16 units; raw32767 units, argument slots floor(32767/2), body2*(32767+M), body read original rows, UTF8 bound3*max(M-1,32766); exact emitted type geometry and alignment |
| Creator source/image Go storage | same emitter: before/after module/name/path/mapping/file/metadata/read rosters; file and mapped-memory bytes; fresh region and loaded-original rows. Existing imageGeometryBytes is only typed geometry, not complete cost/fit |
| Public/private images | actual complete public PE and private Go image, mapped/reserved regions, readonly/code/data/unwind/import/reloc spans and image/helper/provider closure; no double count file bytes versus mapped bytes without separate actual occurrences |
| Threads | initial native entry stack/TLS/loader/GS/CRT, Go runtime startup and worker stacks/TLS, at most four native argument workers as source-selected schedule allows; reservation AND actual commitment/guard/kernel thread/handle costs retained; schema3 writerStack alone is not all-thread funding |
| Native objects/IO | process/job/token/section/view/pipe/event/attribute/kernel costs; suggested_bytes[3], duplex input/output capacities, pending OVERLAPPED buffers, security/readback copies, all duplicate and inherited handle references, descendant writers and loaded-provider borrowers |
| Source/tool/helper | complete source graph, SDK/header/import/helper original spans, compiler/linker family and loaded providers, exact original environment/argv and transport readbacks, original aliases and all their lifetimes |
| Deadline and failure | CLI same child-local original origin/prior/expiration, finite observation/borrower/once-close rows, unsettled process/debug/IO/failure overlap, independent stdout/stderr and cleanup errors; unresolved storage stays charged |

This table specifies ALL known participating classes, not a numeric total or a
claim that Go/OS costs have already been measured/qualified. Complete actual
source/actor/provider operands must enumerate each occurrence and source-derived
cost before acquisition. Unknown runtime/loader/helper/kernel/Go cost is an
explicit missing operand and refuses the entire plan; it is not an omitted row,
free memory, a struct-size approximation or permission to activate.

## Binding, deadline, order and last borrowers

CLI constructor supplies genuine child-local source/reservation/operation/family,
separate effective and primary principals, exact typed memory/arena allocation,
live same-arena original deadline origin/prior/expiration and complete fixed actor
cost roster. TUI does not map a parent control pointer or reconstruct authority
from its scalar address. Parent owns earliest loader/pre-CRT interval; CLI's
native prologue/helper source establishes entry before the typed TUI consumer.

Order: verify complete immutable source/ABI roster -> compute pure layout ->
CLI acquires actual full reservation/allocations under SAME original operation ->
rejoin actual child-local source/family/deadline -> bind all disjoint typed views
and fresh native observation rows -> existing startup/source operations -> actual
own-writer root/readback and public startup gate only after all genuine predicates
-> service/capture -> genuine CLI family/IO/descendant/provider settlement ->
last borrower and once-close/retirement. Plan construction is not execution or
permission to perform subsequent arrows. Current positive path remains refused.

Windows has no TUI deadline type to reuse. Existing CLI native/host-service/
source_deadline.h defines slcli_source_deadline_original (origin native clock,
caller_original, expires) and retained slcli_source_transition_attempt (prior,
deadline/root/origin references, entry/leave/leave_locked clocks and first effect
versus publication results). Its current metadata transition ledger is not a
complete TUI scheduler. The real product-entry adapter must expose the SAME
original deadline with exact operation/arena association; it cannot relabel the
primary metadata ledger as TUI family custody. TUI proposes a finite observation
sequence that BORROWS the actual CLI source_deadline original and preowned native
clock cells via exact reviewed header, never initializes/resets a new origin,
expiry or clock. Each source-selected native operation retains before/after
observations and consumes the same operation budget. Capacity exhaustion or
expiry stops dispatch while preserving live IO/family/backing; it does not mint
cancellation, EOF, teardown or successful closure. Exact CLI deadline API and
operation/family join remain explicit supplied typed operands, not callbacks.

Every failure retains exact first original and independently attempted later
observations/captures under the existing failure discipline. Constructor failure
returns/retains the actual native owner; no finalizer/GC/wrapper abandonment or
Node child close implies settlement. Duplicate native/debugger handles remain
distinct references; event-owned handles are not double-closed.

## Source cases and review boundary

Pure layout cases: checked multiplication/padding/addition overflow, zero or
non-power alignment, missing/duplicate/foreign role/source span, source mutation,
complete disjoint descriptor/source/control exclusions, ABI type extent mismatch,
21-region inventory equality, embedded-versus-separate array accounting, query/
writer/image geometry and simultaneous failed-overlap costs.

Binding/consumer cases: wrong entry/source/operation/family or principal, copied
parent pointer, missing live reservation, changed helper/query/profile/image,
renewed deadline or origin outside same arena, hidden static factory backing,
short/exhausted IO/service/deadline rosters, partial bind before effect, late
borrower/descendant/EOF, exit before signal, duplicate close and first/later error
preservation. Synthetic cases are refusal/correspondence evidence only; genuine
constructor/source inputs and native fault matrix remain mandatory separately.

This freezes a concrete interface/layout/body proposal for bounded intent review,
not names-only product completion. Implementation/header shape may proceed only
after disposition and actual CLI typed operand roster. No target parser/import/
generator/compiler/linker/build/test/native operation was executed here. No
profile/header/hash/mock supplies native authority or release activation.
