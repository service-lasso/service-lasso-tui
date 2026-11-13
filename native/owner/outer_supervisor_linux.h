#ifndef SERVICE_LASSO_OUTER_SUPERVISOR_LINUX_H
#define SERVICE_LASSO_OUTER_SUPERVISOR_LINUX_H
#include "outer_terminal_linux.h"
#include "deadline_poll_linux.h"
typedef struct sl_linux_outer_turn {
 sl_linux_deadline_poll poll;
 struct pollfd input,output;
 sl_linux_deadline_boundary boundary;
 long observation_result,boundary_result,result;
 unsigned attempted,observation_returned,boundary_returned,returned;
} sl_linux_outer_turn;
typedef struct sl_linux_outer_supervisor {
 const sl_linux_outer_continuation *continuation_original;
 sl_linux_deadline_sequence *sequence_original;
 sl_linux_outer_turn *turns_original;
 sl_linux_outer_terminal *observations_original;
 unsigned long capacity;
 const sl_linux_outer_continuation *actual_continuation;
 sl_linux_deadline_sequence *actual_sequence;
 sl_linux_outer_turn *actual_turns;
 sl_linux_outer_terminal *actual_observations;
 unsigned long actual_capacity,next;
 long result;unsigned attempted,returned,terminal,stopped;
} sl_linux_outer_supervisor;
_Static_assert(sizeof(sl_linux_outer_turn)==360,"retained parent poll and terminal clock turn");
_Static_assert(sizeof(sl_linux_outer_supervisor)==112,"retained finite parent supervisor");
long sl_linux_supervise_outer_parent(sl_linux_outer_supervisor *original);
/* Only the actual parent-return source branch invokes this caller. Shared
 * matching records do not authenticate branch identity or finance effects.
 * No process termination, reap, close, retry renewal or FINAL authority. */
#endif