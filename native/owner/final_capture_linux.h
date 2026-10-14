#ifndef SERVICE_LASSO_FINAL_CAPTURE_LINUX_H
#define SERVICE_LASSO_FINAL_CAPTURE_LINUX_H
#include "owner_linux.h"
#include "candidate_profile.h"
struct sl_linux_public_controller;
struct sl_linux_source_cut;
struct sl_linux_gate_delivery;
struct sl_linux_archive_file;
struct sl_linux_archive_directory;
typedef struct sl_linux_final_record {
 void *source_original;
 const unsigned char *body;
 unsigned long length,offset;
 unsigned char header[32];
 unsigned char observation[80]; /* closed native-call scalar representation */
} sl_linux_final_record;
typedef struct sl_linux_final_io {
 sl_linux_call *calls;unsigned long capacity,count,completed;
} sl_linux_final_io;
typedef struct sl_linux_final_capture {
 struct sl_linux_public_controller *controller_original;
 struct sl_linux_public_controller *prepared_controller;
 struct sl_linux_source_cut *cut_original;
 struct sl_linux_gate_delivery *gate_original;
 struct sl_linux_archive_file *file_original;
 struct sl_linux_archive_directory *directory_original;
 sl_candidate_profile *candidate_original;sl_candidate_span candidate_body;
 sl_linux_final_record *records,*actual_records;
 unsigned long capacity,actual_capacity,count,extent;
 unsigned char *readback,*actual_readback;unsigned long bytes,actual_bytes;
 sl_linux_final_io writes,reads,actual_writes,actual_reads;
 sl_linux_call flush,directory_flush,eof;
 unsigned char eof_byte;int file_fd,directory_fd;
 long result;unsigned prepare_attempted,prepared,attempted,returned,readback_equal;
} sl_linux_final_capture;
long sl_linux_prepare_final_capture(sl_linux_final_capture *root);
long sl_linux_persist_final_capture(sl_linux_final_capture *root);
/* Typed records currently include LOCAL/raw/root/family/candidate and the
 * entered installation/private-payload and membership constructor segments. Kinds7/8/9 are
 * LE80 root/lift scalars, original admitted-ABI outputs, and exact path spans;
 * stream UINT64_MAX-3 identifies installation; UINT64_MAX-4 identifies
 * membership, whose kind10 retains exact returned raw read bytes/EOF/error.
 * Stream UINT64_MAX-5 retains mount constructor/native read states and kind11
 * exact original mountinfo bytes, with read offsets/counts bound before output.
 * Stream UINT64_MAX-6 retains target/procs constructor/lift/tuple states,
 * original native calls and statfs/statx/open_how ABI observations.
 * Stream UINT64_MAX-7 retains SELinux snapshot/status/lift/sample states,
 * original calls and ABI outputs; kind12 retains actual per-policy raw frame
 * bytes (stream=policy index), compared with SAME retained parser input body.
 * Stream UINT64_MAX-8 retains the SAME owning gate result, signal state, actual
 * native calls, original eight token bytes and original signal-action ABI.
 * Stream UINT64_MAX-9 retains settled entry/stdio/lift/socket states and calls,
 * original peer-label bytes (kind13) and sockaddr ABI. Embedded entry spans
 * are checked against capture metadata and all four actual output rosters.
 * Stream UINT64_MAX-10 retains shared bootstrap, distinct clone results,
 * closed FD actions, capability/namespace/native calls and credential status
 * bytes (kind14), groups and ABI outputs. Successful exec nonreturn stays
 * entered/unreturned; these records never manufacture an exec return.
 * Stream UINT64_MAX-11 retains full-EOF FD discovery/map states, each native
 * flag observation, original getdents bytes (kind15), original procfs ABI and
 * exact action-to-original-view indices, separately from shared child outputs.
 * Stream UINT64_MAX-12 retains six original namespace views, independent
 * live brackets, clone-target observation and raw pidfd fdinfo body (kind16),
 * native reads/EOF and original identity/namespace/procfs ABI outputs.
 * Stream UINT64_MAX-13 retains terminal/fallback/PTY and three stream-route
 * states, source-present returned calls, original and derived termios ABI.
 * Stream UINT64_MAX-14 retains single-channel/LOCAL IO and full-roster
 * attempted requests, native calls, exact prefix/response/readback bytes
 * (kind17), with original local-prefix index and separate completed extents.
 * Stream UINT64_MAX-15 retains request mode/window/restoration and input
 * start/body/native-join/interpretation states, exact input bytes(kind18),
 * every used native poll/read/write step and original pollfd ABI.
 * Stream UINT64_MAX-16 retains paired raw/forward native joins and distinct
 * interpretation, release lineage, pthread outcomes/notifications, actual
 * forward waits/iterations and per-thread SIGPIPE native state/original ABI.
 * Stream UINT64_MAX-17 retains the fixed scheduler source-kind/index map,
 * generation/claim/native-return distinctions and actual observed waits;
 * future reader notification originals remain live outside this prefix.
 * Stream UINT64_MAX-18 retains constructor/body/collector native join
 * outcomes, each attempted fixed-roster join and separate LOCAL/channel
 * interpretation, with original successful-join index and earlier EBUSY.
 * Stream UINT64_MAX-19 retains every returned service and family observation,
 * exact roster indices, native joins and family read bytes (kind19).
 * Service headers use frame=service index, raw=subrecord identity.
 * Family interpretation completion follows the SAME returned synchronous
 * service call; native return alone does not establish that completion.
 * Stream UINT64_MAX-20 retains every attempted service raw/forward join
 * and release notification, including prior EBUSY and failed release. Headers
 * carry service index/kind; bodies carry worker and referenced-join indices.
 * Stream UINT64_MAX-21 retains returned parser init/read/errno/cursor,
 * memory owner and every memory attempt/prewrite, exact previous allocation
 * indices, after-parse status and source spans(kind20). Pointer scalars remain
 * original observations, not a relocated or transferred parsed graph.
 * Kind24 retains exact current owned DUPLICATE/BOUNDED_DUPLICATE result
 * spans joined to actual arena offset/extent; it never rereads expired inputs.
 * Stream UINT64_MAX-22 retains parser diagnostic frames, original format
 * spans(kind21), used typed argument ABI(kind22), and actual finite output
 * backing(kind23). Output capacity is not claimed as produced byte count;
 * formatter return/error remains separate, as do argument referent lifetimes.
 * Streams UINT64_MAX-23..-26 retain the eight containing context rosters,
 * their diagnostic occurrences, typed context allocation and MLS node bodies;
 * copied diagnostic referents use kind28(policy)/29(context), never expired
 * original aliases or scanned extent as fictitious allocated bytes.
 * Stream UINT64_MAX-27 retains all14 closed traversal category/root/parser
 * coordinates; UINT64_MAX-28/kind30 retains actual visited AVTAB nodes.
 * UINT64_MAX-29/kind31 is actual helper occurrence metadata only.
 * UINT64_MAX-30 states join helper typed referents(kind32), exact source-time
 * name/key extents(kind33), and consumed hash/AVTAB pointer slots(kind34).
 * Kind32/33/34 headers use stream=traversal ordinal, frame=helper ordinal,
 * raw=typed field/name-or-key/hash-or-AVTAB index respectively. State bodies
 * separately retain parser allocation index/interior offset or actual source
 * descriptor index/slot and table bucket count/index. ULONG_MAX-1 denotes
 * checked embedded policy coverage; ULONG_MAX-2 only the exact validated
 * named static array, not arbitrary HASH names. Dynamic names require actual
 * owned parser allocation. These sentinels are ABI observations, not trust.
 * UINT64_MAX-31 states/kind35 retain actual class/permission/common and
 * source/target bitmap headers: stream=traversal ordinal, frame=field, raw=0,
 * checked allocation/interior offset, current original body. Deeper symbol,
 * constraint, datum and bitmap graphs require their own source relations.
 * Live root and
 * descriptor identities remain independently held, never serialized transfer.
 * This is a capture component only, not the complete immutable prefix,
 * native disposition, seal, independent archive acknowledgment or success. */
#endif
