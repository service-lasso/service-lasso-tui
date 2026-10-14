#if defined(__linux__) && defined(__x86_64__)
#include "owner_linux.h"
#include "candidate_preexec_linux.h"
#include <sys/syscall.h>
#include <signal.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/ioctl.h>

_Static_assert(ATOMIC_INT_LOCK_FREE==2,"native bootstrap requires lock-free source publication");

/* Keep the actual signed kernel return, including -errno. No libc syscall
 * wrapper rewrites the return or discards a cleanup cause. All inputs/output
 * belong to the preowned source cell before the instruction. */
long sl_linux_call6(sl_linux_call *call,const char *source,long number,
                           long a0,long a1,long a2,long a3,long a4,long a5) {
    long result;
    register long r10 __asm__("r10")=a3;
    register long r8 __asm__("r8")=a4;
    register long r9 __asm__("r9")=a5;
    call->source=source;call->number=number;
    call->arguments[0]=a0;call->arguments[1]=a1;call->arguments[2]=a2;
    call->arguments[3]=a3;call->arguments[4]=a4;call->arguments[5]=a5;
    atomic_store_explicit(&call->entered,1,memory_order_release);
    __asm__ volatile("syscall" : "=a"(result) : "a"(number),"D"(a0),"S"(a1),
                     "d"(a2),"r"(r10),"r"(r8),"r"(r9) : "rcx","r11","memory");
    call->result=result;
    atomic_store_explicit(&call->returned,1,memory_order_release);
    return result;
}

/* Failure never calls exit/close/kill or makes family zero. The original child
 * remains alive with its native views and shared originals. A separately
 * authorized outer recovery protocol is still required, not implemented here.
 * One real pause observation is retained; if interrupted, no syscall/error loop
 * can overwrite that terminal cell. This fallback's CPU/lifetime charge must
 * be included in admission; it has no finite qualification credit today. */
static _Noreturn void sl_linux_retain_bootstrap(sl_linux_bootstrap *bootstrap) {
    atomic_store_explicit(&bootstrap->failed,1,memory_order_release);
    sl_linux_call6(&bootstrap->failure_pause,"bootstrap.failed.pause",SYS_pause,0,0,0,0,0,0);
    for (;;) __asm__ volatile("pause" ::: "memory");
}

