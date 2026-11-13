# Windows runtime bootstrap source investigation 362

Status: SOURCE INVESTIGATION, not selected architecture, complete method,
implementation, independent review, admission or execution. The full TUI
delivery scope remains active. This record narrows the missing 361 ABI and
first-use relations; it does not replace them with a grant.

## Actual startup boundary

The selected installed Go source has an existing Windows native-call primitive
that predecessor361a did not inspect: internal/runtime/syscall/windows.StdCall.
Its amd64 ABIInternal trampoline moves AX to CX and jumps to asmstdcall; its
arm64 trampoline branches directly. Both implementations use the caller's
StdCallInfo, call the original native function, and save the actual R1 and Err
before returning. The amd64 implementation additionally saves XMM0 in R2;
arm64 explicitly has a floating-point R2 TODO. Proposed bootstrap APIs have
integer/pointer returns only. Both implementations clear last error before the
native call. They neither use g/m nor depend on asmstdcallAddr. This is the
actual existing native ABI implementation, not permission to copy it into an
unverified custom entry shim.

runtime.stdcall_no_g already documents no-G use and invokes this primitive, but
returns only R1. It therefore cannot be the source of complete original-call
custody. A future fixed bootstrap must retain its SAME complete StdCallInfo,
actual argument cells, native output backing and entered/returned originals.

Actual amd64 rt0_go establishes g0/m0/TLS before CPU feature checks,
runtime.check, args, osinit and schedinit. Actual arm64 rt0_go establishes g0/m0,
calls runtime.check, then Windows wintls, then CPU feature checks, args, osinit
and schedinit. An osinit hook alone misses early failure diagnostics. A
repository-owned Windows-only rt0_go hook immediately after g0/m0 establishment
and before check/CPU branches is the concrete investigation direction. It must
be NOSPLIT, allocation-free, write-barrier-free and use fixed static original
backing, with actual stack bounds qualified. It can use the existing StdCall
primitive without changing the installed toolchain, overriding -E, introducing
a CRT entry, or invoking ordinary runtime.stdcall before osinit.

No hook body is implemented here. ABI0/ABIInternal wrapper and nosplit closure,
original OS stack/TLS/loader effects, every earlier rt0 operation and all
failure/return paths still require exact source analysis and qualification.

## Windows arguments and package initialization

runtime.runtime1.go goargs returns immediately on Windows. os/exec_windows.go
init is the actual Windows os.Args constructor: it calls GetCommandLine,
converts UTF16 through the selected WTF8 function, uses the selected Go quote
parser, and uses Executable on an empty command line. Thus publishing only
runtime.argslice would not reconstruct Windows os.Args.

The proposed selected-toolchain source overlay must connect that actual os.init
to the SAME authenticated fixed original argument storage from selected358.
It must preserve nonempty whitespace zero-count, actual empty fallback/query
failure, immutable parsed views and original private argument storage. Standard
flag initialization then sees public args from its first use; all other early
package consumers still require the complete import/source inventory. This is
not an import of application lifetime into the standard library, and no runtime
export, generated constructor or public flag is implemented by this record.

The actual cmd/go build flag documentation describes -overlay with a Replace
mapping of source paths to backing paths; it expressly forbids replacing files
under GOMODCACHE. A complete recipe therefore needs repository-owned pinned
dependency sources, replacement mappings for selected GOROOT source, exact
source hashes and full build-plan/provenance propagation. No overlay was
generated, parsed or used and no source/toolchain was copied or modified here.

## Original console operation relation

SetStdHandle to pipes cannot preserve original console behavior. Actual
os.newFile first observes GetFileType and conditionally GetConsoleMode, selects
console/poll behavior, and normal console writes later use WriteConsoleW.
Runewidth independently calls GetConsoleOutputCP. Existing termenv, x/term,
go-isatty, coninput, owned localereader and BubbleTea consume actual console
operations through several distinct wrappers and direct Proc.Call paths.

AttachConsole plus fresh console opens is not asserted equivalent to the
original standard handles: access rights, failure results and last-use
ownership can differ. A concrete preserving direction is a typed native-owner
original-call broker on the SAME duplex after genuine creator/birth/entry
association. It operates on SAME retained original standard handles; requests
and route values identify an already-enrolled source occurrence, never supply
authority. Each original result/count/error/output remains native-owned before
the source-specific response is interpreted.

The minimum connected operation table must include GetFileType,
GetConsoleMode, SetConsoleMode, GetConsoleScreenBufferInfo, GetConsoleOutputCP,
ReadFile, WriteFile, ReadConsoleW, WriteConsoleW, PeekConsoleInputW,
ReadConsoleInputW, original cancellation operations, and source-timed CONIN$
open/close. The table must cover runtime.write1 before package initialization,
syscall standard-handle globals, os.NewFile/poll classification and conversion,
termenv/lipgloss initialization, runewidth, x/term, go-isatty, localereader and
coninput/BubbleTea. Source hooks route only enrolled originals and preserve the
unchanged native path for foreign handles. A cached terminal Boolean cannot
replace an actual operation. Run-only fallback opens remain Run-only, preserving
help/config/flag paths.

Blocking calls require distinct native worker occurrences and independently
observed completion while the owner continues raw capture/control/deadline
service. Cancellation requests remain original API attempts, never completion.
All requests, responses, native pending buffers, route handles, original and
postbirth aliases, worker stack/TLS and partial/late results require fixed
preowned backing through LOCAL/FINAL/258/independent ACK and eligible last use.
The exact full operation/handle/capacity table, implementation and genuine
creator/entry consumers remain unfinished; this record supplies none of them.

## Exact current source observations

Read-only source observations, not loaded-image evidence or whole-file review
where a span is explicitly identified:

- Entire internal/runtime/syscall/windows/asm_windows_amd64.s:
  3B99B6A681CE2CD27ADC6D4A1B05E841D9CDEB5FFF6962936A83C9204D60D5C6.
- Entire internal/runtime/syscall/windows/asm_windows_arm64.s:
  C855772B50FCEE9BE38C3D422D2321EC01A8CE0E8DFE7E38F9251447243C16E5.
- Entire internal/runtime/syscall/windows/syscall_windows.go:
  8D51118789A846F0EED3E41B1747161F50FB98CDF468434EACED39DD176FCFBD.
- runtime/asm_amd64.s startup spans:
  837D93BA923E8EE89144512A7A9C7A3AA54DA91B0143FA149AC7732FE68756B8.
- runtime/asm_arm64.s startup spans:
  C2D54A06306C90A1D6AB666101C563056578A84C9B11F50575874AF9C54B2133.
- runtime/proc.go schedinit span:
  B0EA864C1DD251CD9146644863AE9126CA57761DFF05862DC80AB0799925FEC6.
- runtime/runtime1.go args/goargs span:
  E9AEA0B82B76931E2181563CC967112DE12416A90C5467DA8F2E178BBB04B8B2.
- cmd/go/internal/work/build.go overlay flag span:
  08741DC7C7502E6525FB54929AA0237919A7E096C7E0192A1F1B21BC19D19A3C.

Installed source root is C:/Program Files/Go/src. No build, import, compiler,
parser, product crypto, test, native effect or admission was executed. Full
instructions/current cumulative inventory intake remains in progress where
large existing documents were only partially displayed. No self SOURCE_GO or
whole-source review claim follows. No disposable output was created. Existing
dirty source/index remains retained under successor31 for issue1/PR33 until
complete reviewed landing and verified safe closure; no partial commit made.

## Follow-through 362a: actual compiler wrapper and TLS seam

Entire selected cmd/compile/internal/ssagen/abi.go read, SHA256
5BE628B614F1576A624AFD7948ADFA963FB9E706DA23AFE8434227319C5BE4D8.
GenABIWrappers consumes actual assembler symbol ABI references and generates
the required wrappers. makeABIWrapper marks wrappers NOSPLIT; a no-parameter,
no-result function uses a tail call. The proposed bootstrap signature is
therefore a no-parameter/no-result runtime function referring only to its
source-owned static original root, not a function carrying arbitrary pointers
from a caller. This establishes a source recipe for the wrapper, not compiled
stack geometry or proof of the function's entire NOSPLIT callee closure.

Selected cmd/compile/internal/amd64/ssa.go1297ff, SHA256
EC36DAA5C90A20E71C7DCFB3FF866FC7459CF9EA23579A73AC3DE3AA08AED862,
explicitly zeroes X15 and obtains R14 from TLS when an ABI0 call/tailcall enters
ABIInternal. The proposed amd64 hook must remain AFTER actual wintls/settls and
g0/m0 publication, before CPU/check branches. Merely marking a function
NOSPLIT does not eliminate this genuine TLS borrower.

Selected arm64 startup already keeps g in its dedicated register, but actual
Windows wintls is later than runtime.check. sys_windows_arm64.s160ff, SHA256
1FF1A9CEA659C0C59DEDC651C95468EFBFDA889FC3D44766B5F48CAB1E4ECF24,
uses the OS initial stack to call TlsAlloc directly, then publishes its chosen
TEB slot offset. tls_arm64.s entire, SHA256
B710CCBED6A02B80CD44A6289FEDC6ADCED85B91CA67FB7893BCFA57A99B4B0E,
contains the actual load_g/save_g users. Any arm64 hook must prove its complete
callee chain does not rely on the later TLS slot, or explicitly account for an
earlier TLS constructor without double acquisition. No arm64 entry body or
unsupported platform promise is inferred. Current native owner build-plan
source accepts linux-x64/windows-x64 only; that source fact does not silently
change the broader product's platform applicability.

Existing StdCall MaxArgs42, amd64 fixed argument stack336 bytes, saved/alignment
frame and hook/wrapper/callee stack are separate actual charges. Native API
stack use and OS stack commitment remain genuine qualification data. All
native raw errors use the SAME StdCallInfo Err; the primitive's SetLastError(0)
behavior must match each original source seam, never be assumed transparent to
arbitrary C calls that previously sampled a stale last-error value.

