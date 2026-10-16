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
20 EXTERNAL backing regions plus the argument turns = 21 EXTERNAL regions.
Startup control is a separately charged 22nd region, excluded by the original
21-region checks. Both equalities are mandatory: external_count==21 and
startup_storage_count==22. Each becomes a distinct item;
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
owns static factory control plus embedded turns. Selected implementation splits
its per-entry factory CONTROL (owner reference and attempted/returned cells)
from the separate source-owned turns array. Factory control is separately
charged, beyond the 22 startup storage regions; turns remain exactly ONE of the
21 external regions, never charged again as embedded factory storage. Preserve
all original factory freshness/exclusion and source association checks while
binding the actual per-entry control/turns. The old uncharged static factory is
not a positive fallback. Factory control is not silently included in the 22 count;
the complete roster adds it and every other non-startup actor independently.

All capacities above require actual source operands. Existing schema3 supplies
query capacities, moduleUnitsIncludingNUL, writer stack/turn capacity and
creatorImages module/name/file/memory/region dimensions,
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
exact 21-external inventory equality plus separately charged startup control
(22 startup storage regions), separately charged factory control and exactly-once
turns, embedded-versus-separate array accounting, query/
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


## Selected bounded pure source implementation after acf intent GO

Fresh DIFFERENT complete acf intent review returned GO. The source body now
implements candidate_layout_windows.h/.c checked full-role geometry, separate
physical cost subtotals/aggregate, retained source descriptor snapshots and
all-source/control/copy exclusions. candidate_prepare_windows.h/.c binds only
existing native startup/debug/profile typed views over fresh disjoint backing,
with no factory/startup/native operation dispatch. Generated Go/query/helper
geometry stays on the complete retained plan; authentic compiled source/profile
and full physical/native binding remain missing operands, never inferred.

The per-entry factory control is separate from its single external turns array;
its binder preserves all original backing/freshness checks and now independently
excludes BOTH control and turns from all20 other startup backings. The Windows
source roster includes these source-only layout/preparation translations.

New C fixtures cover checked geometry/cost overflow, missing role/category/cost,
changed source descriptors, source/output alias and untouched output on denial,
typed view mismatch, command capacity, fresh backing, non-dispatched startup and
factory count/stack/turn alias/once-use. They remain UNRUN here: no matching
native compiler/test command was selected or executed. Existing source CI does
not automatically execute these native C fixtures. Actual CLI header/operation/
family binding and complete runtime/helper/kernel cost acquisition remain open;
no native issuer/profile/root/family/entry activation is claimed. Full known
source freezes for fresh ENTIRE review before new actual-input admission.


## Next selected finite borrowed-deadline source unit (pre-code)

Parent selects this narrow pure bind/revalidate part within acf finite sequence
intent. Real source dependency is CLI351929972757733ca2cfda96d634759b3c767cb1
native/initial-installer/product_storage_windows.h and its actual included
source_deadline.h/arena/native_clock/memory types. No copied local declarations
or lookalike opaque ownership descriptor are permitted. A selected explicit held
header include operand is required; no ambient checkout/include/PATH fallback.

Proposed candidate_deadline_windows.h/.c owns a finite sequence CONTROL outside
its step payload. It borrows the actual typed
slcli_product_windows_operation_caller, its SAME arena/memory/deadline original,
and the deadline's SAME already-published origin pointer/tick/expiration. The
actual slcli_source_deadline_original_matches predicate verifies live same-arena
correspondence; it is not genuine issuer/family admission. CLI storage_constructed
is not consumed as an authority boolean. No original is copied into a new
operation/clock or sampled/reset/renewed here.

Each preowned step stores one fixed source-selected actual sl_win_call native
row association, its source occurrence and finite ordinal, SAME sequence/deadline/
caller references, and fresh BEFORE/AFTER slcli_native_clock storage for the
actual later native scheduler. The original source roster is fixed compiled
consumer data with complete native-row lifetimes; it is never a callback, public
registration list, arbitrary JSON row or a helper allowed to invoke the row.
Binding checks finite exact capacity, overflow, uniqueness, typed alignment and
all source/caller/deadline/origin/control/row exclusions before any write; recheck
retains original pointers/scalars and rejects later source/backing substitutions.
No native clock read or operation dispatch occurs in this unit.

