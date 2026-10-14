#if defined(__linux__) && defined(__x86_64__)
#include "outer_birth_linux.h"
#include <stdint.h>
#include <signal.h>
#include <sys/syscall.h>
#include <errno.h>
_Static_assert(SYS_clone3==435,"selected x86_64 clone3 syscall");

long sl_linux_prepare_outer_birth(sl_linux_outer_birth *o){
 if(!o)return -EINVAL;
 /* Caller establishes containing ownership/noalias before this source-only
  * constructor. Preserve any dirty occurrence instead of resetting it. */
 const unsigned char *b=(const unsigned char*)o;
 for(size_t i=0;i<sizeof(*o);i++)if(b[i])return -EINVAL;
 atomic_init(&o->entered,0);atomic_init(&o->parent_captured,0);atomic_init(&o->child_captured,0);
 o->arguments.flags=CLONE_PIDFD;
 o->arguments.pidfd=(uint64_t)(uintptr_t)&o->pidfd;
 o->arguments.exit_signal=SIGCHLD;
 o->pidfd=-1;o->prepared=1;return 0;
}

long sl_linux_enter_outer_birth(sl_linux_outer_birth *o){
 if(!o||o->prepared!=1||o->attempted||atomic_load(&o->entered)||
    atomic_load(&o->parent_captured)||atomic_load(&o->child_captured)||o->raw_parent||o->raw_child||o->pidfd!=-1)return -EINVAL;
 const struct clone_args *a=&o->arguments;
 if(a->flags!=CLONE_PIDFD||a->pidfd!=(uint64_t)(uintptr_t)&o->pidfd||a->exit_signal!=SIGCHLD||
    a->child_tid||a->parent_tid||a->stack||a->stack_size||a->tls||a->set_tid||a->set_tid_size||a->cgroup)return -EINVAL;
 o->attempted=1;
 long raw;
 /* Normal SysV function and fork-like syscall preserve each continuation's
  * stack. RDX is the held root operand; kernel x86_64 syscall preserves it.
  * Capture AX in the corresponding original BEFORE any C continuation.
  * No common returned cell is overwritten by the child and parent races.
  * x86 aligned MOV stores supply source release ordering; memory clobber
  * prevents compiler movement across the actual entry/capture frontier. */
 __asm__ volatile(
  "movl $1,%c[entry](%%rdx)\n\t"
  "syscall\n\t"
  "testq %%rax,%%rax\n\t"
  "jz 1f\n\t"
  "movq %%rax,%c[parent](%%rdx)\n\t"
  "movl $1,%c[parent_done](%%rdx)\n\t"
  "jmp 2f\n\t"
  "1: movq %%rax,%c[child](%%rdx)\n\t"
  "movl $1,%c[child_done](%%rdx)\n\t"
  "2:"
  : "=a"(raw)
  : "0"((long)SYS_clone3),"D"(a),"S"(sizeof(*a)),"d"(o),
    [entry]"i"(offsetof(sl_linux_outer_birth,entered)),
    [parent]"i"(offsetof(sl_linux_outer_birth,raw_parent)),
    [parent_done]"i"(offsetof(sl_linux_outer_birth,parent_captured)),
    [child]"i"(offsetof(sl_linux_outer_birth,raw_child)),
    [child_done]"i"(offsetof(sl_linux_outer_birth,child_captured))
  : "rcx","r11","cc","memory");
 return raw;
}
#endif
