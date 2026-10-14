#if defined(__linux__) && defined(__x86_64__)
#include "outer_terminal_linux.h"
#include <sys/syscall.h>
#include <linux/wait.h>
#include <limits.h>
#include <errno.h>
long sl_linux_observe_outer_terminal(sl_linux_outer_terminal *o){
 if(!o||!o->birth_original)return -EINVAL;
 const sl_linux_outer_birth *b=o->birth_original;
 uintptr_t output=(uintptr_t)o,source=(uintptr_t)b;
 if(sizeof(*o)>UINTPTR_MAX-output||sizeof(*b)>UINTPTR_MAX-source||
    (output<source+sizeof(*b)&&source<output+sizeof(*o)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;
 for(size_t i=offsetof(sl_linux_outer_terminal,actual_birth);i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 /* Numeric pidfd output can survive a failed clone3. Join actual successful
  * parent return before any native adoption/query of that descriptor. */
 if(!b->prepared||!b->attempted||!atomic_load_explicit(&b->entered,memory_order_acquire)||
    !atomic_load_explicit(&b->parent_captured,memory_order_acquire)||b->raw_parent<=0||b->raw_parent>INT_MAX||b->pidfd<0||
    b->arguments.flags!=CLONE_PIDFD||b->arguments.pidfd!=(uint64_t)(uintptr_t)&b->pidfd||b->arguments.exit_signal!=SIGCHLD)return -EINVAL;
 o->attempted=1;o->actual_birth=b;o->result=-EINVAL;
 atomic_init(&o->wait.entered,0);atomic_init(&o->wait.returned,0);
 o->raw_wait=sl_linux_call6(&o->wait,"outer.creator.waitid.pidfd.nowait",SYS_waitid,
  P_PIDFD,b->pidfd,(long)&o->information,WEXITED|WNOWAIT|WNOHANG,(long)&o->usage,0);
 if(o->raw_wait){if(o->raw_wait<0)o->result=o->raw_wait;goto done;}
 if(!atomic_load_explicit(&o->wait.entered,memory_order_acquire)||
    !atomic_load_explicit(&o->wait.returned,memory_order_acquire)||o->wait.result!=o->raw_wait)goto done;
 if(!o->information.si_pid){o->result=1;goto done;}
 if(o->information.si_pid!=b->raw_parent||o->information.si_signo!=SIGCHLD||
    (o->information.si_code!=CLD_EXITED&&o->information.si_code!=CLD_KILLED&&o->information.si_code!=CLD_DUMPED))goto done;
 o->terminal=1;o->result=0;
 done:o->returned=1;return o->result;
 /* WNOWAIT retains wait/reap ownership. No kill, reap, descriptor close,
  * retry, time-expiry terminal inference or graph-retirement grant. */
}
#endif
