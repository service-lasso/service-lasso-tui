#ifndef SERVICE_LASSO_PUBLIC_MAIN_LINUX_H
#define SERVICE_LASSO_PUBLIC_MAIN_LINUX_H
#include "candidate_arena_linux.h"
#include "gate_linux.h"
/* Generated owner must outlive every source call below; the outer-observer
 * route places it in the actual prebirth shared containing arena. Returning this
 * operation does not dispose it or authorize return from the public C main. */
typedef struct sl_linux_public_main_owner {
 int argc;char **argv,**environment;
 sl_linux_candidate_arena *(*construct_arena_original)(void);
 sl_linux_candidate_arena *arena_original;
 sl_linux_candidate_arena *prepared_arena_original;
 long attachment_result;unsigned attachment_attempted,attachment_returned;
 sl_linux_public_startup *startup_original;
 sl_linux_public_controller *controller_original;
 sl_linux_gate_delivery gate_original;
 long arena_result,startup_result,construction_result,capture_result,result;
 long admission_result,gate_result;unsigned admission_attempted,admission_returned,gate_returned;
 unsigned attempted,arena_returned,startup_returned,construction_returned,capture_returned,returned;
} sl_linux_public_main_owner;
long sl_linux_construct_public_main_owner(sl_linux_public_main_owner *owner);
long sl_linux_attach_public_main_owner(sl_linux_public_main_owner *owner,
 sl_linux_candidate_arena *arena,int argc,char **argv,char **environment);
long sl_linux_admit_public_main_owner(sl_linux_public_main_owner *owner);
long sl_linux_capture_public_main_owner(sl_linux_public_main_owner *owner);
#endif