long sl_linux_birth_into_cgroup(sl_linux_birth *birth) {
    long result;
    sl_linux_bootstrap *bootstrap;
    unsigned index;
    if (!birth || birth->attempted) return -1;
    birth->attempted=1;
    if (!birth->bootstrap || birth->cgroup_fd<0 || birth->payload_fd<0 ||
        birth->entry_gate_fd<0 || !birth->argv) return -1;
    bootstrap=birth->bootstrap;
    /* Refuse an absent or partially prepared native action backing before
     * birth. Preparation is not proof of a closed or authorized FD map. */
    if (!atomic_load_explicit(&bootstrap->fd_actions_prepared,memory_order_acquire) ||
        !bootstrap->fd_actions || !bootstrap->fd_action_count || !bootstrap->authority.prepared) return -1;
    bootstrap->pidfd=-1;
    bootstrap->arguments=(struct clone_args){0};
    bootstrap->arguments.flags=CLONE_INTO_CGROUP|CLONE_PIDFD;
    bootstrap->arguments.pidfd=(uint64_t)(uintptr_t)&bootstrap->pidfd;
    bootstrap->arguments.cgroup=(uint64_t)birth->cgroup_fd;
    bootstrap->arguments.exit_signal=SIGCHLD;
    /* A generic wrapper would write the child's return into COW storage.
     * Preown distinct shared output cells for the SAME kernel call and branch
     * before any post-return operation can overwrite the actual raw result. */
    bootstrap->clone_parent.source="clone3.into-owned-cgroup.parent-return";
    bootstrap->clone_parent.number=SYS_clone3;bootstrap->clone_child.number=SYS_clone3;
    bootstrap->clone_child.source="clone3.into-owned-cgroup.child-return";
    bootstrap->clone_parent.arguments[0]=(long)&bootstrap->arguments;
    bootstrap->clone_parent.arguments[1]=sizeof(bootstrap->arguments);
    bootstrap->clone_child.arguments[0]=(long)&bootstrap->arguments;
    bootstrap->clone_child.arguments[1]=sizeof(bootstrap->arguments);
    atomic_store_explicit(&bootstrap->clone_parent.entered,1,memory_order_release);
    __asm__ volatile("syscall" : "=a"(result) : "a"((long)SYS_clone3),
             "D"((long)&bootstrap->arguments),"S"(sizeof(bootstrap->arguments))
             : "rcx","r11","memory");
    if (result!=0) {
        bootstrap->clone_parent.result=result;
        atomic_store_explicit(&bootstrap->clone_parent.returned,1,memory_order_release);
        return result;
    }
    bootstrap->clone_child.result=result;
    atomic_store_explicit(&bootstrap->clone_child.entered,1,memory_order_release);
    atomic_store_explicit(&bootstrap->clone_child.returned,1,memory_order_release);

    /* Only trusted native bootstrap instructions run in the new family before
     * the real gate. No Go runtime, heap, loader or operator instruction runs.
     * The preowned physical shared cells retain actual child native IO results.
     * No libc, allocation, callback or Go runtime fork hook is called here. */
    /* The contained root needs its own actual session and controlling PTY,
     * not merely three descriptors pointing at a slave. Both branch originals
     * live in prebirth shared backing through exec and later required cuts. */
    result=sl_linux_call6(&bootstrap->create_session,"bootstrap.session.setsid",SYS_setsid,0,0,0,0,0,0);
    if (result<0) sl_linux_retain_bootstrap(bootstrap);
    if (bootstrap->controlling_terminal_fd<3) sl_linux_retain_bootstrap(bootstrap);
    result=sl_linux_call6(&bootstrap->acquire_terminal,"bootstrap.terminal.acquire",SYS_ioctl,
                         bootstrap->controlling_terminal_fd,TIOCSCTTY,0,0,0,0);
    if (result<0) sl_linux_retain_bootstrap(bootstrap);
    /* This complete action roster must be constructed from the genuine sealed
     * original FD map before clone3, in separately held MAP_SHARED backing.
     * It is not accepted from public args or an untrusted numeric FD list.
     * Explicit close-before-dup avoids dup3 silently replacing an old target
     * and losing that old view's close outcome. The entry builder must prove
     * each target is genuinely vacant at its exact action, not merely named. */
    for (unsigned long action_index=0;action_index<bootstrap->fd_action_count;action_index++) {
        sl_linux_fd_action *action=&bootstrap->fd_actions[action_index];
        if (!action->outer_original || action->source_fd<0) sl_linux_retain_bootstrap(bootstrap);
        if (action->kind==SL_FD_CLOSE_VIEW) {
            result=sl_linux_call6(&action->call,"bootstrap.fd.close-original-view",SYS_close,
                                 action->source_fd,0,0,0,0,0);
        } else if (action->kind==SL_FD_DUPLICATE_VIEW) {
            if (action->target_fd<0 || action->source_fd==action->target_fd) sl_linux_retain_bootstrap(bootstrap);
            result=sl_linux_call6(&action->call,"bootstrap.fd.duplicate-into-vacant-view",SYS_dup3,
                                 action->source_fd,action->target_fd,0,0,0,0);
        } else if (action->kind==SL_FD_CLEAR_EXEC) {
            result=sl_linux_call6(&action->call,"bootstrap.custody-view.clear-cloexec",SYS_fcntl,
                                 action->source_fd,F_SETFD,0,0,0,0);
        } else sl_linux_retain_bootstrap(bootstrap);
        if (result<0) sl_linux_retain_bootstrap(bootstrap);
    }
    result=sl_linux_apply_preexec_authority(&bootstrap->authority);
    if (result<0) sl_linux_retain_bootstrap(bootstrap);
    if (!bootstrap->candidate_preexec || bootstrap->candidate_preexec->bootstrap_original!=bootstrap)
        sl_linux_retain_bootstrap(bootstrap);
    result=sl_linux_apply_candidate_preexec(bootstrap->candidate_preexec);
    if (result) sl_linux_retain_bootstrap(bootstrap);
    /* Parent must inspect these SAME shared typed originals and the complete
     * actual supported profile before supplying the existing final gate token.
     * Child publication by itself never releases this blocking gate. */
    for (index=0;bootstrap->gate_count<sizeof(bootstrap->gate_received);index++) {
        unsigned remaining=sizeof(bootstrap->gate_received)-bootstrap->gate_count;
        result=sl_linux_call6(&bootstrap->gate_reads[index],"bootstrap.entry-gate.read",SYS_read,
               birth->entry_gate_fd,(long)(bootstrap->gate_received+bootstrap->gate_count),remaining,0,0,0);
        if (result<=0 || (unsigned long)result>remaining) sl_linux_retain_bootstrap(bootstrap);
        bootstrap->gate_count+=(unsigned)result;
    }
    for (index=0;index<sizeof(bootstrap->gate_received);index++)
        if (bootstrap->gate_received[index]!=bootstrap->gate_expected[index]) sl_linux_retain_bootstrap(bootstrap);
    result=sl_linux_call6(&bootstrap->gate_close,"bootstrap.entry-gate.close-view",SYS_close,
                         birth->entry_gate_fd,0,0,0,0,0);
    if (result<0) sl_linux_retain_bootstrap(bootstrap);
    atomic_store_explicit(&bootstrap->exec_entered,1,memory_order_release);
    /* AT_EMPTY_PATH is the Linux UAPI flag, 0x1000. Use the held original
     * executable descriptor: pathname lookup is not loaded-byte identity.
     * Actual descriptor/image authentication remains the caller's obligation. */
    result=sl_linux_call6(&bootstrap->execute,"bootstrap.execveat.held-payload",SYS_execveat,
                           birth->payload_fd,(long)"",(long)birth->argv,
                           (long)birth->environment,0x1000,0);
    (void)result;
    sl_linux_retain_bootstrap(bootstrap);
}
#endif
