#if defined(__linux__) && defined(__x86_64__)
#include "schedule_linux.h"
#include "forward_workers_linux.h"
#include <sys/syscall.h>
#include <linux/futex.h>
#include <errno.h>
long sl_linux_prepare_forward_worker(sl_linux_forward_worker *worker){
 unsigned long index;
 if(!worker||worker->prepare_attempted)return -1;
 worker->prepare_attempted=1;
 if(!worker->reader_original||!worker->waits||!worker->wait_capacity||worker->reader_original->forward_original||
 !worker->publications||worker->publication_capacity!=worker->reader_original->capacity||
 !worker->iterations||!worker->iteration_capacity)return -1;
 worker->prepared_waits=worker->waits;worker->prepared_wait_capacity=worker->wait_capacity;
 worker->prepared_publications=worker->publications;
 worker->prepared_frames=worker->reader_original->frames;
 worker->prepared_frame_capacity=worker->reader_original->capacity;
 worker->prepared_iterations=worker->iterations;worker->prepared_iteration_capacity=worker->iteration_capacity;
 atomic_init(&worker->held_wait.entered,0);atomic_init(&worker->held_wait.returned,0);
 atomic_init(&worker->release_attempt_original,0);
 for(index=0;index<worker->wait_capacity;index++)if(worker->waits[index].attempted)return -1;
 for(index=0;index<worker->publication_capacity;index++)if(worker->publications[index].attempted)return -1;
 for(index=0;index<worker->iteration_capacity;index++)if(worker->iterations[index].entered)return -1;
 worker->mutex_init_entered=1;
 worker->mutex_result=pthread_mutex_init(&worker->mutex,0);
 worker->mutex_init_returned=1;
 if(worker->mutex_result)return worker->mutex_result;
 worker->condition_init_entered=1;
 worker->condition_result=pthread_cond_init(&worker->changed,0);
 worker->condition_init_returned=1;
 if(worker->condition_result)return worker->condition_result;
 atomic_init(&worker->entered,0);atomic_init(&worker->forwarding_finished,0);atomic_init(&worker->returned,0);
 worker->reader_original->forward_original=worker;worker->prepared=1;
 return 0;
}
long sl_linux_notify_forward_worker(sl_linux_forward_worker *worker,sl_linux_notification *observation){
 if(!worker||!worker->prepared||!observation||observation->attempted)return -1;
 observation->attempted=1;
 observation->lock_result=pthread_mutex_lock(&worker->mutex);observation->lock_returned=1;
 if(observation->lock_result)return observation->lock_result;
 observation->signal_result=pthread_cond_signal(&worker->changed);observation->signal_returned=1;
 observation->unlock_result=pthread_mutex_unlock(&worker->mutex);observation->unlock_returned=1;
 return observation->signal_result?observation->signal_result:observation->unlock_result;
}
static int wait_changed(sl_linux_forward_worker *worker){
 sl_linux_forward_wait *observation;
 if(worker->wait_count==worker->prepared_wait_capacity)return -1;
 observation=&worker->prepared_waits[worker->wait_count++];observation->attempted=1;
 observation->result=pthread_cond_wait(&worker->changed,&worker->mutex);observation->returned=1;
 return observation->result;
}
static void *forward_source(void *argument){
 sl_linux_forward_worker *worker=argument;
 sl_linux_capture_worker *reader=worker->reader_original;
 atomic_store_explicit(&worker->entered,1,memory_order_release);
 worker->result=sl_linux_write_signal_begin(&worker->signal);
 if(worker->result<0)goto held;
 for(;;){
  sl_linux_capture_frame *frame;
  sl_linux_forward_iteration *iteration;
  if(worker->iteration_count==worker->prepared_iteration_capacity){worker->result=-1;goto held;}
  iteration=&worker->prepared_iterations[worker->iteration_count++];iteration->entered=1;
  iteration->lock_result=pthread_mutex_lock(&worker->mutex);iteration->lock_returned=1;
  if(iteration->lock_result){worker->result=iteration->lock_result;goto held;}
  while(worker->next_frame==atomic_load_explicit(&reader->completed_count,memory_order_acquire)&&
        !atomic_load_explicit(&reader->returned,memory_order_acquire)){
   worker->result=wait_changed(worker);if(worker->result)break;
  }
  iteration->unlock_result=pthread_mutex_unlock(&worker->mutex);iteration->unlock_returned=1;
  if(worker->result||iteration->unlock_result)goto held;
  if(worker->next_frame==atomic_load_explicit(&reader->completed_count,memory_order_acquire))break;
  if(worker->next_frame>=worker->prepared_frame_capacity){worker->result=-1;goto held;}
  frame=&worker->prepared_frames[worker->next_frame++];
  if(frame->raw_only||!frame->read_returned||frame->raw_count<=0)continue;
  frame->controller_signal=&worker->signal;
  worker->result=sl_linux_capture_forward(frame);
  if(worker->result!=frame->raw_count)goto held;
  worker->result=0;
 }
held:
 worker->forwarding_result=worker->result;
 atomic_store_explicit(&worker->forwarding_finished,1,memory_order_release);
 if(worker->terminal_wake)sl_linux_schedule_notify(worker->terminal_wake);
 if(worker->result==0){
  sl_linux_forward_iteration *terminal=&worker->terminal_lock;
  terminal->entered=1;
  terminal->lock_result=pthread_mutex_lock(&worker->mutex);terminal->lock_returned=1;
  if(!terminal->lock_result){
   while(!worker->release_original){
    worker->result=wait_changed(worker);if(worker->result)break;
   }
   terminal->unlock_result=pthread_mutex_unlock(&worker->mutex);terminal->unlock_returned=1;
   if(!worker->result&&!terminal->unlock_result&&worker->release_original){
    worker->result=sl_linux_write_signal_finish(&worker->signal);
    if(worker->result==0){
     worker->worker_result=worker;
     atomic_store_explicit(&worker->returned,1,memory_order_release);
     return worker->worker_result;
    }
   }
  }
 }
 /* Failed/uncertain source or signal disposition retains actual thread/mask.
  * Successful source-specific return above still needs real native join and
  * does not imply root/family/EOF/archive FINAL acceptance. */
 sl_linux_call6(&worker->held_wait,"forward.unresolved-runtime.wait",SYS_futex,
               (long)&worker->hold_word,FUTEX_WAIT_PRIVATE,0,0,0,0);
 /* Interrupted/spurious return is retained once. No retry overwrites it and
  * no release is inferred. Connected final/recovery controller remains open. */
 for(;;)atomic_signal_fence(memory_order_seq_cst);
}
long sl_linux_release_forward_source(sl_linux_forward_release *release){
 sl_linux_forward_worker *worker;sl_linux_capture_worker *reader;sl_linux_reader_join *joined;
 sl_linux_notification *notification;unsigned long index;sl_linux_forward_release *expected=0;
 if(!release||release->attempted||!release->worker_original||!release->raw_join_original)return -1;
 release->attempted=1;release->result=-1;
 worker=release->worker_original;reader=worker->reader_original;joined=release->raw_join_original;
 /* Read worker-owned final fields only after release-published body boundary.
  * This conjunction is source-specific; no receipt or generic class grants it. */
 if(!atomic_load_explicit(&worker->forwarding_finished,memory_order_acquire)||
    worker->forwarding_result||!reader||joined->worker_original!=reader||!joined->returned||joined->result||!joined->interpretation_returned||joined->interpretation_result||
    joined->joined_result!=reader||reader->joined_by!=joined||
    !atomic_load_explicit(&reader->returned,memory_order_acquire)||reader->outcome!=SL_READER_READ_EOF||
    worker->next_frame!=atomic_load_explicit(&reader->completed_count,memory_order_acquire))goto denied;
 for(index=0;index<worker->next_frame;index++){
  sl_linux_capture_frame *frame=&worker->prepared_frames[index];
  sl_linux_notification *published=&worker->prepared_publications[index];
  if(!published->unlock_returned||published->lock_result||!published->signal_returned||published->signal_result||published->unlock_result)goto denied;
  if(!frame->read_returned)goto denied;
  if(frame->raw_count<0&&!(frame->raw_count==-EAGAIN&&
      atomic_load_explicit(&frame->read.returned,memory_order_acquire)&&frame->read.result==-EAGAIN))goto denied;
  if(!frame->raw_only&&frame->raw_count>0&&(!frame->forward_returned||frame->forwarded!=(unsigned long)frame->raw_count||frame->public_signal_result<0))goto denied;
 }
 if(!worker->terminal_notification.unlock_returned||worker->terminal_notification.lock_result||
    !worker->terminal_notification.signal_returned||worker->terminal_notification.signal_result||worker->terminal_notification.unlock_result)goto denied;
 if(!atomic_compare_exchange_strong_explicit(&worker->release_attempt_original,&expected,release,
       memory_order_acq_rel,memory_order_acquire))goto denied;
 notification=&release->notification;notification->attempted=1;
 notification->lock_result=pthread_mutex_lock(&worker->mutex);notification->lock_returned=1;
 if(notification->lock_result)goto denied;
 if(worker->release_original){
  notification->unlock_result=pthread_mutex_unlock(&worker->mutex);notification->unlock_returned=1;
  goto denied;
 }
 worker->release_original=release;
 notification->signal_result=pthread_cond_signal(&worker->changed);notification->signal_returned=1;
 notification->unlock_result=pthread_mutex_unlock(&worker->mutex);notification->unlock_returned=1;
 release->result=notification->signal_result?notification->signal_result:notification->unlock_result;
denied:
 release->returned=1;return release->result;
}
static int observe_forward_join_body(sl_linux_forward_join *observation){
 sl_linux_forward_worker *worker;
 if(!observation||observation->attempted||!observation->worker_original)return -1;
 observation->attempted=1;worker=observation->worker_original;
 if(!worker->create_returned||worker->create_result||worker->joined_by)return -1;
 observation->result=pthread_tryjoin_np(worker->thread,&observation->joined_result);observation->returned=1;
 if(observation->result)return observation->result;
 worker->joined_by=observation;
 if(observation->joined_result!=worker||!atomic_load_explicit(&worker->returned,memory_order_acquire))return -1;
 if(worker->terminal_wake){long result=sl_linux_schedule_joined_notification(worker->terminal_wake,worker);if(result)return result;}
 return 0;
}
int sl_linux_observe_forward_join(sl_linux_forward_join *o){
 if(!o||o->attempted)return -1;
 long result=observe_forward_join_body(o);o->interpretation_result=result;o->interpretation_returned=1;return result;
}
int sl_linux_start_forward_worker(sl_linux_forward_worker *worker){
 if(!worker||!worker->prepared||worker->create_entered)return -1;
 worker->create_entered=1;
 worker->create_result=pthread_create(&worker->thread,0,forward_source,worker);worker->create_returned=1;
 return worker->create_result;
}
#endif