Known consumer coupling is candidate_prepare_windows: the existing retained
SL_WIN_LAYOUT_DEADLINE_CUSTODY item supplies exact count/sizeof/alignment of
these typed steps, exposed with other pure native views. Binding occurs AFTER
fresh view preparation, before genuine CLI scheduling. Sequence control stays
outside its payload. This does not enable startup or replace the stillmissing
CLI product-entry/family/native-cut/header contract. Actual full source scheduler
and entry remain owned coupled follow-on work, not supplied by pure binding.

The existing CLI source_transition_begin is explicitly not reused: it allocates
metadata attempts, samples a clock and traverses a hard1MiB metadata prior list;
that is not the fixed TUI observation schedule. Only actual published source
deadline representation/live-arena predicate is borrowed. Required new source
cases cover copied or foreign caller/deadline/origin, equal-expiry neworigin,
changed tick/expiry/source row, count exhaustion, output/source overlap, dirty
clock cells, duplicate ordinal/native row and once-use/revalidation. All remain
UNRUN source fixtures until complete actual tool/header/CRT and ROOT admission.


### Finite borrowed-deadline bounded source body

The selected source now implements candidate_deadline_windows.h/.c. Its exact
real-header operand is SERVICE_LASSO_PRODUCT_STORAGE_HEADER, supplied only by an
explicit held source-qualified include; without it compilation refuses. Reference
CLI4aa1e9526759eec0e4d4354a58186f20ecd63e48 product_storage_windows.h blob
9a59a40b4dc1b6d3a26e46be677de4f3516ccc45 includes real source_deadline.h
(a6e5fe4a637c798d72dbceb36aed753bd0e1835e), arena.h
(80bc0ff58286691e769897173bde676e3db4e19f) and native_clock.h
(6aa8575d4c477262f6b4025ed717f8502a1f7665) through its actual nested closure.
Those pins describe source bytes, never native issuer/header authority by hash.

Binding directly consumes existing candidate preparation and its typed
DEADLINE_CUSTODY payload item. It verifies the actual CLI live-arena/deadline/
clock correspondence predicates, SAME caller/memory/arena/published origin/expiry,
exact source-row type/owner/extent/ordinal links and separate live control. Every
nested initial-source/actor/occurrence/control and step/native-owner write
exclusion is retained. Raw origin comparison scalars retain history only; no
second original clock or fresh clock sample is constructed. Revalidation is not
validation of future clock cuts, native dispatch permission or family settlement.

The source fixture has no standalone main or synthetic native constructor. An
admitted future genuine harness must supply its actual paid prepared source,
original clock/caller/arena and fresh control/scratch before cases can execute.
It tests foreign/copy/equal-expiry/deadline/origin/control/source substitution,
dirty UNOBSERVED clock output, duplicate row/ordinal/type, once-use and original
revalidation without replacing native observations. This fixture is UNRUN.

Current native compile plan does not yet wire the real CLI include/link closure
or call this helper from a fabricated entry; that is precisely the next TUI
MSVC-plan/generated-consumer plus CLI genuine-entry/family coupled unit. Linking
requires actual source_deadline.c/arena.c/native_clock.c predicates and their
complete qualified helper/import/source closure, not test stubs or default CRT.
No target parser/import/compiler/build/test/native operation ran. Current source
GO is historical for f12; this complete changed bundle requires fresh ENTIRE
review and new actual-input admission before execution. Production stays refused.


## Whole84 aggregate repair pre-code after fcc NO_GO

Fresh DIFFERENT whole84 review found two P2 only; this entire repair is selected
before body. First, aligned containment is insufficient native-row binding.
ARGUMENT_ROOT has no sl_win_call and is removed from supported call kinds,
without changing that original owner type. Each remaining source row gains a
closed native-member selector plus array-slot index. A source-owned switch maps
kind to exact retained candidate-layout role, native sizeof/_Alignof and owner
occurrence; owner must equal that role's base+occurrence*sizeof(actual type).
The selector resolves ONLY an actual declared sl_win_call member/embedded member
or array element in that exact type, with exact finite slot bounds and zero
slot on singular members. Native pointer must equal that exact resolved member.
Ordinal equals finite source position. Duplicate resolved row/role/member/slot
associations refuse; aligned arbitrary bytes, wrong member/type/owner/occurrence/
slot and foreign arrays cannot pass. This remains correspondence, not authority.

