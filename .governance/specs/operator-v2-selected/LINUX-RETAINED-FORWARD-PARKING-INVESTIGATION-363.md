# Linux unresolved forward-thread parking investigation 363

Status: source investigation only; no implementation, selected method, native
result, review admission or ROOT. Whole TUI scope remains outstanding.

Actual forward_workers_linux.c retains its failed thread and blocked SIGPIPE
mask by one recorded FUTEX_WAIT_PRIVATE, followed after any return by an
unbounded atomic_signal_fence spin. Replacing this with thread return would
lose the original unresolved thread/mask and is not a repair. Retrying into the
same observation would overwrite the first native result and is also invalid.

A separately preowned anonymous blocking pipe is a concrete parking candidate:
keep both ends under the original native owner's custody; the worker issues a
nonzero read from its empty read end; no own actor writes, closes the last
writer, changes file status flags or transfers aliases. A successful full
catchable-signal block on that SAME worker must precede this read and retain its
own original mask/return observations. The earlier SIGPIPE episode remains
separate. SIGKILL/SIGSTOP cannot be blocked; task memory/control boundary remains
unselected and is not inferred from the selected filesystem boundary.

Fresh upstream source inspection:
https://raw.githubusercontent.com/torvalds/linux/master/fs/pipe.c
anon_pipe_read lines318..448 distinguishes zero-count success, absent writers,
nonblocking error and wait_event_interruptible_exclusive. pipe_readable at283
checks actual data or no writers. The wait macro in
https://raw.githubusercontent.com/torvalds/linux/master/include/linux/wait.h
579..590 uses TASK_INTERRUPTIBLE and schedule, with condition rechecks in the
wait machinery. Thus it can sleep inside one recorded native invocation while
empty with a held writer, rather than requiring repeated user-space calls.
This moving upstream source is investigation evidence only, not a selected or
qualified Linux kernel source/build.

Required before body selection: exact selected-kernel signal/wait/restart source
closure (including stop/continue), genuine pipe creation/fcntl/mask originals,
FD allocation and all dup/fork/inherit/transfer/close aliases, thread-specific
mask custody, fixed held read buffer, stack and kernel backing capacity, actual
first-use ordering before worker creation, and failure paths for every setup
operation. A bad FD, unexpected data, writer loss or failed mask operation must
retain its genuine return and cannot silently claim parked state. No arbitrary
retry budget, new file/path premise or closed unresolved mask is granted. This
candidate is not sufficient while its own error-after-read disposition is
unfinished. It also does not settle normal forward return/native join or other
Windows/Linux/public/LOCAL/FINAL/archive obligations.

## Additional actual source relations

The same broader retained-runtime debt exists before exec: owner_linux.c
sl_linux_retain_bootstrap records one SYS_pause then spins with x86 pause.
A forward-worker-only repair cannot claim whole Linux failure disposition.
The bootstrap runs after clone3 before libc/Go and may fail before, during or
after its FD action map; any parking descriptor would need survive that exact
partial action prefix without adding an unreviewed child inherited view. The
worker candidate above is therefore not automatically applicable to bootstrap.

Final capture also owns the representation of these originals. Current
final_capture_linux.c final_forward enumerates exactly seven native-call
originals (six signal operations plus held_wait), forwards original disposition
bytes, and serializes hold_word among signal state. A real parking source change
must update actual archive occurrence, capacity counting, independent reader
schema and alias/last-borrower relations together; leaving the old seven-call
roster would omit genuine new mask/pipe/read operations. The bootstrap final
capture roster similarly includes failure_pause. No wire-only row can create
missing native originals or authorize thread/mask/descriptor closure.

Actual read cuts SHA-256 (source only, no execution):
- native/owner/forward_workers_linux.c: 16D5FCBD385406E8A3368E3862D5875453B97931509E8176F0F356F083E8DC5A
- native/owner/signal_linux.c: B1B9E0AD4BAA2FE69DA4D2628189B29BE8E49DD2A6F6F1743AB8C1E96F93669B
- native/owner/owner_linux.c: 82A72489DC839F9F4CD76F54E71B44A17404C4DF3A871630329EB589A85D3227