Additional original consumers found during this source cut: os.NewFile's
IsNonblock and internal/poll FD initialization/disassociation/cancel/close paths
introduce actual NtQuery/NtSetInformationFile/IOCP and handle-alias obligations.
They cannot be funded by or omitted from a terminal-mode Boolean. Actual
current native public stream constructor rejects INVALID_HANDLE_VALUE before
child birth; original syscall.getStdHandle ignores the error and os.NewFile
returns nil for that value. The complete method must preserve NULL versus
INVALID and original downstream diagnostics instead of rejecting help or
configuration early. No dependent product repair is implemented here.

## Follow-through 362b: original-owner operation and alias frontier

A tempting postbirth remote DuplicateHandle plus SetStdHandle route is rejected
for console originals. Microsoft documents both console input and screen-buffer
handles as duplicable for use only in the same process. Ordinary shared console
association is a different relation and does not override that restriction.
The API reference and console-handle guidance were read directly, not tested:
https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle
https://learn.microsoft.com/en-us/windows/console/console-handles
https://learn.microsoft.com/en-us/windows/console/creation-of-a-console
No remote duplicate body was written. File/pipe remote duplication remains a
separate possible operation, with actual native failure retained, never a
universal console solution. No fresh CONIN$/CONOUT$ open substitutes for the
rights/failures of an existing original handle.

The following is the concrete source-derived operation frontier for the
original-owner broker direction. It is a dependency table for finishing the
method, not a selected protocol, implemented worker, complete source inventory,
capacity proof or authority issued by a wire opcode.

| Actual consumer seam | Native original and retained output | Worker/cancel/alias relation still required |
| --- | --- | --- |
| runtime.write1 fd1/fd2 | GetStdHandle, ASCII WriteFile; non-ASCII GetConsoleMode then WriteConsoleW; original bytes/UTF16, written count, raw result/error before public return projection | Separate stdout/stderr original cells even when same object; runtime console lock/chunk boundaries retained; early failure custody before osinit; no diagnostic can require Go scheduler startup |
| syscall standard globals and os.NewFile | GetStdHandle NULL versus INVALID; GetFileType, GetConsoleMode, NtQueryInformationFile FileModeInformation with actual IO_STATUS_BLOCK | Root enrollment precedes package init. os.NewFile invalid returns nil; NULL proceeds with native invalid-handle failures. No early native constructor rejection changes help/config behavior |
| internal/poll FD.Init | runtime poll association, SetFileCompletionNotificationModes result; FD kind, isBlocking, skipSyncNotif projections | Full owner/child IOCP relation must be explicit; a remote handle token cannot be passed unmodified to child's poller. No fabricated IOCP completion or forced blocking classification |
| internal/poll FD.execIO read/write | ReadFile/WriteFile actual original buffer, OVERLAPPED, event, immediate count/error; genuine pending and late completion count/error | Preserve submit/wait/cancel/query distinction, Go pinned buffers and actual owner native buffers until settled and last borrower. Cancellation race can succeed before cancel; request/ERROR_NOT_FOUND does not itself establish completion |
| internal/poll cancelIO / Close | CancelIoEx specific OVERLAPPED or nil; actual GetOverlappedResult; NtSetInformationFile FileReplaceCompletionInformation; CloseHandle after refcount reaches destruction | Child fdmu/pd eviction and native original completion are different occurrences. Keep original event, completion-key/context, cancel outputs and all aliases. Close cannot free pending backing or issue success |
| internal/poll console read/write | ReadConsoleW original UTF16 and count before Ctrl-Z/UTF8 projection; WriteConsoleW original chunks/count before rune-to-byte projection | Same input/screen-buffer held handle in owner process. Finite independent blocking worker original; source retained 10000-unit input allocation, 40000-byte conversion and per-write chunks charged separately |
| x/term and termenv/lipgloss | GetConsoleMode, SetConsoleMode, GetConsoleScreenBufferInfo actual structures/results | Early global output initialization is before tea.init. Every get/set is an original call; cached is-terminal/color values cannot replace these calls. Restore mode retains actual attempted/returned failure |
| go-isatty Cygwin path | GetFileType, GetFileInformationByHandleEx and NtQueryObject query output/length/result | Direct Proc.Call seam needs explicit owned source fork/hook; pathname-looking output is interpreted only after raw extent/result retained. No GOMODCACHE overlay substitution |
| runewidth Windows init | GetConsoleOutputCP actual scalar/result, independent of standard handle | Execute against original owner's console association at source-timed initialization, not ConPTY/fresh console. This is console context, not a handle-token lookup |
| BubbleTea fallback terminal open | Source-timed os.OpenFile CONIN$; actual CreateFile inputs/output/error then os.newFile classification | Run-only occurrence; owner opens at corresponding source point against original console context. New owner handle has its own original lifecycle and cannot be substituted earlier for standard input |
| coninput / BubbleTea input frame | WinPeekConsoleInput and WinReadConsoleInput exact INPUT_RECORD backing/count/error; GetConsoleMode/SetConsoleMode | Existing source maximum, peek/read and partial output are separate rows. Native worker still alive while owner services controls. Record-count extent validation must follow raw capture, before event conversion |
| BubbleTea Cancel | CancelIoEx nil then CancelIo only when first fails; each original result/error before Boolean return | CancelIo is issuing-thread scoped. Full method must bind each source physical-thread occurrence to SAME corresponding native worker lane, or demonstrate another original relation; arbitrary dispatch worker changes semantics |
| Go runtime signal handler | SetConsoleCtrlHandler plus actual CTRL_C/BREAK/CLOSE/LOGOFF/SHUTDOWN callback and sigsend/block projection | Original console association and process-group delivery need separate custody. ConPTY key bytes cannot silently stand in for actual control delivery; handler return must not terminate owner before capture/settlement |
| localereader Windows decode | IsDBCSLeadByte/MultiByteToWideChar CP_ACP and source-specific GetLastError | These are local code-page calls, not console-original operations. Preserve actual source seam and local originals; do not reroute solely because module name contains locale |

Native-owner state needed for every row is the SAME original sl_win_call-like
entered/returned/raw-result/error occurrence, disjoint native argument/output
backing, source locator enrolled before effect, worker entry/return original,
independent completion observer and readonly interpreter. A route integer,
thread id, echoed handle, SID or frame signature is not enrollment. The
initial selected358 argument frame remains the first actual byte phase;
operations begin only after its actual body/ACK and creator/entry association.

Alias table to complete before a body: owner startup original standard handle;
owner noninheritable held view; genuine child endpoint and gate aliases; failed
CreatePipe indeterminate output cells; successful gate/input/output pipe ends;
duplex server/client/event/OVERLAPPED and inherited endpoint; process/thread/job;
per-worker thread/event; owner CONIN$ fallback handle; each pending operation's
event/OVERLAPPED/output; each file/pipe remote duplicate if that route is chosen;
child os.File/poll reference and each exported Fd/duplicate; completion-port
association and key; mode restoration borrower; source capture/258/archive
writer and independent reader/ACK borrowers. Each needs actual first use,
source-owned constructor, all dup/inherit/transfer paths and actual last use.
No close is eligible merely because public function returned, cancel was
requested, worker was signaled, process exited, or an interpreter accepted a
response. Failed dup outputs never become adopted handles.

Remaining concrete obstacles are method inputs: a no-scheduler prehook failure
channel with genuine creator/entry authentication; full early rt0/loader/TLS
failure custody and stack geometry; issuing-thread CancelIo mapping; native
IOCP versus child poller translation without invented completion; source
signal delivery; finite worker/operation/late-row capacities derived from
actual commands and graph; full immutable raw/output borrowing through
independent ACK. They are not waived by this table. Current Windows owner has
no raw IO workers, resume/admitted entry, broker implementation or LOCAL/FINAL
completion. Linux entry/return/forward-spin and full cross-platform public
behavior remain active obligations. No target execution or partial freeze.

## Follow-through 362c: pre-TLS closure correction

Actual runtime/sys_windows_amd64.s217ff, SHA256
24EFE0D0CC1F7743CD4BFC889FBC6DABB73409BD865C7EC7A1E86EAA01863516,
shows wintls directly calls TlsAlloc on the initial OS stack after reserving48
bytes plus alignment. Slot>=64 falls back to the TEB arbitrary pointer; this
includes the failed allocation value. rt0_go then performs a TLS store/read
self-check and calls abort on mismatch BEFORE the proposed post-TLS Go hook.
Therefore 362a's no-argument compiler wrapper only describes the later hook;
it cannot claim custody of every earlier constructor/failure.

Concrete earlier-source direction is an explicitly authored ABI0 ASSEMBLY
bootstrap closure in the repository-owned selected runtime overlay, fixed
static source root and direct existing StdCall ABIInternal trampoline. The
trampoline takes actual StdCallInfo in AX on amd64 and immediately moves it to
CX; it does not load g from TLS. An explicit assembly ABI0 caller can supply
that register without traversing a compiler-generated Go wrapper. No Go
function, builtin, allocation, defer, stack-growth, write barrier, g/m lookup,
ordinary runtime.stdcall or scheduler call is allowed in this earlier closure.
Actual rt0 saved arguments, registers, flags and original stack restoration
must be preserved; every native imported function pointer and argument/output
cell is part of the full fixed root. The existing StdCall argument-count bound
must be statically respected, not tested by entering its INT3 branch.

