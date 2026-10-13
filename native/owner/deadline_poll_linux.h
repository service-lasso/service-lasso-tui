#ifndef SERVICE_LASSO_DEADLINE_POLL_LINUX_H
#define SERVICE_LASSO_DEADLINE_POLL_LINUX_H
#include "deadline_linux.h"
#include <poll.h>
typedef struct sl_linux_deadline_poll {
 sl_linux_deadline_sequence *sequence_original;
 const struct pollfd *descriptors_original;
 struct pollfd *outputs_original;unsigned long count;
 sl_linux_deadline_sequence *actual_sequence;
 const struct pollfd *actual_descriptors;
 struct pollfd *actual_outputs;unsigned long actual_count;
 sl_linux_deadline_observation *before,*after;
 struct timespec requested_timeout,timeout;
 sl_linux_call poll;
 long raw_poll,before_result,after_result,result;
 unsigned attempted,poll_returned,returned,timely;
} sl_linux_deadline_poll;
_Static_assert(sizeof(struct pollfd)==8&&offsetof(struct pollfd,revents)==6,"selected native ppoll ABI");
_Static_assert(sizeof(sl_linux_deadline_poll)==240,"retained deadline ppoll occurrence");
_Static_assert(offsetof(sl_linux_deadline_poll,actual_sequence)==32&&offsetof(sl_linux_deadline_poll,requested_timeout)==80&&offsetof(sl_linux_deadline_poll,timeout)==96&&offsetof(sl_linux_deadline_poll,poll)==112,"deadline ppoll native offsets");
long sl_linux_poll_original_deadline(sl_linux_deadline_poll *original);
/* One native attempt. result0/timely1 means original deadline still eligible,
 * not FD readiness, terminal state or disposal. raw_poll and every revents
 * remain distinct originals even when the after observation fails/expires.
 * requested_timeout is the immutable input projection; raw SYS_ppoll may
 * update its separate timeout output, which is retained without restoration.
 * The actual caller owns descriptor lifetimes and sequence serialization. */
#endif
