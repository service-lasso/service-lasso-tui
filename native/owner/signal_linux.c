#if defined(__linux__) && defined(__x86_64__)
#include "signal_linux.h"
#include <sys/syscall.h>
#include <signal.h>
_Static_assert(sizeof(sl_linux_kernel_action)==32,"x86_64 kernel action ABI");
static void initialize(sl_linux_call *call) {
    atomic_init(&call->entered,0);atomic_init(&call->returned,0);
}
long sl_linux_write_signal_begin(sl_linux_write_signal *signal) {
    long result;
    if (!signal || signal->attempted) return -1;
    signal->attempted=1;
    initialize(&signal->thread);initialize(&signal->disposition);
    initialize(&signal->block);initialize(&signal->pending_before);
    initialize(&signal->pending_after);initialize(&signal->restore);
    signal->block_set=UINT64_C(1)<<(SIGPIPE-1);
    result=sl_linux_call6(&signal->thread,"controller.public-write.thread",SYS_gettid,0,0,0,0,0,0);
    if (result<0) return result;
    /* Query and preserve disposition; never silently change it or install an
     * inherited global ignore policy. This operation belongs to the actual
     * outside controller after producer birth, not the child bootstrap. */
    result=sl_linux_call6(&signal->disposition,"controller.sigpipe.disposition",SYS_rt_sigaction,
                         SIGPIPE,0,(long)&signal->original_disposition,8,0,0);
    if (result<0) return result;
    result=sl_linux_call6(&signal->block,"controller.sigpipe.block",SYS_rt_sigprocmask,
                         SIG_BLOCK,(long)&signal->block_set,(long)&signal->original_mask,8,0,0);
    if (result<0) return result;
    signal->blocked=1;
    result=sl_linux_call6(&signal->pending_before,"controller.sigpipe.pending-before-write",SYS_rt_sigpending,
                         (long)&signal->pending_before_bits,8,0,0,0,0);
    return result;
}
/* Called at this SAME thread's genuinely settled source/borrower cut, after
 * its last write and actual source join. The thread's restore/return/native
 * join precede overall archive FINAL, so FINAL cannot authorize this call.
 * Actual caller enforces source-specific original obligations; this helper
 * grants none. Async signal exclusion/restore race remains a genuine native
 * qualification premise, never established by a pending query alone. */
long sl_linux_write_signal_finish(sl_linux_write_signal *signal) {
    long result;
    if (!signal || !signal->blocked || signal->finish_attempted) return -1;
    signal->finish_attempted=1;
    result=sl_linux_call6(&signal->pending_after,"controller.sigpipe.pending-after-write",SYS_rt_sigpending,
                         (long)&signal->pending_after_bits,8,0,0,0,0);
    if (result<0) return result;
    /* No drain: a bit alone cannot identify our write's signal versus another
     * original producer. Unblocking a pending SIGPIPE could destroy the live
     * owner before retained graph disposition. Preserve actual blocked mask
     * and runtime for genuine authorized recovery instead. */
    if ((signal->pending_after_bits & signal->block_set) &&
        !(signal->original_mask & signal->block_set)) return -1;
    result=sl_linux_call6(&signal->restore,"controller.sigpipe.restore-mask",SYS_rt_sigprocmask,
                         SIG_SETMASK,(long)&signal->original_mask,0,8,0,0);
    if (result<0) return result;
    signal->restored=1;
    return result;
}
#endif