Selected cmd/asm/internal/asm/parse.go symbol-attribute span SHA256
857A6CC9D551935E2ED0BFBDE2D5523B1B7B53F06E2567555134C3C212FEC195
accepts ABI0/ABIInternal selectors only for packages marked AllowAsmABI.
Selected cmd/internal/objabi/pkgspecial.go SHA256
EA48BF2A0E5D40C6B70A5A476CB3D0A489BA25614BAB7A6763C0E4EEA66A61D7
is the actual package allow-map. Runtime already uses cross-package assembler
symbol spelling with division-slash separators for internal/cpu originals.
A complete earlier-shim recipe must use the actual selected package/ABI/root
and link relocation relation; this source observation is not an assembled or
linked proof and no shim body is implemented yet. OS PE loader/TLS callback
and imported-code startup before rt0 remain their own native qualification
inputs; no assembly hook turns them into posthook custody.

For later broker workers the proposed map must bind EVERY actual source issue
and cancel physical OS-thread occurrence independently, including goroutine
migration. Stable goroutine identity is not a thread identity. A native-owner
lane is constructed/enrolled with its own actual thread/rights/stack/TLS and
retained lifetime before first issue; CancelIo dispatched to another lane may
legitimately find no matching work, rather than canceling all handle work.
Genuine owner operation completion stays distinct from a transport response or
child wake delivery. The exact finite lane table, IOCP mapping and prehook
creator/loaded-entry authentication remain unfinished method operands.

## Follow-through 362d: default entry reachability recipe

Re-read selected actual linker spans and hash bracket: ld/deadcode.go
014B1506F026602A7088928230B33709D2977A68478FA821774A1BCE2FB38BF9,
ld/lib.go172B125EFA804348C5E1572D8BE975D96933004124EE0106CC3FC4F4A6965F68,
ld/pe.go268658672120085BDF42D5FF59E7DA4A510B058F83331A4D0AE468772ABC4602.
Default exe/PIE entry selection is _rt0_GOARCH_GOOS. Deadcode roots that symbol
in ABI0 and ABIInternal and follows nonweak relocations. Internal PE linking
writes Entryvalue minus PEBASE; Entryvalue resolves actual ABI0 text. Thus a
more direct proposed overlay recipe preserves the default entry SYMBOL and
adds a genuine direct CALL relocation in selected rt0_windows_amd64.s before
its existing JMP to _rt0_amd64. The assembly bootstrap's strong relocations to
its static original root and actual StdCall trampoline keep those reachable.
No custom -E symbol is necessary for this direction. Actual ordinary exe,
internal-link, CGO0 recipe and full source/provenance ownership remain mandatory;
external linking's replacement by main is not silently supported.

Actual _rt0_amd64 subsequently reads 0(SP) into DI and takes 8(SP) as SI before
rt0_go. The proposed Windows entry hook therefore must restore EXACT original
entry SP and leave those original stack cells untouched, not merely preserve
DI/SI or recompute them from copied arguments. It also must preserve required
Windows-entry nonvolatile registers, original return/loader state and any
source-consumed flags. Its explicit assembly frame and StdCall's original
frame/alignment are charged below the original OS stack; no Go synthetic64KiB
g0 bound proves actual OS commitment or native callees' maximum stack. Linker
rooting proves only selected-source reachability, never loaded-byte identity,
creator/entry authentication, native ABI/fit or genuine initial ROOT.

The proposed shim body is still unwritten pending full method disposition;
this source relation does not authorize execution or weaken any entry gate.

362d entry-frame correction BEFORE dependent body: parent identified and own
bounded source reread confirms actual cmd/internal/obj/x86/obj6.go
D77764BC3450BFD094D42F84F232527376B5D27F36A8415A2B5DCB19DFF96F1D.
Adding CALL to the existing NOSPLIT,$0 Windows entry makes it nonleaf; without
NOFRAME preprocessing adds an8-byte BP frame with PUSHQ BP/MOVQ SP,BP. Plain
JMP to _rt0_amd64 does not perform RET/retjmp cleanup, so merely restoring the
shim's SP still leaves the wrong stack for subsequent 0(SP)/8(SP) operands.
The proposed ENTRY text itself must explicitly use reviewed NOSPLIT|NOFRAME,
not only the helper; every owned save/restore and CALL return slot must balance
before the unchanged JMP. This corrects the source recipe, not an executed
assembler/link/native verification or permission to implement a partial shim.

## Follow-through 362e: source-thread lane and native completion proposal

This is a concrete investigation refinement, still not a selected protocol or
body. Microsoft's CancelIo contract confirms it acts only on overlapped calls
issued by the calling thread, returns a cancellation REQUEST result, and
normal completion notifications still occur. It does nothing to a handle not
opened for overlapped IO. Original failed/successful projections are preserved;
CancelIoEx is not substituted for CancelIo to make cancellation more effective.
https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelio

Each source hook must capture its actual physical thread at the native syscall
seam, AFTER any Go scheduling transition that can migrate its goroutine, and
before an operation is assigned to its owner lane. A goroutine-to-lane map or
GetCurrentThreadId captured before allocating/dispatching Go code is wrong.
The same rule applies separately to later CancelIo. Different actual physical
threads select distinct lanes even if their Go goroutine is the same. Same
physical thread reuses only its genuinely held native-thread association, not
a recycled integer thread id.

Proposed owner lane enrollment retains actual OpenThread with noninheritable
QUERY_LIMITED_INFORMATION|SYNCHRONIZE access, GetProcessIdOfThread, GetThreadId,
GetThreadTimes original outputs and independent nonblocking thread wait. Its
association must join the already authenticated SAME child birth/process and
actual source hook occurrence; these API results alone do not authenticate a
wire request or loaded code. Retain the actual opened thread object throughout
lane life to avoid thread-id reuse as a cleanup/routing grant. Unknown/dead/
foreign association retains the failed request without dispatching a native
operation. No SeDebugPrivilege enablement or access-right escalation is used.
OpenThread's original error is retained, never repaired by privilege mutation.
https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openthread
https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocessidofthread
https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes

The corresponding owner native worker must issue ALL thread-scoped originals
for that lane. An asynchronous ReadFile may return pending, allowing that SAME
worker to execute a subsequent CancelIo while an independent observer retains
and services the original OVERLAPPED completion. A synchronous console call
can block its lane while other lanes and the separate collector/controller
continue. Another source physical thread's CancelIo legitimately does not
cancel that blocked lane; the broker must not turn it into CancelSynchronousIo
or all-thread cancellation. These relations avoid a deadlock claim based only
on goroutine migration, while requiring actual native worker control/return
and complete pending lifetimes.

Three occurrences remain disjoint: owner native submit result; independently
observed SAME original kernel completion with original count/error/backing;
transport response and child scheduling wake. A response/wake can deliver an
already held completion projection but cannot create native completion. The
child original internal/poll prepare/wait/deadline/cancel/error projection must
consume this association explicitly; blindly passing a route token to its
ordinary native IOCP or posting a copied status as native proof is forbidden.
The actual IOCP/poll correspondence and operation layout are still unfinished.

Lane table storage is preowned and source-derived; R remains UNKNOWN until the
complete Go/runtime/dependency/command graph and two overlapping runtimes are
qualified. No arbitrary thread count or allocation from wire/thread id is
introduced. Enrollment exhaustion is retained failure, not permission to reuse
a worker with pending originals. Lane/child/native-thread death does not permit
closing original handles, events or output while pending IO/interpretation/
258/archive/readback/late-ACK borrowers remain. Earliest no-scheduler bootstrap
is a separate lane-free assembly closure, not this scheduler-era service.

## 362f: pre-user-mode debug image-base alternative (investigation only)

Fresh official API reads on 2026-10-11 give a concrete alternative to the
unreliable pre-loader module-enumeration route. CREATE_PROCESS_DEBUG_EVENT is
reported before user-mode execution and before other debug events. Its
lpBaseOfImage identifies the mapped executable base. This is an OS-produced
observation joined to the actual creator's debug operation, not a supplied
PID, image name, echoed hash or creator assertion.

Sources:
- https://learn.microsoft.com/en-us/windows/win32/debug/debugging-events
- https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-create_process_debug_info
- https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-debugactiveprocessstop

The structure explicitly permits NULL hFile, hProcess and hThread. These
outputs therefore need individual validity and custody observations; failure
must not turn them into invented originals. lpStartAddress is only approximate
and can change through SetThreadContext. It is not authentic entry authority.
The held image file, mapped image/base, expected fixed root RVA, source-linked
entry, all permitted writers and actual creator operation would still need a
complete conjunction before WriteProcessMemory may initialize a private root.
This alternative does not add an inherited handle or authorize console-handle
remote duplication, PEB mutation, arbitrary executable patching or new issuer.

A debug event suspends all child threads until ContinueDebugEvent. This
suspension and CREATE_SUSPENDED's initial-thread count are distinct originals.
No Continue call may be treated as an unconditional ResumeThread substitute.
The creating OS thread must own WaitForDebugEvent; scheduler-era goroutine
identity is insufficient. Debug event handles are not ordinary owner-created
handles: the system closes process/thread debug handles at the documented EXIT
and continuation cuts, whereas image/DLL file handles require debugger closes.
A separately duplicated process handle is needed to observe completed kernel
shutdown after EXIT_PROCESS continuation. Each duplicate is a new held alias.

Debug attachment also introduces DBG_CONTROL_C exception delivery. A permanent
debugger must pass the exception onward with the actual documented unhandled
continuation behavior to preserve normal handler dispatch. Detaching before
public execution might avoid later debugger behavior, but DebugActiveProcessStop
alone documents only stop/success/error; it does not establish settlement of an
outstanding create event, debug handles, suspension counts or first-entry order.
No detach recipe is selected from that documentation alone. Complete bounded
debug lifecycle, loader/TLS callbacks, source-owned root write/read authority,
control semantics, original observation retention and failure settlement remain
required before adopting this route. Existing explicit early ABI0 assembly
investigation remains independent. No body, execution or admission follows.