The source mapping above used entire forward worker and signal source/header,
owner_linux.h entire, owner_linux.c first115 lines and final_capture_linux.c
bounded final_forward lines around742..779. It is not an ENTIRE final archive or
ENTIRE owner_linux.c review claim.

## TUI31 external memory/control choice now selected

2026-10-11 parent record SHA9E9562E9C65B2B9396B92F96256D98E3DC0D6863E61CEDFF9E75FE6CDA0E91E2 and referenced proposal SHA283247EEFB84F42E861E2E8DAED5E09BDC1EA14E3215EA1C7CAFB11F32769A29 were ENTIRE read. The previous pending-choice statements in this investigation are historical only. External creator proof/tracee relevant memory/register/control nonmutation is selected over actual first-observation→last-borrower windows, including preexisting writable capabilities/maps and injected unobserved writers. Unrelated memory and mutable application DATA remain excluded. All own native/kernel/worker/loader/signal/descriptor/mapping/descendant writers, outputs and true last borrowers remain actual obligations. Thus this choice does not prove the proposed parking read cannot return or settle any pending signal/mask/FD operation. Canonical intent/backlog/spec/INIT/matrix/blueprint/acceptance now map the exact selection before dependent bodies. No execution or method GO.

## 363a: zero-descriptor raw ppoll alternative

Investigate one raw SYS_ppoll(0,0,0,&fixed_full_mask,8): no pipe/keeper descriptors,
no separate signal-mask syscall. Moving upstream fs/select.c1023–1043 installs
the temporary mask before polling. do_sys_poll911–950 uses stack storage and
skips FD copies/allocation when nfds0. do_poll826–907 has no FD events/busy-loop
candidate then sleeps; NULL timeout supplies no expiry. This is not selected
installed-kernel proof. Primary source: https://raw.githubusercontent.com/torvalds/linux/master/fs/select.c

Still prove exact selected x86_64 syscall/mask ABI, set_user_sigmask saved-mask
ownership, fatal/stop/restart/user-return paths, admission/seccomp/provider,
preowned readable mask and ALL own writers. A denied or unexpectedly returned
call remains a real original failure, not permission to retry/spin/exit. Preserve
existing signal originals and retained runtime. Apply distinct actual calls to
forward-worker and preexec-bootstrap roots; update their schema/rosters and
independent reader together. Kernel stack/scheduler/pending-call lifetime and
last borrower remain charged. No body, method GO or native fit follows.

Moving upstream kernel/signal.c3133–3147 set_user_sigmask rejects wrong size or
unreadable storage before installation, then saves the actual current blocked
mask in current->saved_sigmask and installs the temporary mask. The setter
removes SIGKILL/SIGSTOP. Thus no source claim may say those two are masked, or
that a prior failed SIGPIPE query supplied the real saved mask. Kernel-owned
saved-mask/restore-flag lifetime and actual stop/continue/fatal/signal-return
behavior must join the genuine selected-kernel proof. A logical pending raw
call, kernel rescheduling/restart, and a real returned user-call observation are
distinct; do not fabricate returned cells for internal scheduler wakeups.
Primary research: https://raw.githubusercontent.com/torvalds/linux/master/kernel/signal.c

Moving upstream x86 signal.c309–339 restarts ERESTARTNOHAND without a handler
by restoring orig_ax and rewinding IP to the syscall instruction, then restores
the saved mask. With a delivered handler,237–261 instead maps that restart case
to EINTR. Thus stop/continue or other signal paths cannot be called impossible
merely from a full temporary mask: SIGSTOP/SIGKILL remain unmasked, and restored
original masks/pending signals require the complete exit-work/own-control graph.
The selected external-control premise narrows external writers only; own group
freeze/stop/signal/recovery actors still need their exact relation. No user retry
loop is proposed, and kernel restart is not a fabricated second returned result.
Primary research: https://raw.githubusercontent.com/torvalds/linux/master/arch/x86/kernel/signal.c

## 363b: actual current own signal/control edges

