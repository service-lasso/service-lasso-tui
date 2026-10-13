#ifndef SERVICE_LASSO_INPUT_WORKERS_LINUX_H
#define SERVICE_LASSO_INPUT_WORKERS_LINUX_H
#include "terminal_linux.h"
#include <pthread.h>
#include <poll.h>
enum sl_linux_input_outcome{SL_INPUT_PENDING,SL_INPUT_STOPPED,SL_INPUT_EOF,SL_INPUT_FAILURE,SL_INPUT_EXHAUSTED};
typedef struct sl_linux_input_step{sl_linux_call call;struct pollfd readiness[2];unsigned kind;}sl_linux_input_step;
typedef struct sl_linux_input_frame{unsigned char bytes[4096];sl_linux_input_step *steps;unsigned long capacity,count;sl_linux_input_step *prepared_steps;unsigned long prepared_capacity;long raw_count;unsigned long written;}sl_linux_input_frame;
struct sl_linux_input_stop;
typedef struct sl_linux_input_worker{
 struct sl_linux_schedule_wake *terminal_wake; void *owner_original;sl_linux_mode_episode *mode_original;
 sl_linux_input_frame *frames;unsigned long capacity,count;
 sl_linux_input_frame *prepared_frames;unsigned long prepared_capacity;
 const sl_linux_call *master_nonblocking_original;long start_result;
 int master_fd,input_fd,raw_control[2],control[2];sl_linux_fd_lift control_lifts[2];sl_linux_call control_pair;
 pthread_t thread;int create_result;void *worker_result;
 struct sl_linux_input_stop *stop_original;
 _Atomic unsigned entered,returned;
 enum sl_linux_input_outcome outcome;
 unsigned prepare_attempted,prepared,start_attempted,start_returned,create_entered,create_returned;
}sl_linux_input_worker;
typedef struct sl_linux_input_stop{
 sl_linux_input_worker *worker_original;sl_linux_call send;unsigned char byte;
 int join_result;void *joined_result;unsigned attempted,send_completed,join_entered,join_returned;
 long interpretation_result;unsigned interpretation_returned;
}sl_linux_input_stop;
long sl_linux_prepare_terminal_input(sl_linux_input_worker *worker);
long sl_linux_start_terminal_input(sl_linux_input_worker *worker,sl_linux_mode_episode *mode,int master_fd,const sl_linux_call *nonblocking);
long sl_linux_stop_join_terminal_input(sl_linux_input_stop *stop,sl_linux_input_worker *worker);
#endif