The debug creator thread has another concrete failure obligation:
https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-debugsetprocesskillonexit
states that the default on this calling thread's exit terminates its attached
debuggees. FALSE changes exit behavior to detach, but can be called only after
at least one debug connection exists and affects all current/future debuggees
of that thread. Therefore it cannot be assumed successfully configured before
first birth. Its actual postbirth result/error is mandatory; a failure retains
the actual creator thread and destructive default, never permits ordinary
return/cleanup. Successful FALSE does not settle outstanding debug-event/root
borrowers or independently authorize detach. WaitForDebugEvent explicitly warns
against queueing an APC to its thread; ContinueDebugEvent also belongs to the
creating thread. These are new actor and APC relations to close against the
existing birth/control contract, not authority to add a debugger worker now.

## 362g: actual os-init integration seam, not main-only installation

The early native closure need only validate the closed private eight ASCII
locators, authenticate the real creator/entry/endpoint conjunction, and retain
the SAME complete original UTF-16 header/body before TLS/runtime diagnostics.
It must not substitute a newly invented public quote parser in assembly. The
later actual os exec_windows.go init is where public WTF-8 conversion and Go
quote interpretation replace the private command line, before flag/termenv and
all other os-dependent initializers. Empty-original fallback and nonempty
whitespace zero arguments remain distinct; main must not index a zero slice.

There is a concrete package constraint: os cannot import the product lifetime
package to call its current interpreter. custody_collector.go imports os (and
io/runtime/debug/sync), while receiver.go uses sync/context; this would create
an import cycle and is not a pre-runtime no-G closure. The current interpreter
also enrolls into a mutex-bearing Receiver before interpretation. A proposed
os-init hook must therefore place an explicit source-owned lower-layer fixed
root/decoder seam in the reviewed runtime/stdlib overlay, with no product
package import and a complete init dependency graph. The later product factory
must bind the SAME underlying originals and string backing, not reconstruct
them from wire or call a helper and claim early-init coverage. Whether the
existing interpreter is factored into that lower layer or independently checks
the already constructed Args remains a whole source ownership/schema decision;
no duplicate authoritative interpreter or fabricated native enrollment is
selected here. All new raw storage/string views and last public borrowers need
actual capacity and lifetime accounting.

The current module replaces only Bubble Tea, cancelreader and localereader.
termenv, lipgloss, runewidth, coninput, go-isatty and x/term remain module inputs.
Any consumer hook selected by the original-console method must be an explicit
owned pinned source replacement plus provenance, rather than writing the
installed toolchain or module cache. tea_init's lipgloss call alone cannot order
these dependency initializers or runtime write1. This is a concrete integration
requirement; it does not grant overlay bodies or execute a generator.

## 362h: later private root binding under ordinary linker checks

A possible supported linkage for the later product adoption seam is an explicit
provider-side push linkname in the reviewed stdlib overlay and matching private
product declaration. This would avoid importing a stdlib internal package or
adding an exported os API. Actual cmd/link/internal/loader/loader.go SHA-256
0B6B271AEE4951E7202E854795C008733F0F2A6F5726603C7067EDA3A2413DFE,
checkLinkname2496..2543, rejects unmarked standard-library targets while allowing
a definition carrying IsLinkname (provider push) or ABIWrapper, subject first to
its blocked-name allowlist. No -checklinkname=0 is needed or permitted by this
investigation. New provider names must be reviewed against the real allowlist,
compiler directive handling and exact signatures before bodies.

This is only a later initialized-Go linkage candidate. It does not make a Go
wrapper callable before TLS, authenticate a root, retain missing native
observations, or grant a new authority API. The actual provider must expose only
the SAME fixed originals/backing through a source-owned once-adoption relation;
ABI layout, pointer/GC visibility, BSS/relocation, immutable string slices,
receiver binding and all late borrowers remain explicit whole-method work.

Fresh actual compiler seam reads: noder/writer.go collectDecls2651..2690
(70456B4B6C660B6A9189613AFAA7148C8F0455F1D7F5F4D1B087EC184AB5C8FE)
requires unsafe imported in the directive's file, rejects instantiation targets
and duplicate linknames, and accepts declared function/variable local symbols.
noder/reader.go linkname1225..1248
(93ECE30388D9E6EC2EB2D7E9BB42A08FA093467C5EFC0DAF56D98797F2B00BA4)
marks the current package's nonempty linkname AttrLinkname so the linker can
check its actual origin. Entire blockedLinknames map2400..2493 was read; it does
not supply any product provider. A deliberately new reviewed private name is
required; reusing a privileged existing runtime/FIPS target would not be this
recipe. These bounded compiler/linker reads do not claim ENTIRE tool review.

## 362i: actual caller and provenance obligations

Entire current assert-go-source-provenance.mjs read: clean source checkout,
empty ambient/effective GOFLAGS, GOWORK explicitly off, correct module path and
identity remain guards. A proposed owned overlay should be an explicit reviewed
build argv item with its closed map and complete replacement/tool input custody;
it does not require admitting arbitrary ambient GOFLAGS. Exact internal-link
selection and CGO mode must be part of that command and admitted source graph,
not inferred from an incidental non-CGo source package. Existing workflows have
multiple distinct build occurrences and not all explicitly set CGO_ENABLED.
No existing call has been replaced or executed.

Entire current produce-scoped-native.mjs read: archive creation still takes
one public executable byte body. nativeOwnerCompilePlan exposes separate public
and private member names, but that helper alone has not connected two-member
packaging, loaded-byte identity, native caller/factory, same published bytes or
protected publication. Those real caller/archive/source/provenance operations
must change coherently with the selected runtime root and full native fit.
Existing native outcome checks still demand install/config/start/stop/restart,
keyboard/help/resize/escape, cancellation denial, terminal/error cases and
reconnect behavior. Bootstrap source progress supplies none of those results.

## 362j: NOSPLIT linker accounting is separate from native prehook stack fit

Fresh actual stackcheck.go SHA-256
763683B19EC72FB8BF23D64DD841CC9284AE8ACCEE672D1C2B04EFDA46AC9534:
49..110 walks every text function against objabi.StackNosplit minus call size;
197..292 follows direct/indirect-call relocations and PCSP local growth for
NOSPLIT functions. External functions are excluded on the system-stack premise.
objabi/stack.go entire read: ordinary Windows multiplier is one; race changes
it. The reviewed entry/helper closure must pass normal linker accounting, not
silently change its limit, hide stack adjustments or use an indirect call to
avoid a real dependency. Its native Windows loader stack, guard/commit state,
Win32 target/provider frames and asynchronous handlers need separate capacity
qualification even if the linker check succeeds. Excluding an external call
from linker height is not evidence that its actual stack cost is zero.

Existing StdCall assembly additionally clobbers SI/DI itself, clears native
last-error, can clear DF while copying arguments, writes floating-point return
register views and uses its fixed MaxArgs frame plus alignment. The early
NOFRAME entry must preserve true incoming SP/cells and every required incoming
register/flags across the whole closure, not merely rely on the Windows callee
preserving its own nonvolatile registers. These observations constrain exact
assembly layout and error semantics before bodies; no NOSPLIT or native-fit
credit is claimed from reading them.

## 362k: selected flag initialization and zero public arguments

Actual selected flag/flag.go SHA-256
EB3238110346601E8E014DAC5E5FA873BDE7C94BB2F33053F92FD175C1B2A1F1:
init1201..1213 already chooses empty CommandLine name when os.Args has length
zero and installs commandLineUsage, which dynamically calls the global Usage.
NewFlagSet1223..1230 additionally creates default Usage; FlagSet.Init1236..1239
changes only name/errorHandling, preserving flags and Usage. Therefore later
resetting CommandLine via NewFlagSet would discard existing registered flags or
Usage behavior and is not the proposed hook. Correcting actual os init before
flag runs preserves its own original behavior. Independently actual product
main still indexes os.Args[0]/[1:] and must use an explicit zero-argument branch
when wired to the generated root; no synthetic argv0 or public parser grant is
created from whitespace-only original raw input. Current main-return fixtures
cover helper help/error outcomes and five flag registration, not this entry.

## 362l: existing lower package avoids overlay-only package discovery

Parent source investigation identified a fresh virtual stdlib-package discovery
hazard: cmd/go modindex can fall back to physical goroot directory scanning, so
an overlay-only new package cannot depend on cache state, file age or sleeps.
Concrete revised direction is to extend the already physical, allowAsmABI
internal/runtime/syscall/windows package with reviewed overlay files. runtime
already imports it; actual os init may import that existing package subject to
full cycle/init verification. Product later binds an explicit reviewed provider
push as in362h. No installed-toolchain or module-cache mutation is introduced.
This is a source direction, not a selected root/layout/authentication body.

Fresh import-source scan found existing selected amd64 windows package imports
internal/abi, internal/goarch and unsafe. internal/abi selected Go files import
internal/goarch and unsafe; goarch generator gengoarch.go is separate ignored
source, not an admitted runtime dependency. Full selected files/build tags/init
closure still must be enumerated before code. Directly copying product decoder
imports is concretely invalid here: errors/wrap.go imports reflectlite, whose
value.go imports runtime; binary.go imports errors, io, reflect, slices and sync.
Since runtime itself imports the lower windows package, these would form a real
cycle. Inspected hashes:
errors/wrap.go 098116636610AE87DD80E49C4DBE6A2B6C37918D7E1AF35DED005BE621D68A40
internal/reflectlite/value.go 37526F8C1C775BF455060CD93C6CFCC2518BA0DE38F63D78CDD1DEF75B597212
encoding/binary/binary.go FC4FAB16EE184C5EAAFB75C2E2B8728C9DD04228F96EC8D394C9D3110F9C91DF