Fresh non-test native/owner source search found no kill/tgkill/tkill/
pidfd_send_signal/pthread_kill/PTRACE/cgroup.freeze operation. This is a bounded
source finding, not a provider/descendant exclusion proof. Actual signal_linux.c
queries disposition, blocks SIGPIPE, observes pending bits and conditionally
restores its original mask. Actual forwarding starts only after begin succeeds;
therefore the worker's own pipe write can generate SIGPIPE only after the block
succeeds. A failed begin reaches retention before that forwarding write.

Actual terminal_linux.c raw TCSETS clears ISIG; restoration reinstates the
original termios. Original pre-raw public TTY foreground-group keyboard signals
remain real public semantics, not hostile external actors removed by selection.
The TIOCSWINSZ private-PTY copy also has an actual kernel SIGWINCH/group relation.
Close child setsid/foreground group, owner public original session, input
forwarding and mode restore timing against genuine kernel borrowers. Own
libc/pthread/provider cancellation and any future recovery/control source must
be enumerated from actual selected code. These edges narrow the investigation;
they do not qualify unexpected early ppoll return or settle an unknown mask.

## 363c: persistent full-mask plus raw rt_sigsuspend candidate

Source construction proposal only, not selected or activated. Apply separately to failed forward-worker and preexec-bootstrap retained-runtime occurrences. Before suspension, one distinct raw rt_sigprocmask(SIG_SETMASK,&source_full_mask,&actual_original_mask,8) installs a persistent mask of all catchable x86_64 signals and captures the original. After actual successful zero return, one distinct raw rt_sigsuspend(&source_full_mask,8) parks. No descriptor, inherited view, retry, user spin, signal-disposition change or deadline renewal is introduced. Retain both calls, raw parameters/results, mask bytes, real issuing-thread identity, original SIGPIPE episode, stack and all native owners. Normal successful forward restore/return/join remains separate.

Fixed upstream research v6.18 kernel/signal.c3109–3135 validates mask size, copies input, removes SIGKILL/SIGSTOP, applies sigprocmask, then copies the old mask to output. Therefore an output-copy failure can follow an actual mask effect: nonzero return never proves no effect. At4536–4563, sigsuspend saves the actual current blocked mask, installs its input, sleeps while no signal is pending, and requests mask restoration with ERESTARTNOHAND. With successful persistent full-mask installation, the saved mask is already full; this addresses363a's restoration-to-original-unblocked-mask frontier. These are exact candidate-source relations, not proof of the selected installed kernel, seccomp/native admission or no own writers.
Source: https://raw.githubusercontent.com/torvalds/linux/v6.18/kernel/signal.c

Fixed v6.18 arch/x86/kernel/signal.c237–261 converts the restart condition to EINTR when a handler is actually delivered. At309–339, the no-handler path restores orig_ax/rewinds the syscall instruction before restoring the saved mask. SIGKILL/SIGSTOP remain unmaskable; termination, group stop/continue, scheduler wake/restart and a real returned user invocation remain distinct. Full persistent mask plus actual own control/handler graph must establish the supported nonreturn relation rather than assuming all stops impossible.
Source: https://raw.githubusercontent.com/torvalds/linux/v6.18/arch/x86/kernel/signal.c

Remaining required coupled source: held exact kernel/provider/syscall/ABI and seccomp permission inputs; immutable readable8-byte mask/output and actual native stack; actual all own signal/group/TTY/SIGWINCH/libc/pthread/control writers; original first-effect→last-borrower custody; explicit output-failure/mask-effect retention; full final_capture/archive258/independent-reader schemas and containing cost. A failed preparation or unexpected suspend return is retained and has NO selected safe live continuation yet; no caller may fall through, spin, retry, return private Args or claim native parked/FINAL. The existing genuine outer terminal/late-ACK route must own actual termination observations. Narrow Linux external memory/control selection does not qualify kernel/provider denial or excuse own actors. No body change has been made.
## 363d: selected construction mapping and nonactivated custody frontier

