#ifndef SERVICE_LASSO_FAMILY_EVENTS_LINUX_H
#define SERVICE_LASSO_FAMILY_EVENTS_LINUX_H
#include "owner_linux.h"
#include <linux/openat2.h>
#include <linux/stat.h>
#include <pthread.h>
struct sl_linux_candidate_target;
typedef struct sl_linux_family_read {sl_linux_call call;unsigned char byte;long count;unsigned attempted,returned;} sl_linux_family_read;
typedef struct sl_linux_family_events {
 struct sl_linux_candidate_target *target_original,*actual_target;
 sl_linux_call open,filesystem_query,identity_query,close;
 sl_linux_fd_lift lift;struct open_how how;struct statfs filesystem;struct statx identity;
 sl_linux_family_read reads[22];unsigned long read_count,extent;
 int raw_fd,fd;long primary_result,close_result,result;
 unsigned attempted,returned,eof,excess,framing_complete,populated,frozen,close_attempted,close_returned;
} sl_linux_family_events;
/* Observation only. Natural family zero additionally needs actual continuing
 * migration exclusion, genuine terminal root, writer EOF and selected cuts. */
long sl_linux_observe_family_events(sl_linux_family_events *root);
struct sl_linux_family_join;
typedef struct sl_linux_family_worker {
 struct sl_linux_schedule_wake *terminal_wake; sl_linux_family_events observation;sl_linux_root_observation *root_terminal_original;pthread_t thread;int create_result;
 unsigned attempted,create_entered,create_returned;
 _Atomic unsigned body_entered,body_returned;
 struct sl_linux_family_join *joined_by;
} sl_linux_family_worker;
typedef struct sl_linux_family_join {
 sl_linux_family_worker *worker_original;void *joined_result;
 int native_result;long interpretation_result;unsigned attempted,returned;
} sl_linux_family_join;
long sl_linux_start_family_worker(sl_linux_family_worker *worker);
long sl_linux_observe_family_join(sl_linux_family_join *join);
#endif