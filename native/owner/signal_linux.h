#ifndef SERVICE_LASSO_SIGNAL_LINUX_H
#define SERVICE_LASSO_SIGNAL_LINUX_H
#ifdef __linux__
#include "owner_linux.h"
/* Linux x86_64 kernel ABI, not libc sigaction/sigset_t. Loaded tool/ABI and
 * actual single controller thread ownership remain qualification inputs. */
typedef struct sl_linux_kernel_action {
    unsigned long handler,flags,restorer;
    uint64_t mask;
} sl_linux_kernel_action;
typedef struct sl_linux_write_signal {
    sl_linux_call thread,disposition,block,pending_before,pending_after,restore;
    sl_linux_kernel_action original_disposition;
    uint64_t block_set,original_mask,pending_before_bits,pending_after_bits;
    unsigned attempted,blocked,finish_attempted,restored;
} sl_linux_write_signal;
long sl_linux_write_signal_begin(sl_linux_write_signal *signal);
long sl_linux_write_signal_finish(sl_linux_write_signal *signal);
#endif
#endif