Parent source disposition on 2026-10-12 selects363c for construction only. Installed-kernel/provider and full outer termination relation remain unqualified. No existing spin/pause/futex body is replaced by this mapping.

Each distinct bootstrap or forward-worker occurrence needs three separately retained raw-call originals: issuing-thread gettid, rt_sigprocmask, rt_sigsuspend. The thread query is not PID authority; it joins the already-owned native thread occurrence and actual call ordering. Each uses the existing sl_linux_call source/number/six arguments/raw signed result/entered/returned schema. Two distinct uint64 mask cells hold immutable full input through the kernel borrower and writable actual prior-mask output through genuine return or independently established post-termination stability; prior output is never read as a successful mask observation after failed mask call. The mask input is eight bytes for the selected x86_64 raw ABI, not libc sigset_t. Original catchable mask and effective kernel removal of SIGKILL/SIGSTOP remain distinct. No mask result authorizes release.

Source locations: owner_linux.h sl_linux_call; owner_linux.c sl_linux_call6 and sl_linux_retain_bootstrap; forward_workers_linux.h held_wait/hold_word and forward_workers_linux.c unresolved path; final_capture_linux.c final_native_call/final_bootstrap/final_forward. Raw call archive body is80B (number8 + args48 + result8 + entered8 + returned8), kind3 with32B framing. Three new originals therefore require336 serialized bytes plus actual independent native storage, source strings, masks, owner-state records, all archive copies/headers and full native stack/provider lifetimes. Native storage is derived from exact selected C layout and assertions, not archive80B assumed as sizeof(sl_linux_call). Original old held_wait/failure_pause remains retained if previously attempted; no reuse/overwriting to make room.

Bootstrap existing call roster uses233 rows: initial21 + gate8 + namespace4 + bounding/ambient192 + credential8. Adding3 distinct parking originals gives236, within current240 pointer slots; this arithmetic is not reservation authority and any further source call requires rederivation. Forward normal roster currently has7 pointers (six signal calls plus held_wait); parking adds3 to explicit10. Two mask bodies add16 payload bytes plus two32B records if independently serialized; alternatively a single original16-byte body needs48 serialized bytes. Choose one closed schema in implementation and independent interpreter, retain exact owner association and no alias with native outputs, source records or future archive writes. All formulae are pending actual containing reserve and simultaneous lifetime charge.

Normal final_forward requires successful native reader/forward joins and actual return of each archived source call. A genuinely parked worker cannot satisfy that cut; removing this guard or manufacturing returned is forbidden. final_bootstrap only allows execute to remain entered/nonreturned. Parking calls must not be granted that exception as a shortcut to FINAL. Failed occurrence snapshots require the genuine outer terminal observer's post-termination stable-original chain and separate late ACK, retaining shared bootstrap mapping/thread stack/process handles and all objects until those borrowers settle. Current owner_linux.c explicitly says outer recovery is not implemented. Current source does not yet establish the exact caller owning external termination or failed forward-worker stable snapshot; those are actual coupled implementation units, not future native observations presumed filled.

Before any caller activation: preflight full containing owner/output/backing noalias and once-only state before mutation; retain exact thread query; install persistent mask once; on zero return enter distinct suspension once. Failure may have changed the mask, and unexpected suspension return retains all partial originals. Neither case has a selected safe live continuation until actual outer terminal/custody source is connected. No retry, spin, normal thread return, private Args publication or successful FINAL may be supplied as fallback. Supported suspension nonreturn and actual process termination remain distinct evidence. Full own signal/group/TTY/loader/pthread actor mapping, kernel/seccomp/provider input and source-bound stack/cost admission continue independently.
## 363e: actual creator storage and outer-observer connection frontier

The concrete native source distinguishes two processes. controller_linux.c9–25 uses child bootstrap CLONE_PIDFD plus waitid(P_PIDFD,WEXITED|WNOWAIT|WNOHANG) to observe the payload. service_linux.c installs root_terminal_original from that actual child wait and separately joins a post-root family sample. Neither is an outer nativecreator terminal observation. candidate_arena_linux.c646–669 constructs current startup/controller/forwarder/service backing with MAP_PRIVATE|MAP_ANONYMOUS. candidate_arena_linux.h explicitly excludes worker stacks/Go/Windows/FINAL. Thus the payload/bootstrap shared mapping does not preserve all nativecreator originals through creator termination.

