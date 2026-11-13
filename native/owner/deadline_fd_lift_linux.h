#ifndef SERVICE_LASSO_DEADLINE_FD_LIFT_LINUX_H
#define SERVICE_LASSO_DEADLINE_FD_LIFT_LINUX_H
#include "deadline_linux.h"
typedef struct sl_linux_deadline_fd_lift {
 const sl_linux_call *producer_original;
 sl_linux_deadline_sequence *sequence_original;
 sl_linux_fd_lift lift;
 sl_linux_deadline_boundary duplicate_boundary,close_boundary;
 long duplicate_clock_result,close_clock_result,result;
 unsigned attempted,returned;
} sl_linux_deadline_fd_lift;
long sl_linux_lift_deadline_fd(sl_linux_deadline_fd_lift *output,
 const sl_linux_call *producer,sl_linux_deadline_sequence *sequence);
int sl_linux_deadline_fd_lift_matches_original(const sl_linux_deadline_fd_lift *original);
#endif
