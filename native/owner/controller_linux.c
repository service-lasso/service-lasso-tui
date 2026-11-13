#if defined(__linux__) && defined(__x86_64__)
#include "owner_linux.h"
#include <sys/syscall.h>
#include <linux/wait.h>
/* One actual controller observation cell, allocated before its operation.
 * Root waitability/status is separate from cgroup population, every inherited
 * writer, stream EOF and all original disposition. WNOWAIT keeps actual parent
 * wait/reap ownership; this function never reaps, closes, kills or retries. */
long sl_linux_observe_root_exit(sl_linux_root_observation *observation) {
    sl_linux_bootstrap *bootstrap;
    if (!observation || observation->attempted || !observation->birth_original ||
        !observation->birth_original->bootstrap) return -1;
    observation->attempted=1;
    bootstrap=observation->birth_original->bootstrap;
    if (!atomic_load_explicit(&bootstrap->clone_parent.returned,memory_order_acquire) ||
        bootstrap->clone_parent.result<=0 || bootstrap->pidfd<0) return -1;
    observation->information=(siginfo_t){0};
    observation->usage=(struct rusage){0};
    atomic_init(&observation->wait.entered,0);atomic_init(&observation->wait.returned,0);
    /* WNOHANG never blocks the collector: zero si_pid is an actual not-yet-exit
     * observation and needs another separately preowned admitted observation,
     * not an overwritten cell or timeout-as-death. P_PIDFD is native idtype3. */
    return sl_linux_call6(&observation->wait,"controller.root.waitid.pidfd.nowait",SYS_waitid,
                         P_PIDFD,bootstrap->pidfd,(long)&observation->information,
                         WEXITED|WNOWAIT|WNOHANG,(long)&observation->usage,0);
}
#endif