The production producer currently launches tui28-native-owner-observer.py, which Popen/waits a Python five-action owner. That historical owner launches compat-v1 payload via Python fork/fexecve and does not bind the new native C startup/arena. Its receipt tuples, /proc startticks, local fifteen-second loop and Python wait do not qualify nativecreator/shared-originals/late ACK. Preserve historical evidence; do not silently relabel this caller as the complete native method.

The implementation-ready next relationship is the existing outer observer role as actual parent of the nativecreator: before that birth it must own the containing reservation and retained backing for every required late-original occurrence, then genuine native birth/terminal observation and independent reader/ACK. Current MAP_PRIVATE arena and private pthread stacks cannot be read after process death merely because a pidfd is held. A source-owned shared backing construction must enroll exact storage before first effect, keep observer input views readonly while nativecreator outputs are live, and retain both mapping references/source geometry through stable postterminal interpretation. No payload inherited descriptor count changes follow from outer parent mapping. Introducing the actual mapping constructor remains a source decision coupled to own descriptor/mapping writers, exec inheritance, exact source factory, native stack placement and containing finance; no current body is changed by this proposed relation.

Do not use a preterminal copy as if it contained later raw return/termination originals, or use a creator snapshot to certify its own last write. An observer mapping alone is not immutable: own creator/kernel/descendant writers must each be source-qualified and joined at true last use; only actual creator terminal plus settled external/kernel borrowers permits the corresponding stable views. Any still-live payload/descendant writer remains separately held. Required late observer outputs/ACK themselves require their containing original owners and terminal chain; no recursion is discharged by naming an observer or setting a completion bit. Whole actor/caller/finance/source acquisition must be concrete before activating parking.
## 363f: one concrete Linux outer-parent construction for disposition

Smallest proposed Linux delta: the existing native-owner member enters an outer-parent function on its original public invocation, acquires the already-required containing reserve/profile/source inputs, constructs shared original backing, and performs one pre-thread fork-like clone3 into the nativecreator function in the SAME image. No additional package member, public option, environment input, executable installation, payload inherited descriptor or new operator command. The branch is a genuine native birth return, not a supplied role flag. Existing public argv/stdio/terminal behavior belongs to that original invocation; nativecreator continues the current startup with SAME original views. This is a proposal requiring bounded source disposition, not an installed body or input authority.

Before clone, change the containing candidate arena construction from private anonymous to shared anonymous RW with the SAME checked layout, plus include the arena root, creator-birth/outer observations, park originals/masks, archive and late-original records in the containing layout. Observer and creator map identical virtual addresses through fork-like clone (no CLONE_VM/CLONE_THREAD/CLONE_FILES); only creator owns output writes while alive. Parent retains its mapping reference, no arbitrary path/name lookup or inherited mapping descriptor. Source inputs are separately immutable; no child COW output may be mistaken for parent-visible original. Exact full layout and all native output targets must be audited before this change, including output pointers currently referring to stack/private globals. Every required original moves to held shared storage before its first effect; genuine native stacks themselves remain separately charged through process terminal, without claiming their bytes survive death.

Native birth wrapper must retain distinct parent and child return cells, not invoke existing sl_linux_call6 against one shared output that both clone returns overwrite. A reviewed raw assembly syscall branch captures AX into the corresponding preowned occurrence before calling any C code; same clone_args contains source-owned CLONE_PIDFD output plus SIGCHLD. No stack replacement is requested; fork-like clone preserves each native stack and source execution state. Parent holds the actual pidfd returned by this exact clone and its actual parent-child wait ownership, not a PID reopened from receipts. Clone failure/partial pidfd remains retained; no creator starts from unknown birth. Fork occurs before any product pthread or runtime worker; exact libc/loader/atfork or raw-fork safety must be qualified from selected native source/provider rather than assumed from a single observed thread count.

