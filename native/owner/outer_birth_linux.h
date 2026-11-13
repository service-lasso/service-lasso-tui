#ifndef SERVICE_LASSO_OUTER_BIRTH_LINUX_H
#define SERVICE_LASSO_OUTER_BIRTH_LINUX_H
#if defined(__linux__) && defined(__x86_64__)
#include <linux/sched.h>
#include <stdatomic.h>
#include <stddef.h>
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"raw-fork unsigned publication must not call libatomic");

/* One genuine syscall entry has two native return continuations. The root
 * must already be held in observer/creator shared containing backing. These
 * cells do not finance it, authenticate it, or prove raw-fork provider safety. */
typedef struct sl_linux_outer_birth {
 struct clone_args arguments;
 _Atomic unsigned entered,parent_captured,child_captured;
 unsigned prepared,attempted;
 long raw_parent,raw_child;
 int pidfd;
} sl_linux_outer_birth;
_Static_assert(sizeof(struct clone_args)==88,"selected clone3 v2 input");
_Static_assert(sizeof(long)==8&&sizeof(unsigned)==4,"selected x86_64 raw ABI");
_Static_assert(sizeof(sl_linux_outer_birth)==136,"outer birth original layout");
_Static_assert(offsetof(sl_linux_outer_birth,entered)==88,"entry original");
_Static_assert(offsetof(sl_linux_outer_birth,parent_captured)==92,"parent original");
_Static_assert(offsetof(sl_linux_outer_birth,child_captured)==96,"child original");
_Static_assert(offsetof(sl_linux_outer_birth,raw_parent)==112,"parent raw return");
_Static_assert(offsetof(sl_linux_outer_birth,raw_child)==120,"child raw return");
_Static_assert(offsetof(sl_linux_outer_birth,pidfd)==128,"actual kernel pidfd");
typedef struct sl_linux_outer_return {long raw,result;unsigned captured,matched;_Atomic unsigned published;} sl_linux_outer_return;
typedef struct sl_linux_outer_continuation {
 sl_linux_outer_birth *birth_original,*actual_birth;
 unsigned attempted;sl_linux_outer_return parent,child;
} sl_linux_outer_continuation;
_Static_assert(sizeof(sl_linux_outer_return)==32&&sizeof(sl_linux_outer_continuation)==88,"distinct shared native continuation geometry");
long sl_linux_enter_outer_continuation(sl_linux_outer_continuation *original);
long sl_linux_prepare_outer_birth(sl_linux_outer_birth *original);
long sl_linux_enter_outer_birth(sl_linux_outer_birth *original);
#endif
#endif