A proposed lower-layer decoder therefore uses fixed source-owned typed arrays,
explicit checked little-endian reads and a fixed custom failure representation,
with no errors/encoding/binary/os/runtime/syscall/product imports. Its ordinary
Go body runs from actual os init after runtime initialization, never pre-TLS.
The earlier assembly root acquisition stays separate. Unsafe string views must
remain bound to SAME immutable static buffers through the last actual public
borrower; the later Receiver cannot reenroll fabricated native observations.
Compiler implicit runtime helpers, bounds/panic paths, static pointer layout,
provider ABI/GC exposure and complete capacity remain review obligations even
when source imports themselves are acyclic.

## 362m: actual ordinary console path is blocking, separate from redirected IOCP

Actual os/file_windows.go newFile selected spans show that successful
GetConsoleMode classifies typ=console then breaks before IsNonblock detection;
NewFile's incoming nonBlocking is false, and newConsoleFile explicitly supplies
false. Actual internal/poll/fd_windows.go FD.Init console,false sets kindConsole
and isBlocking then returns before pd.init/SetFileCompletionNotificationModes.
FD.Read kindConsole calls readConsole, whereas file/pipe paths use execIO and
may wait/cancel/GetOverlappedResult through the poller. This supplies a concrete
separate operation relation: ordinary console os.File calls need genuine
blocking owner native call completion and existing conversion/error behavior,
not a fake native child IOCP event. Direct coninput/syscall consumers remain
independent operations; the distinction does not authorize forcing other
handles blocking or classifying a pipe as console through a Boolean.

For redirected file/pipe, execIO retains pinning, operation-pool ownership,
prepare/submit, synchronous-notification selection, actual wait result,
CancelIoEx request followed by waitCanceled, GetOverlappedResult and precise
aborted/incomplete error mapping. A remote source route must preserve all these
relations explicitly or use a demonstrated genuine native handle path. A
transport wake cannot replace native completion; copied OVERLAPPED fields cannot
supply it. Native original stream forwarding/capture and D2 independence also
remain required even when ordinary file/pipe cross-process duplication is an
API-supported possibility. No raw-output/last-borrower obligation is removed.

## 362n: original standard-handle failure projection is not universal startup denial

Actual syscall/syscall_windows.go globals call getStdHandle, whose body retains
only the returned handle and ignores its error for the public projection.
os/file_windows.go NewFile returns nil for InvalidHandle; NULL/zero proceeds
through ordinary newFile query/FD behavior. os/file_posix.go checkValid returns
ErrInvalid for a nil File. These distinct original outcomes precede flags but do
not make the selected Go runtime universally reject startup at GetStdHandle.
Current native owner_windows_entry.c instead returns0 immediately on
INVALID_HANDLE_VALUE; that cannot be credited as preserving all help/config/
flag/independent-diagnostic paths merely because it retains GetLastError.

The coherent method must retain native errors and raw return values without
silently replacing the public projection with an earlier mandatory console
success predicate. Root/creator/entry authentication remains mandatory for
private transport; preserving original public semantics is not permission to
skip authentication or invent a usable stream. Console mode, CONIN$ fallback,
original output failure and each operation's timing remain independently mapped.
Per-stream absence/failure/console/redirected classification and actual partial
held-view failures therefore need explicit source states and eligible behavior,
not one streams_prepared Boolean granting every public borrower.

## 362o: lower package layout and init ownership cut

Implementation construction remains unselected. Use virtual overlay files in
existing internal/runtime/syscall/windows, not a new standard package directory.
Keep its existing imports and original StdCall unchanged. Divide added source
into (1) amd64 entry assembly, (2) pointer-free fixed native root/record storage,
(3) ordinary post-runtime checked argument interpretation, and (4) later private
provider binding. No lower file imports os, runtime, syscall, errors, binary,
sync, or product packages. The runtime package already depends on this package;
a reverse import is therefore invalid even if hidden inside an unused helper.

The proposed root is one source-owned static object, not an address received
from public argv or a decoded wire field. Its actual linker symbol/RVA and
native write extent must come from the reviewed final image/layout. Native
creator must retain the exact remote mapped-image observation, image file view,
process/thread objects, source-owned write operands and actual write result.
An echo of those values inside the root does not authenticate the writer. The
entry assembly must establish the creator/entry conjunction before the first
bootstrap stream read. A debug-event supplied mapped base is only one input to
that conjunction; its existence grants neither endpoint nor loaded-code trust.

At ordinary os init, import the existing lower package under an explicit alias
and request its completed argument result. Replace the actual GetCommandLineW
public reconstruction at that seam; do not append a late init hoping to repair
Args after flag or terminal consumers. Failure before publication retains the
partial lower object and original native observations. Success publishes only
the exact strings backed by the retained Parsed storage. Whitespace input may
publish a zero-length Args slice, so the actual product entry must avoid both
os.Args[0] and unconditional os.Args[1:] before choosing the parser input. The
flag package already permits an empty program name; preserve its existing
CommandLine object, registered flags and Usage function.

The lower interpreter cannot call Receiver.Enroll during os init: product
imports os and the receiver uses synchronization. Later binding instead takes
views of SAME original root/header/body/native records and SAME immutable
parsed bytes. A private provider push supplies these actual views to the
candidate-owned receiver after the product factory exists. Binding consumes a
single retained state transition; it must not rerun native calls, rebuild
originals from copied values, allocate replacement authority objects, or grant
a second receiver enrollment. Independent interpretation reads the original
immutable input and validates that binding and true last-borrower relation.
The exact provider declaration, ABI, GC/static pointer layout and wrapper
closure are required source, not an assumed linkname bypass.

All limits are source operands from the selected full-domain native backing:
raw units include their NUL; fallback raw is exactly one unit and module units
include their separate NUL. They are mutually exclusive layouts, not two
simultaneously maximal command lines. UTF-8 capacity must cover WTF-8 for every
allowed UTF-16 unit, parsed capacity cannot assume quote removal shrinks every
input, and the public string table must cover the maximum argument count of
the selected Go parser including empty quoted arguments. Derive exact constants
from the actual bound native constructor before bodies; wire lengths may only
compare against them, never size storage. Fixed stack/native callback/TLS costs
and overlapping full runtimes remain part of schema4 fit, not just these arrays.

## 362p: suspended debug-root construction distinction

Fresh primary ResumeThread remarks explicitly distinguish a debug event freeze
from the suspend count: a continue operation does not release threads held by
SuspendThread. CREATE_SUSPENDED supplies an initial primary-thread suspension.
This supports investigating a short preentry debug attachment, rather than
assuming permanent debugging preserves console control behavior:
CREATE_SUSPENDED|DEBUG_ONLY_THIS_PROCESS; obtain genuine create-process event;
retain original mapped image/process/thread views and perform qualified fixed
root write; Continue while the initial suspension remains; actually detach;
only then perform the owned final ResumeThread.

This is not a selected executable recipe. DebugActiveProcessStop documentation
does not itself close outstanding debug handle custody or prove every failure
and suspend-count transition. ResumeThread returns the previous count; zero,
minus one, greater than one and one have distinct real effects. No retry may
overwrite its original. Additional SuspendThread probes would themselves be
native effects with new records/count changes, not passive observations.
DebugSetProcessKillOnExit FALSE can be attempted only after a debug connection
exists; failure retains the original destructive creator-thread-exit relation.
No creator thread may disappear while that relation or unsettled debug events
remain. Complete image relocation/TLS/loader ordering, loaded-entry authenticity,
root write stability and partial failure settlement remain obligations before
this path can replace the independent assembly/private-eight route.

Primary API: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread
Primary creation flags: https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags
Primary suspend effect: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread

## 362q: independent PI handles survive the debug-root observation cut

Parent primary-source investigation answers the final-Resume handle dependency:
retain the original PROCESS_INFORMATION hProcess/hThread from CreateProcess,
independent of debug-event hProcess/hThread. Official process-handle lifetime
is until actual close. The final Resume and kernel wait must use these actual
PI objects and their own original records; no event-handle survival assumption
across DebugActiveProcessStop is required. Event handle postcall custody remains
UNKNOWN where the provider contract does not establish it, including failure;
that is retained debt, not eligible close/reuse or completed resource retirement.
The actual image event hFile remains independently owned under its documented
close rule, with retained image views/last borrowers tracked separately.

CREATE_SUSPENDED protects the primary thread until Resume, not a blanket claim
about every possible thread in the process. Actual selected image/TLS callback/
loader/provider/own-actor creation graph must show no relevant user actor can
race root writes or use streams before the gated primary entry. Official WinDbg
engine guidance about debugger-added suspend counts is informative but does not
replace the raw DebugActiveProcessStop provider postconditions. Thus the short
debug-root construction still has unresolved resource settlement and actor
closure; independent PI lifetime closes only one specific dependency.

Primary lifetime: https://learn.microsoft.com/en-us/windows/win32/procthread/process-handles-and-identifiers
Engine context, not raw API proof: https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/controlling-processes-and-threads
Raw detach contract: https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-debugactiveprocessstop

## 362r: direct lower-package import-pointer construction is compiler supported

Actual compiler noder.go311 onward explicitly permits go:cgo_import_dynamic
for general use. A quoted safe library argument is validated and recorded;
other cgo directives have different restrictions. Thus lower overlay source
can declare its own distinct pointer symbols and import directives for the
closed native bootstrap API roster, rather than linkname-borrowing runtime's
private dynamically imported variables. This does not require CGo activation,
a runtime import cycle, lazy DLL lookup, installed toolchain mutation or an
uninitialized runtime.stdcall address. Every selected export/library/argument
count remains explicit reviewed source; no peer-supplied function pointer is
accepted.

Actual linker setCgoAttr reads local symbol, remote name and library, assigns
SDYNIMPORT for accepted undefined/BSS symbols and records the library/export.
Actual PE initdynimport admits only reachable SDYNIMPORT symbols. It parses
remote %n decoration as argument count times pointer size and strips it for
actual export lookup; the internal-link branch assigns these pointers interior
slots in .windynamic. The entry assembly's strong data relocations therefore
must retain the exact pointers it reads. Declaring imports without reachable
relocations is insufficient. Existing runtime/os_windows.go explicitly records
that its analogous native pointers are initialized by the Windows executable
loader before Go begins; corresponding actual lower imports must be included
in the final mapped-image/provider/loader source closure, not treated as a new
authentication premise.