Parent owns distinct precharged finite service rows and original D/clock, native pidfd waitid(WEXITED|WNOWAIT|WNOHANG) observations and exact poll/scheduler wait originals. The schedule cannot renew D; all rows and wait costs are already financed. Zero si_pid is not terminal. Successful terminal code joins SAME birth/pidfd and CLD_EXITED/KILLED/DUMPED, then separately source-qualifies native process-wide thread exit and kernel borrowers of shared outputs. No creator-requested kill or parent kill/reap is introduced as recovery authority. A parked creator remains live/UNKNOWN until an independently authorized genuine terminal event; no deadline expiry manufactures that event. This source method is useful for authentic natural/adverse terminal observations while retained failure remains explicitly charged.

After actual terminal and all other source writers/kernel borrowers are settled, parent independently reads the SAME shared originals and verifies them using existing independent interpretation rules. Park entered/nonreturned stays entered/nonreturned; parent must never fill creator Returned cells. Failed-terminal archive is a separate schema/cut from existing successful final_forward joins; preserve archive258 purpose/last-borrower/once-close/readonly reader/native/interpreter ACK and distinct late-original suffix. Observer's own wait/archive/read/ACK/close originals occupy separate preowned storage, never the prefix it consumes. The ACK last-use chain must include native completion/reader thread terminal and observer's own final output borrowers; no self-termination certificate is added. All remaining outer originals remain held by the existing qualification/custody chain until its exact last borrower, with no success inferred from a source-written status.

Rights/lifetime table: creator receives copied prebirth public views by fork semantics; parent performs no public stream IO/TTY changes through those copies. Any private pipe/socket/file created before fork must have exact parent-copy close originals and true last-borrowers; currently choosing to create those only in creator after fork avoids extra parent private aliases, subject to actual factory trace. Shared anonymous backing has no file descriptor; parent never munmaps while creator or independent reader/kernel output borrower lives. Child may not unmap/reuse any shared original region before parent's terminal read/ACK. Existing default pthread stacks are not shared original backing; all actual native result/buffer targets must be in the reviewed held arena, or preserved by an explicit separate native lifetime relation. Raw syscall wrappers/C locals/stdio/provider internals remain source-qualified costs and effects, not blanket exemptions.

Additional charge over current source: actual outer process and native runtime/stack/loader/TLS/provider resources, full shared arena/root and park rows, two distinct clone-return originals/pidfd storage, finite observer schedule/waitid/status/rusage records, failed-terminal archive/readback/ACK/late suffix, parent copies of public view refs and every partial-failure lifetime, concurrent creator/payload/full Node/Core runtime overlap. Sharing pages does not remove physical/native accounting or per-reference custody. Values remain UNKNOWN until authentic profile/containing reserve qualification; no literal budget or deadline is invented. Windows requires its own source-qualified mapping/birth/console operation relation; this Linux fork recipe is not imported as Windows approval.
### Selected Linux outer birth original ABI precode (TUI31, 2026-10-12)
Implement parameterized x86_64 raw clone3 fork-like birth under363f: one actual syscall entry and distinct parent/child raw-return storage in a preowned shared containing root, actual CLONE_PIDFD kernel output, SIGCHLD, allothercloneargszero/no CLONE_VM/THREAD/FILES/VFORK/newstack. Normal SysV call preserves callee-saved/native SP; inline assembly source captures each raw return immediately before C continuation. Fullzero/freshness/once/source layout before effects; no sl_linux_call6 sharedreturnoverwrite. Parent pidfd/child0/error remain native observations only. Actual prethread libc/TLS/loader fork safety, containing shared root/arena, copied descriptor/writer graph, D/reserve and outer failedterminal/lateACK caller must dominate activation; native body remains uncalled until complete relation.