The complete mapping covers startup public-stream, birth security/token/creator/
attribute calls, channel and duplex calls, private command/endpoint calls and
embedded four writer/worker slots; separate debug-event/mapped-root/root-write/
debug-release/reader-cancel/duplex-completion/settlement/module-query/service/
send-turn calls map their actual role arrays. No generic offset/callback or
lookalike native-call cell is accepted. Source role/member/slot catalogue and
meaningful aligned-nonmember/wrongmember/foreignowner/slot tests are required.

Second, the initial source declared graph did not intrinsically prove coverage
of nested requirements_per_command, subjects/reader_role, provider principalSID,
producer entry_symbol or effective/primary SID spans. CLI owns extracting the
actual complete existing product_storage source_away walk unchanged into a real
shared pure product_source_windows API, and its constructor consumes that SAME
API. TUI will remove its partial duplicate and import the actual qualified
header/API only after source exists. No forward factory, callback, assumption of
graph completeness or new native authority is introduced. Exact CLI source/header
pins follow actual extraction, not guessed declarations. Source fixtures must
exercise every nested pointee alias for sequence CONTROL and STEP output spans,
failed/overlap writes and untouched retained originals on refusal.

Both known findings repair together before the next source freeze. Genuine
entry/family/producer/native cuts and complete input ROOT remain absent; no local
execution or positive activation is selected. The stable earlier capture-error
signature remains two historical failed iterations; these are distinct reviewed
source-contract findings, not a reset or third rediscovery of that signature.


### Whole84 aggregate repair source and actual shared API pins

Both findings are repaired together in the selected source. Native-row matching
now derives the exact layout role, native type/alignment and owner occurrence,
then resolves a closed actual declared member and bounded array slot. All eleven
supported owner kinds have explicit native-member switches; ARGUMENT_ROOT is
not a call kind and its original owner struct is unchanged. Singular members
require slot0; duplicate resolved members/slots refuse. The aligned-containment
shortcut and its invalid ARGUMENT_ROOT branch are removed.

TUI's duplicated initial_away implementation is removed. Production exclusion
calls the actual shared pure CLI function
slcli_product_windows_initial_source_excludes_output. Exact dependency now is
CLI2e9ffb465d918f32c70595f7a9417212be0b0d4f:
product_source_windows.h blobcaad7861720dee3ffec157328bde8d89b353e145,
product_source_windows.c blob00bc2d96e3d3a7246a5f20e12bdd735fc9bd81cb,
product_storage_windows.h blob88bd6d4360883fee8f303d593a905e339014158a.
These files are under native/initial-installer; storage includes the real shared
header. The complete existing nested source walk is reused by both CLI storage
and TUI, with no native imports or grant. Old4aa/3519 headers alone no longer
satisfy this source/link contract; exact qualified nested source closure and new
shared .c are required, never an invented declaration/stub or hash authority.

The unexecuted genuine-input fixture now requires every supported owner kind
and iterates aligned-nonmember, wrongmember, foreignowner, occurrence and slot
adenials without mutating native owners or clocks. It tests actual ARGUMENT_ROOT
bytes as an invalid call owner, full output-byte preservation for failed binds,
and exact ordinal/once-use/source-history checks. Actual shared-API alias cases
cover requirements/subjects/providerSID/entry_symbol/everyreader_role/both
effective-primary SIDs and all declared source/actor/occurrence spans against
BOTH sequence-control and step output ranges, plus existing source pointees as
forbidden output spans. Only private metadata copies are varied; actual source
originals stay retained. Scratch/snapshot ranges are separately preowned and
excluded from the payload, control and input originals. No mocked constructor,
new native sample, execution, admission or activation follows. Complete changed
bundle freezes for a NEW DIFFERENT ENTIRE review.
