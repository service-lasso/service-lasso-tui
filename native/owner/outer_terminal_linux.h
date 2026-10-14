#ifndef SERVICE_LASSO_OUTER_TERMINAL_LINUX_H
#define SERVICE_LASSO_OUTER_TERMINAL_LINUX_H
#include "outer_birth_linux.h"
#include "owner_linux.h"
typedef struct sl_linux_outer_terminal {
 const sl_linux_outer_birth *birth_original,*actual_birth;
 sl_linux_call wait;
 siginfo_t information;struct rusage usage;
 long raw_wait,result;unsigned attempted,returned,terminal;
} sl_linux_outer_terminal;
_Static_assert(sizeof(sl_linux_call)==80&&sizeof(siginfo_t)==128&&sizeof(struct rusage)==144,"selected x86_64 native output ABI");
_Static_assert(sizeof(sl_linux_outer_terminal)==400,"outer terminal containing row");
_Static_assert(offsetof(sl_linux_outer_terminal,wait)==16,"wait original offset");
_Static_assert(offsetof(sl_linux_outer_terminal,information)==96,"siginfo original offset");
_Static_assert(offsetof(sl_linux_outer_terminal,usage)==224,"rusage original offset");
_Static_assert(offsetof(sl_linux_outer_terminal,raw_wait)==368,"raw return original offset");
/* Parameterized original observation only. The actual native parent-return
 * branch owns calling this row; shared bytes are not proof of caller identity. */
long sl_linux_observe_outer_terminal(sl_linux_outer_terminal *observation);
#endif
