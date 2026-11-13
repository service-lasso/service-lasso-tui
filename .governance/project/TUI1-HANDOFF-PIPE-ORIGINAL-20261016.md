# Issue1 actual handoff pipe original

Pre-code internal boundary selected by the parent under GOV14, mapped to
SPEC004 retained child/stream originals and issue1 source-test verification.
This adds no public CLI flag, environment variable, action or native ABI.

Exact57e and8aae Windows natural CI both failed the SAME EOF fixture with the
unchanged30s handoff timeout. The second route did not advance that boundary.
Pinned Node22.23.2 libuv `win/fs.c`620..645 now supplies a materially new source
constraint: close of fd0..2 returns success without closing the descriptor.
`win/pipe.c` duplicates those stdio handles, so ending a writable duplicate
cannot by itself establish closure of the original descriptor. This is source
mechanism plus missing observed EOF, not a native handle census.
https://github.com/nodejs/node/blob/v22.23.2/deps/uv/src/win/fs.c
https://github.com/nodejs/node/blob/v22.23.2/deps/uv/src/win/pipe.c

## Selected API and correspondence

`observeNativeProducerChild(child)` privately snapshots the actual original
stdin/stdout/stderr and every extra `child.stdio` pipe at its existing
observation point immediately after spawn. Aliases are deduplicated by SAME
actual stream object before installing listeners. Each private row retains
the original descriptor indices, readable/writable direction and every actual
error/end/finish/close observation. Public `owner.pipes` remains a frozen
projection, never a resolver or capability. Once-close counts belong to these
actual rows; aliases cannot install duplicate observers or issue duplicate
close requests. The observer does not itself close any pipe.

An internal `resolveNativeProducerHandoffPipe(child, selected)` resolves only
from the already-observed SAME child's private original snapshot. An omitted
selection resolves its original stdout. An explicit actual pipe must match a
private readable row and must not alias stdin. Foreign child pipes, missing
pipes, stdin and streams later added to a mutable `child.stdio` array refuse
before handoff listeners. This is JS original-object correspondence, not OS
identity, readiness, issuer or closure authority.

`readNativeProducerHandoff(child, selectedPipe)` factors the SAME existing
30s/1MiB/strict-parse/listener/settlement implementation over that resolved
original. The production call stays `readNativeProducerHandoff(child)` and
therefore fixed to the actual observed original stdout. Child exit/error
listeners still belong to the SAME actual ChildProcess. No surrogate child,
stdout reassignment, synthetic end or private handle operation is permitted.

## Owned fixture and failure cuts

The original stdout positive/quota/parser tests remain on their existing
descriptor. Non-Windows natural stdout EOF retains its passing closeSync1
route. Only the Windows EOF fixture acquires an additional actual inherited
pipe3, and the actual child closes fd3 with `closeSync(3)`, whose libuv close
path is real. The parent explicitly selects the SAME privately observed pipe3.
Stdin remains live. The parent must observe actual selected-pipe EOF, no ready
record, no child exit/close and no retained readiness listener. It then releases
stdin once and awaits the SAME child's successful terminal closure, including
all extra pipe errors, readable EOF, writable finish where present and actual
single close. An early EOF never hides a later child/pipe failure.

Negative source cases cover foreign pipe, absent selection, stdin alias,
invalid/non-pipe selection and late-added/mutated descriptor array. Alias
deduplication and real extra-pipe closure use actual child/stream originals;
public projections cannot manufacture a matching private row. Every owned
child stays held through input release and completion in failure cleanup.
Handoff errors, child errors and cleanup errors remain actual failures.

Both prior EOF failures remain recorded. This proposed route is UNRUN and
neither source review nor source semantics claims an actual Windows pass.
Fresh entire-patch independent review and new complete-input/exact-command
ROOT admission remain required before any local target execution. Natural
push CI is separate; no rerun/cancel/skip/timeout relaxation follows.