Assembly loads each fixed imported function pointer into a fixed StdCallInfo,
sets its exact N and source-owned Args address, then calls the existing lower
StdCall ABIInternal with AX pointing at that structure. Existing StdCall moves
AX to native CX and enters asmstdcall. No Go wrapper is called before TLS.
Every native return R1/R2/Err remains in the SAME preowned record before any
projection; the actual helper clears LastError0, which is an explicit seam
behavior, not claimed transparency for arbitrary original API calls. Full
register/flags/entry stack preservation, NOSPLIT closure and native provider
stack costs from362c/d/j still apply. This is supported source construction,
not complete API roster, selected root, successful link or execution evidence.

Inspected source hashes:
cmd/compile/internal/noder/noder.go C72DCECA3A06FF0C6AF301EFBFDCD094C963D05F43A26EC4CCB25C388C213A50
cmd/compile/internal/noder/lex.go 6C7DDB7DB0C27B7CC4851ED80092FCD5D557305DFA7AAEC5AD01C8AF83836077
cmd/link/internal/ld/go.go 1035A2C6CA034D8A47AE6EE3A4BE530E0B32170120D527D75FC46DF29BE68C84

## 362s: separate module fallback backing needs its own proven extent

Fresh native production-source search finds original_capacity and
module_capacity only as supplied constructor operands, not an actual generated
full-domain Windows binding. The32767 raw fixture does not supply that binding.
Selected358 already specifies raw2*C, WTF8/Parsed3*(C-1), string headers floor(C/2)
for C32767; keep those exact source operands. The fallback explicitly has
separately source-qualified module backing, not an assumption that the command
line limit qualifies the executable-name result.

Actual os/executable_windows.go grows query storage from1024 by1024 until a
GetModuleFileName result is below its bound, or returns empty on genuine error.
Both this fallback and the raw command line ultimately use the installed
syscall.UTF16ToString WTF-8 rule; there is no replacement-Unicode discrepancy.
Primary GetModuleFileNameW documents caller-size truncation and original loaded
name format, but does not itself state an exact global module-name maximum.
The primary path-limitation page calls32767 approximate because prefix expansion
changes length. Therefore those two pages alone do not justify silently binding
module capacity to C, nor truncating fallback to MAX_PATH. A complete candidate
must derive the actual selected module-name provider/loaded-image bound and
preown corresponding UTF16/WTF8/Parsed/body storage, or provide another complete
source-qualified exact extent relation. Wire reports never fund storage. This
is unresolved source construction, not an operator-selected new public cap.

Actual os/executable_windows.go SHA336118C7F02EF311EA7901D86956290079A6683A6D807A52638A8FFE2A80E2F1
Actual native private_arguments_windows.c SHAFCF1931F65F7B5B9DCDE7671220447E03FD2CA78ABA9D4D24BF83A60C7F598DE
Primary API: https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamew
Primary path wording: https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation

## 362t: os.init must retain actual private runtime argv before publication

Clarification to362o: replacing the public result at the os.init seam does not
permit dropping actual private runtime argv construction and backing. Fresh
ENTIRE native_arguments_windows_source.go confirms WindowsNativeArguments.Raw
is the actual runtime argv roster; its observer requires exactly eight strings
and retains their original aliases, never authority. Selected358 separately
charges original Go runtime private argv storage. Reconstructing an eight-string
roster later from numeric fields would fabricate that original occurrence.

Concrete ordering is: the reviewed os.init executes/retains its genuine original
private GetCommandLine/native result and private parser occurrence; lower root
captures the SAME returned private Args roster/backing; completed original-public
interpretation then replaces Args before this SAME init returns. There is one
public publication, with original private aliases still held through every
required borrow/representation cut. flag and terminal consumers importing os
therefore see the public binding, while the later candidate receiver observes
the original private roster and lower original observations rather than copies.
All native attempts at this seam remain genuine separately retained records.
The original Go parser's allocation/temporary extent is bounded by source-owned
fixed eight-argument private command construction, never by public wire lengths;
its full allocator/runtime/temporary cost still belongs in schema4 fit.

Alternatively an exact reviewed original-constructor hook could retain the same
actual roster without repeating the constructor, but neither deleting the
original nor creating a later proxy roster qualifies. Early pre-TLS ASCII locator
interpretation remains a distinct source occurrence used for authentication and
must not masquerade as this post-runtime private argv constructor. No body or
method-wide selection follows from the clarified ordering.

## 362u: exact cross-package assembly reference spelling

Actual assembler tokenizer permits U+2215 division slash and U+00B7 middle dot
inside identifiers; lex.go109–110 converts them to package slash and symbol dot.
Thus runtime entry overlay can use a direct strong reference spelled
internal∕runtime∕syscall∕windows·ServiceLassoBootstrap<ABI0>(SB), where the target
is the explicitly defined lower assembly ABI0 symbol. It need not enter a
compiler-generated Go wrapper or export a public product package API. Inside
that lower assembly, ·StdCall<ABIInternal>(SB) selects the actual existing
register ABI primitive, with AX holding fixed StdCallInfo. Both runtime and the
existing lower package have the inspected allowAsmABI permission. The compiler
ABI0 wrapper machinery is not the pre-TLS execution path.

This reference becomes part of the default rooted entry's direct relocation
closure under the reviewed internal-link command. Explicit NOSPLIT|NOFRAME at
entry and owned exact stack/register/flags save/restore remain mandatory; a
correct symbol spelling does not repair an implicit BP frame. Actual runtime
preempt_amd64.s shows NOSPLIT|NOFRAME, PUSHFQ and ADJSP source constructs, but its
post-runtime g/stack assumptions are not borrowed as entry proof. No generated
stack-check suppression or NOP-SP trick is proposed. Keep exact actual SP and
linker PCSP accounting, and restore original OS entry cells before the original
_rt0_amd64 JMP. Full native/FP/vector/flags preservation and entry/native stack
geometry remain source work, not an execution result.

Inspected tokenizer SHA448EC4E8AEA5ED423142B25F90CF32AF7C2B4E9059E68450381A8B75198BACDC;
actual cmd/asm/internal/lex/lex.go109–110 and asm/operand_test.go299–300 source
show the corresponding normalized qualified symbol forms (fixtures UNRUN).

## 362v: pre-TLS failure retention without trusting a gate locator

Investigate a genuine GetCurrentThread observation followed on SAME physical
thread by nonalertable WaitForSingleObject(self,INFINITE), using the reviewed
fixed import pointers/StdCall records. Primary GetCurrentThread permits its
pseudo handle wherever a thread handle is required, with THREAD_ALL_ACCESS;
it is neither inherited nor a close-owned real handle. Primary wait API accepts
thread objects, requires SYNCHRONIZE and distinguishes WAIT_FAILED. Primary
termination semantics signal the thread object when that thread terminates.
Thus this path does not depend on trusting an inherited gate before entry
authentication, or adding a handle/capability. It does not imply native success:
retain any actual failed return and full genuine call observations, never spin,
retry over a record, exit or claim disposal from it. A pseudo handle copied to
another worker refers to that other worker, not this pending thread.

Close exact selected provider, loader/impersonation/control/own-termination,
entry stack/native callback costs and kernel last-borrower correspondence before
crediting nonreturn behavior. No runtime scheduler/G/m, Go function wrapper,
window/message-loop creation or alertable APC path is introduced here. A retained
failure root plus pending wait remains live source/native custody; it cannot be
published as a completed LOCAL/FINAL or native join. Native independent views
must distinguish pending kernel borrowing from genuinely returned output cells.

Primary: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentthread
Primary: https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
Primary: https://learn.microsoft.com/en-us/windows/win32/procthread/terminating-a-thread

## 362w: preservation follows actual entry borrowers and native ABI

Clarify earlier full-state language: no universal volatile-register transparency
or XSAVE machinery is selected. Official x64 ABI makes XMM0–5, upper vector
portions and tile registers volatile; XMM6–15, x87 control and MXCSR6–15 are
nonvolatile. Actual StdCall touches XMM0–3 and observes XMM0 return, without
writing FP control. Native providers still owe their ABI preservation. Primary:
https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170

Actual bounded rt0_go175–365 borrows argc/argv through DI/SI supplied by the
original _rt0 stack cells, then overwrites GP/flags/BP during CPU/TLS setup.
No concrete incoming volatile-vector borrower was identified at this cut;
absence of a reset alone is not proof. Complete selected subsequent consumers
and provider ABI closure before credit. Save helper-clobbered SI/DI, original
BP/SP/cells and exact required GP/flags explicitly; preserve actual nonvolatile
FP/control requirements. Review exceptional unwind/failure paths separately.
Only introduce additional state machinery if an actual required borrower makes
it necessary, with its full source/native storage/stack costs. No body GO.

## 362x: early bad-CPU output and actual build configuration

Actual rt0_go365–435 calls check, args, osinit, schedinit, newproc and mstart.
Its bad_cpu path writes stderr through runtime.write and exits/aborts before
ordinary os.init, so later Args/provider binding cannot own this path alone.
Actual asm_amd64.s95–175 defines NEED_* feature checks under GOAMD64_v2/v3/v4.
Current producer/workflow source search finds no explicit GOAMD64 pin; the
provenance guard rejects GOFLAGS but does not itself fix GOAMD64/GOENV/
GOEXPERIMENT. Defaults alone therefore do not prove that early branch absent.

The complete recipe must close effective compiler microarchitecture/experiment
inputs and actual same CPU checks/borrowers, with retained genuine refusal, or
preserve/intercept that actual early writer/exit path and its original terminal
custody. No normal-supported-host assertion substitutes for this relation.
Pre-os TLScheck/abort remains independently real even when bad_cpu is absent.

