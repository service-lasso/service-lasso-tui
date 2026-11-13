#ifndef SERVICE_LASSO_FORWARD_WORKERS_LINUX_H
#define SERVICE_LASSO_FORWARD_WORKERS_LINUX_H
#include "capture_workers_linux.h"
typedef struct sl_linux_notification {
 int lock_result,signal_result,unlock_result;
 unsigned attempted,lock_returned,signal_returned,unlock_returned;
} sl_linux_notification;
typedef struct sl_linux_forward_wait {
 int result;unsigned attempted,returned;
} sl_linux_forward_wait;
typedef struct sl_linux_forward_iteration {
 int lock_result,unlock_result;unsigned entered,lock_returned,unlock_returned;
} sl_linux_forward_iteration;
struct sl_linux_forward_release;
struct sl_linux_forward_join;
typedef struct sl_linux_forward_worker {
 struct sl_linux_schedule_wake *terminal_wake;
 sl_linux_capture_worker *reader_original;
 sl_linux_capture_frame *prepared_frames;unsigned long prepared_frame_capacity;
 pthread_mutex_t mutex;pthread_cond_t changed;
 int mutex_result,condition_result,create_result;
 unsigned prepare_attempted,prepared,create_entered,create_returned;
 unsigned mutex_init_entered,mutex_init_returned,condition_init_entered,condition_init_returned;
 pthread_t thread;
 sl_linux_write_signal signal;
 sl_linux_forward_wait *waits;unsigned long wait_capacity,wait_count;
 sl_linux_forward_wait *prepared_waits;unsigned long prepared_wait_capacity;
 sl_linux_notification terminal_notification;
 sl_linux_notification *publications;unsigned long publication_capacity;
 sl_linux_notification *prepared_publications;
 sl_linux_forward_iteration *iterations;unsigned long iteration_capacity,iteration_count;
 sl_linux_forward_iteration *prepared_iterations;unsigned long prepared_iteration_capacity;
 sl_linux_call held_wait;unsigned hold_word;
 sl_linux_forward_iteration terminal_lock;
 struct sl_linux_forward_release *release_original;
 _Atomic(struct sl_linux_forward_release *) release_attempt_original;
 struct sl_linux_forward_join *joined_by;
 void *worker_result;
 unsigned long next_frame;long result,forwarding_result;
 _Atomic unsigned entered,forwarding_finished,returned;
 /* A finished forward body still owns its blocked signal episode and waits
  * alive. Final disposition is a separate connected controller operation;
  * no reader EOF, wire ACK or caller timeout sets that operation here. */
} sl_linux_forward_worker;
typedef struct sl_linux_forward_release {
 sl_linux_forward_worker *worker_original;
 sl_linux_reader_join *raw_join_original;
 sl_linux_notification notification;
 long result;unsigned attempted,returned;
} sl_linux_forward_release;
typedef struct sl_linux_forward_join {
 sl_linux_forward_worker *worker_original;void *joined_result;
 int result;long interpretation_result;unsigned interpretation_returned;unsigned attempted,returned;
} sl_linux_forward_join;
long sl_linux_prepare_forward_worker(sl_linux_forward_worker *worker);
int sl_linux_start_forward_worker(sl_linux_forward_worker *worker);
long sl_linux_notify_forward_worker(sl_linux_forward_worker *worker,sl_linux_notification *observation);
long sl_linux_release_forward_source(sl_linux_forward_release *release);
int sl_linux_observe_forward_join(sl_linux_forward_join *observation);
#endif