### Linux shared candidate backing constructor precode (TUI31, 2026-10-12)
Refactor the SAME checked candidate arena layout/bind into internal source-owned private/shared anonymous construction wrappers. Existing private caller stays equivalent; new shared wrapper remains uncalled until actual outer sharedroot/source outputs/descriptor/fork/provider/D/reserve linkage is complete. Retain actual native mapping flags in original call, use distinct source descriptor for shared map, no fd/name/inheritedcap introduced, no remap/copy or numeric defaults. Shared mapped data is not all required originals: candidate root/prebirth outputs/private globals/stack result targets/bootstrap/worker stacks/latearchive still need explicit containing relocation and lifetime audit before birth activation. Source mapping geometry is not finance.

TUI31 nested prebirth storage precode: selected containing-map geometry reserves bootstrap, fd-action outputs, both credential status/read/group histories and each policy preexec shared/query/input region before outer birth. Preexec input envelope derives from actual candidate source extent, not parser capacity. Exact later binders must retain containing acquisition/range origins without fabricated nested mmap results; credential/SELinux/public IO effects remain child-only. Full output-pointer/alias/root/stack audit and genuine finance/terminal caller remain required; unactivated source only.

TUI31 containing-origin archive precode: serialize each used nested range as an explicit closed LE80 kind36 representation paired with its SAME actual containing mapping native-call original (kind3), retaining offset/extent and actual derived address without fabricating a nested mapping result. Independent readonly representation decoding must check the exact native mmap arguments/result/entered-returned and overflow/range/address association. Copies/repeated references remain charged separately; archive representation does not authenticate producer, finance memory, prove last borrower, or replace whole prefix/late-ACK/outer terminal custody. Full existing consumer integration remains required before activation.

TUI31 outer output-root precode: reserve typed public-main owner (including gate originals) and distinct outer-birth root in the SAME prebirth containing map. Generated source must bind these addresses before birth; returning an already constructed arena is not a second native acquisition or finance grant. Existing public constructor/argv/provider/deadline semantics and all original outputs remain required; raw-fork TLS/libc/loader/stack safety, actual observer caller and complete failed-terminal/lateACK custody stay unqualified.

TUI31 outer terminal precode: distinct parent observation root retains exact successful parent clone return/pidfd association, original nonblocking waitid(P_PIDFD,WEXITED|WNOWAIT|WNOHANG) call, siginfo and rusage. Zero si_pid remains not-terminal; only actual matching PID/CLD_EXITED/KILLED/DUMPED/native returned zero can project terminal. No kill/reap/close/retry or D renewal. Each row is prebirth containing storage under an explicit generated outer_observations source dimension (no default); parent branch, exact original D/finance/schedule, raw-fork safety, full stable-source/lateACK custody remain independently required before activation.

TUI31 generated shared-factory precode (2026-10-12): replace generated private-map/static-public-owner path with once-only prebirth shared arena construction and explicit SAME contained public-owner attachment. A separate prebirth factory occurrence retains preparation failures outside the not-yet-acquired mapping; successful attachment points into that actual containing acquisition. Preparation performs no public startup/admission/capture or outer birth and supplies no finance/D/authentication. Actual parent/child entry and complete observer/late-ACK/provider/stack/argv lifetime closure remain required before activation.
TUI31 preexec transitive storage exclusion precode (2026-10-12): before mutating a containing-route preparation occurrence, exclude its entire output backing and the occurrence itself from SAME completed parse/context memory arenas, allocation-attempt rosters, diagnostic frame/argument/output backing, policy-body input and context-byte inputs. Use checked source capacities and each actual retained frame; no traversal pointer becomes authentication. Existing owning arena construction supplies disjoint region origins; this explicit consumer check protects retained originals at the preparation seam. Full provider/raw-fork/own-writer/string referent/stack/terminal/finance qualification remains separate and no activation is added.
TUI31 preexec source-graph continuation: the same containing-route preflight additionally excludes completed candidate-policy/profile roots, full source bytes and all five configured/actual profile row arrays; four retained status sample referents exclude their owning status structures and exact five-u32 kernel view without reading that live view. These are storage exclusions only, never status currentness/provider proof. Added profile-byte/status-view alias fixtures remain synthetic UNRUN; selected source-derived arena geometry, all string/native referents and full caller/late-terminal lifetimes remain independently reviewed obligations.