Actual compiler amd64/ssa.go1296–1302 explicitly zeroes X15 and loads R14 from
TLS for a compiler ABI0→ABIInternal static Go call. This is concrete evidence
why the pre-TLS path must call the existing assembly StdCall ABIInternal directly,
not a compiler-generated wrapper. Later genuine runtime wrappers after TLS
remain part of ordinary initialized execution; their source use does not grant
the earlier path a valid G register. Source-only construction, no method GO.

## 362y — proposed broad amd64 CPU recipe and effective experiments (2026-10-11)

This is a concrete build-input proposal for parent disposition, not selected implementation, SOURCE_GO, execution, or native evidence. Existing selected platform scope is SELECTION.md:16 (direct Windows/Linux, Mac N/A), ACCEPTANCE.md:101 and :202 (scoped native OS and two-member assets); actual producer scripts/produce-scoped-native.mjs:43 names win32-amd64/linux-amd64. Existing release-scoped.yml:45–54 selects matching Windows/Linux hosts but pins only GOFLAGS empty/GOWORK off. No inherited GOAMD64 value qualifies this method.

Proposed reviewed recipe fixes GOOS to the matching Windows/Linux platform, GOARCH=amd64, GOAMD64=v1, CGO_ENABLED=0, GOENV=off, GOTOOLCHAIN=local, GOWORK=off, GOFLAGS empty, and GOEXPERIMENT empty. Explicit owned overlay and internal-link argv must be separately recorded and admitted; the source-qualified local toolchain binary/source/version/defaults and complete dependency inputs are held, hashed and bound to the producer and workflow. No fallback auto-download, ambient GOENV/config, host CPU tuning or v2/v3/v4 requirement is allowed. This retains broad amd64 support rather than introducing a higher CPU floor. The exact selected v1 preprocessing of runtime asm is required before excluding its conditional NEED_* bad_cpu branch; runtime TLScheck/abort and other genuine early write/exit paths remain independent obligations. No claim follows from merely setting an environment variable.

Effective GOEXPERIMENT is NOT 'none'. Actual internal/buildcfg/cfg.go:52–57 envOr treats empty as embedded default, and zbootstrap.go:15 currently embeds an empty default. Actual exp.go:55–153 then supplies the Windows/Linux amd64 baseline: RegabiWrappers=true, RegabiArgs=true, Dwarf5=true, RandomizedHeapBase64=true, GreenTeaGC=true; other flags remain false. amd64 regabi is forced on after overrides. 'none' zeroes baseline flags before that forcing and changes semantics. Admission therefore binds the actual embedded default and complete effective flag set, not the empty display string alone; a changed embedded default/toolchain is a new source candidate requiring review. These observations describe installed source, not proof that a future runner binary matches it.

Actual read/hash anchors: internal/buildcfg/exp.go EB81C42738C23056BA44561039C8EE09B337F045F852A1BF4BF81A24B5196B4D (entire read); cfg.go F22EB5691BD1299EC46246380917693244239E459021BAE776C58ABA8F523DB6 (bounded envOr/goamd64); zbootstrap.go 5D99D50D3085478B80C756853EF1F4142192F40162AFF3A278A2CA795470471A (default constant). All paths under C:/Program Files/Go/src. Actual effective producer environment and command provenance must be captured in the later admitted qualification, with finite runtime/stack/TLS/GC geometry reflecting this baseline. No target command was run.

## 362z — held tool root, FIPS baseline and source macro closure (2026-10-11)

Parent disposition of 362y selects the broad-v1/baseline build-input construction direction only, not whole method, body, provider, CPU-unreachable, fit or execution GO. Amend its concrete recipe to explicitly bind GOROOT to the independently held exact toolchain root and GOFIPS140=off. GOENV=off does not erase process environment. Actual buildcfg/cfg.go:23 caches ambient GOROOT; :37/:74–84 independently select FIPS input. Actual zbootstrap.go:21 embeds off. An empty FIPS variable alone would permit a changed embedded default; explicit off preserves this inspected baseline. Tool executable, pkg/tool binaries, source, include headers, go.env and overlay backing must belong to the same admitted held tool root; path spelling/version echo alone is insufficient. No installed toolchain mutation is proposed.

Actual cmd/go/internal/work/gc.go:327–349 builds asm argv with -D GOOS_<platform>, GOARCH_amd64 and exactly GOAMD64_<cfg value>, alongside forced/package asm flags. Actual cmd/asm/internal/lex/input.go:37–68 predefines those -D macros; :370–380 handles #ifdef by actual macro presence. Runtime asm_amd64.s:95–174 defines NEED_* only under GOAMD64_v2/v3/v4. Its :308–370 branches to bad_cpu only under those NEED_* conditionals. Thus the proposed v1 source relation excludes those guarded branches only when admitted argv/source/include/overlay closure proves no additional macro injection or replacement; package -asmflags/-toolexec and forced args are part of that closure. The actual bad_cpu body remains present, and other early runtime failures are not excluded. No preprocessor or assembler was executed.

Read/hash anchors: cmd/go/internal/work/gc.go 7B91523777A11587412B7D4E35F965D771CA7BDE60CA6D65D54206171544819F (bounded asmArgs); cmd/asm/main.go 3566341D4A978154403412B3A65D475CD5E32C89DBDB0A4970DCBEAB873C2B7B (first125, includes additional GOEXPERIMENT macro injection for AllowAsmABI); cmd/asm/internal/lex/input.go 2E9C95601E13798CD1FFE2B2A27277C40552A8703724C28708B0B6AFBE301DB7 (bounded predefine/ifdef). Same held installed source root as 362y. Full dependency/overlay/source method remains active and incomplete.

Parent method record ENTIRE read/hash verified: D:/projects/service-lasso/service-lasso/audit-evidence/continuation-20260930-1520/parent-tui-build-input-method-selection-20261011.md SHA256 CAB83E4BE83D88EB9819DDCAF322E9174C50D55ADF209C49D27BA68D4F5D13CF. It selects the narrow 362y construction direction plus held GOROOT and explicit reviewed effective FIPS baseline; it does not waive any existing product FIPS profile. Current inspected baseline is off; any contrary actual product requirement must be preserved and its exact profile reviewed before bodies. It grants no whole source, body/provider/native/fit/ROOT/execution or publication authority. Actual producer/workflow propagation remains owed.

## 362aa — concrete split root and later adoption signature construction

Keep two distinct storage classes in the existing physical lower package. The pre-TLS root is pointer-free fixed native cells/byte arrays, not a Go slice/string/interface/function value; assembly accesses only its source-owned offsets and retains entered/returned native originals there. The post-runtime retained owner is an ordinary typed Go global holding the ACTUAL private command-line string, ACTUAL constructor's []string roster and its backing strings, and the subsequently decoded public []string/backing. These typed references are assigned only after ordinary runtime initialization, with normal GC barriers, and remain reachable through true last public borrower. NOPTR must never be used to hide those Go references. The pre-TLS assembler cannot publish fabricated Go slice headers into that owner.

The os.init overlay preserves its actual original query/UTF16-WTF8/parser/fallback occurrence first, assigns that result to private retained ownership, then invokes the lower public decoder on SAME authenticated native UTF16 originals. Only successful independent interpretation publishes os.Args before os.init returns. Public zero-count is retained as zero-count; private eight-count and public count are distinct. Original private native queries/errors and actual constructor backing stay retained even if public interpretation fails. A later product check must consume SAME originals and must not re-query GetCommandLine, re-run private parsing, or manufacture a replacement private roster.

The private provider-push adoption seam can use a zero-argument function returning a source-owned typed adoption record containing pointers to fixed native cells plus the actual private/public string slices, with identical declaration types on both sides established in the reviewed source. Its provider-side marked linkname and product declaration remain under normal linker checks (362h); they cannot import product lifetime into stdlib. A source-owned once-only state is occurrence control, never creator authority. The record is borrowed, not transferred/cleared: lower typed globals remain roots, product keeps actual retained references, and every borrower joins the same lifetime. Function entry must refuse before decoder success and before complete original-root/owner association, retaining failure, rather than constructing a usable record from mere nonzero locators. Exact function signature/type-layout/compiler ABI, real provider marking/deadcode/relocations, failure custody and independent interpreter source need whole-method review before bodies. No unsafe cross-package struct-layout guess or callback installed before runtime is proposed.

This closes the concrete GC/lifetime construction constraint independently of still-open first-entry creator/loaded-code authentication, original-console operation routing, root locator, loader/TLS failures and native fit. It is not selected decoder duplication or a new authority capability. No source body, generator, parser or compiler was run.

Concrete signature refinement for 362aa: use a matching private declaration `func adopt() (nativeRoot unsafe.Pointer, privateCommand string, privateArgs []string, publicArgs []string)` on provider and consumer, rather than separately named cross-package record types. These exact builtin string/slice/unsafe.Pointer types avoid a guessed foreign record layout; ordinary initialized Go compiler handles the matching return ABI. Product constructs its own typed borrowed record from those actual returned references. This signature still needs actual compiler/linkname source review and full return/GC/lifetime fit; it is not a pre-TLS call, authority grant, public API or permission to execute. NativeRoot is an address of already authenticated original storage, never authorization merely because nonnil. Public decoding output and actual os.Args must retain SAME backing, with no second copying constructor substituted.

## 362ab — public fixed-buffer decoder versus private genuine constructor

Actual os/exec_windows.go:94–102 is the genuine private constructor seam to retain unchanged as the first occurrence; :113–173 readNextArg/commandLineToArgv allocate append-backed bytes/strings/slices. Running that allocator-based parser a SECOND time on original public text is not the selected fixed UTF8/Parsed/Public-view construction. The public seam instead uses source-owned disjoint fixed byte/header views and immutable string views, preserving the current windows_argument_decode.go grammar (WTF8 conversion BEFORE Go-quote parsing, including double-double-quote toggling), while the actual private constructor remains genuine and retained. The product current receiver-bound interpreter cannot execute in os.init because it imports errors/binary/Receiver/mutex-bearing ownership. Its grammar/core operations must be represented in the reviewed lower package without those imports, with actual native geometry/once state and failure originals; product independent interpretation remains a separately retained check of SAME completed native input and actual resulting Args, never reconstructed authority.

Current decoder is 100 lines read entire in this cut: geometry requires disjoint Body/UTF8/Parsed/Public backing and SAME locator; Header/BodyExtent exact framing/NUL/count; UTF8 and Parsed exhaustion/partial cells retained; Public strings alias actual Parsed backing. Those obligations carry into lower root; they cannot be replaced by the typed-global GC construction in 362aa. Typed globals root actual private allocation references and public string slice/backing after runtime, while the native/fixed disjoint view geometry remains an additional obligation. Original private constructor allocations must be included in finite runtime/GC fit rather than hidden behind fixed public buffers. Any selected fallback module extent still requires the actual provider/source bound (362s), not automatic reuse of raw command-line capacity.

This is an explicit source mapping before dependent bodies. It does not select copying an unreviewed parser into stdlib, replace independent interpretation with equality echoes, or grant whole-method review/execution. Full lower init graph, actual prefix/lastuse/cancel/ACK and all public borrower joins remain owed.

## 362ac — finite ambiguous debug-handle retention can permit source progress

Parent proposed evaluating retained event-handle UNKNOWN occurrences rather than requiring undocumented DebugActiveProcessStop retirement before resume. Operative ACCEPTANCE.md:13–23 retains unresolved mixed graphs/runtime/outer control and charges; :26–40 requires required outer originals across FINAL archiveACK. No inspected clause requires event-handle retirement BEFORE child resume. Therefore a finite conservatively charged no-use/no-close/no-reuse occurrence may survive detach while independently returned PI handles drive actual resume/wait. This is a construction inference, not successful detach/settlement proof.

Primary ExitProcess remarks54–69, freshly read at https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitprocess, order process handle closure before process-object signaling. DLL detach can block; child processes are not automatically terminated, and process object deletion depends on last handles. Thus a genuine independently observed exact creator process natural termination can potentially bound retirement of that creator's handle-table references. It does not prove deletion of all referenced kernel objects or settle duplicates/other borrowers.

The creator cannot self-observe its own process termination or use predicted exit to grant early FINAL. A real outside observer must retain exact original occurrence associations/native process handle and join actual creator termination, with actual loaded provider/own writer/family/lateACK/terminal custody and final handle retirement connected. If existing selected outside custody can supply this relation without new authority or an extra inherited child handle, retained ambiguity need not block child startup; if it cannot, UNKNOWN remains live and prevents public success. Exact existing observer construction and charges must be mapped before adopting the debug-root method. Detach failure, debug-event continuation/suspend counts, control semantics and before-entry own loader/TLS actors remain independent gaps. No permanent debugger substitution, forced exit, body grant or execution follows.

## 362ad — concrete debug-located fixed-root construction, actors and terminal owner

Candidate build relation: link the Go payload FIRST with the owned lower pointer-free root, strong default-entry assembly relocation and internal/no-strip recipe. Parse its exact COFF symbol/section table under admitted build commands to derive that exact root symbol's RVA and fixed source extent; emit an owned native-owner build input, then build the native owner SECOND. This needs no third package member, extra inherited child handle, public flag/env locator or operator-supplied root address. The two selected package members remain unchanged. A symbol/RVA/hash is a locator/binding input, never creator/entry authority.

Actual installed linker pe.go SHA268658672120085BDF42D5FF59E7DA4A510B058F83331A4D0AE468772ABC4602: :697–711 writes name/string-table-offset, section-relative value, section, type/class and zero aux entries; :717–747 maps internal data including BSS to .data section; :840–868 includes reachable named data unless AttrNotInSymbolTable; :879–901 emits symbols only when not stripped for internal linking. Therefore exact recipe must forbid -s, prove root reachable/nonhidden-to-symbol-table, unique exact symbol spelling, checked string/symbol/section extents, root RVA plus complete fixed extent within writable mapped section/SizeOfImage, correct image architecture and exact payload bytes. It cannot use adjacent symbol difference as root size or assume root has file-backed bytes. Actual parser/emitter and producer/workflow integration are still owed, not executed.

Proposed source sequence and occurrence ownership:

| Cut | Actual actor/native originals | Required next relation and retained failure |
| --- | --- | --- |
| Before birth | Existing independent native owner, genuine original caller/deadline, selected Job/endpoint/gate/image/collector/profile | Exact ordinary stream operation routes and full root/provider/loader/own-writer closure already held; no copied claim grants entry |
| Birth | SAME creating OS thread enters CreateProcessW with Job-list/exact two inherited handles, CREATE_SUSPENDED plus DEBUG_ONLY_THIS_PROCESS; retains PI and every argument/output | Debug freeze and primary suspend count are separate; only primary suspend is promised; no unrelated worker may Wait/Continue |
| Create event | SAME thread retains actual WaitForDebugEvent return/error and complete fixed DEBUG_EVENT, native image hFile, mapped lpBaseOfImage and event handles | Bind actual event process/thread to retained birth and held exact mapped image/provider graph; lpStartAddress is not entry authority; all event outputs/partial failures remain original |
| Root write | Owner uses independent PI process handle and checked actual event base plus source-derived RVA; source-owned fixed root input and exact WriteProcessMemory result/byte count retained | Exact full write and independently read-back native originals, expected zero/root state and no relevant intervening own loader/TLS/provider writes; pre-entry child hook verifies actual conjunction before reading bootstrap stream |
| Continue/detach | SAME creator thread enters ContinueDebugEvent for actual event while primary remains independently suspended, then DebugActiveProcessStop; each result/error retained | Failed/unknown result retains creator/debug connection/process/thread/root and default kill-on-thread-exit debt. Successful Stop does not invent event-handle retirement |
| Resume | Only independently returned PI thread handle drives actual ResumeThread after complete root/actor/provider/stream readiness | Check genuine returned count against actual creation/continuation relation; mismatch denies useful entry and retains originals. No extra suspend probes or arbitrary guessed count |
| Public run | Child explicit NOFRAME ABI0 entry consumes fixed root through reviewed direct StdCall closure, preserves entry state then normal rt0; SAME endpoint carries selected original argv phase and later typed operations | Genuine root/creator/loaded-entry association precedes wire reads; startup failures/worker/cancel/native completion/console semantics and finite fit remain mandatory |
| Child natural zero/FINAL | Existing native owner observes exact child root/whole Job family and raw EOF; selected independent archive reader/late ACK retains required outer originals | Ambiguous event occurrences stay UNKNOWN/no-use/no-close/no-reuse, conservatively charged through creator lifetime; this does not authorize FINAL or disposal by itself |
| Creator natural terminal | Already-required genuine outer observer/late-ACK chain retains exact creator process and occurrence associations and independently observes natural termination | ExitProcess handle-table closure can discharge only calling-process references under actual provider/order; distinct kernel refs/aliases/other actors stay separately held/charged; creator cannot self-certify termination |

The actual existing Windows source does not yet implement this sequence or the complete outer observer/late-ACK terminal chain. The table is a connected construction candidate, not a renamed helper or completed method. It removes undocumented pre-resume event-handle retirement as an invented prerequisite while preserving final settlement. Exact root write/read authority, loader zero/relocation/TLS ordering, creator-loaded-image authentication, Suspend/Continue/Stop source/provider behavior and fixed debug-event capacity remain real source/native qualification obligations. No permanent debugger, forced termination, extra child inheritance, execution or body grant follows.

## 362ae — preserve incremental module query originals and projection

Fresh actual ENTIRE os/executable_windows.go read plus internal/syscall/windows/zsyscall_windows.go:348–355 confirms fallback starts n=1024, creates a new []uint16 at each n, and advances n by1024 whenever returned count equals n. The wrapper makes an error only for count zero; nonzero truncated count with raw ERROR_INSUFFICIENT_BUFFER remains nil error and retries. A single large native query is therefore not asserted equivalent to this actual ordinary sequence. Current private_arguments_windows.c:88–93 performs one full-capacity query and exhaustion-refusal; complete source repair must replace that behavior under this source mapping, not merely enlarge a fixture.

Concrete finite construction once the actual selected loaded-provider/module-input bound M is proved: preown K query occurrences with n_i=1024*(i+1), where K is the checked least positive integer with 1024*K strictly greater than the maximum successful returned module units. Each occurrence retains its actual supplied n_i, disjoint WCHAR backing of n_i units, entered/returned/result/raw error and projection; total backing is checked 1024*K*(K+1)/2 WCHAR units plus native frames. This is a derivation, not a selected numerical cap or fit. Do not reuse one buffer across truncated-query originals or overwrite the previous native call cell. Call only while preceding actual count equals preceding n_i and no error projection occurred. Count zero yields original public empty-one-arg fallback; count below n_i requires actual terminator/prefix validation and WTF8 preserving projection. Exhausted roster or impossible provider count retains graph and denies qualification; it is not authorized truncation of an ordinary supported path.

The fixed native owner acquisition and genuine private os.init query/parser allocations remain distinct source occurrences. Actual raw-error clearing/native wrapper semantics must be recorded at each applicable source seam; success-error projection cannot erase raw errors, and a copied value cannot become a new genuine query. Full module source domain/provider bound is still UNKNOWN; raw argv32767 and approximate maximum-path documentation do not select M. Preserve actual provider/path public outcome behavior and all intermediate originals, then include native buffers and private Go allocations in complete fit. No body/target execution or selected bound follows.
