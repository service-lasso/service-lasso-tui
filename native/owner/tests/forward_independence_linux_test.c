/* UNRUN: actual pthread scheduling/join, substituted read/public-write/signal
 * effects. This proves no terminal, signal-mask, family, or native admission. */
#if defined(__linux__) && defined(__x86_64__)
#include "../forward_workers_linux.h"
#include <assert.h>
#include <string.h>
typedef struct source_fixture{
 pthread_mutex_t mutex;pthread_cond_t changed;
 unsigned blocked,entered,released,reads;
 pthread_t writer_thread;
}source_fixture;
long sl_linux_capture_read(sl_linux_capture_frame *frame){
 source_fixture *source=frame->writer_original;
 frame->bytes[0]='q';frame->raw_count=source->reads++?0:1;
 frame->read_returned=1;return frame->raw_count;
}
long sl_linux_capture_forward(sl_linux_capture_frame *frame){
 source_fixture *source=frame->writer_original;
 source->writer_thread=pthread_self();
 assert(pthread_mutex_lock(&source->mutex)==0);source->entered=1;
 assert(pthread_cond_signal(&source->changed)==0);
 while(source->blocked&&!source->released)assert(pthread_cond_wait(&source->changed,&source->mutex)==0);
 assert(pthread_mutex_unlock(&source->mutex)==0);
 frame->forwarded=frame->raw_count;frame->forward_returned=1;frame->public_signal_result=0;
 return frame->raw_count;
}
long sl_linux_write_signal_begin(sl_linux_write_signal *signal){signal->attempted=1;signal->blocked=1;return 0;}
long sl_linux_write_signal_finish(sl_linux_write_signal *signal){assert(signal->blocked);signal->finish_attempted=1;signal->restored=1;return 0;}
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 (void)call;(void)source;(void)number;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;
 assert(0);return -1; /* This success fixture must not enter unresolved hold. */
}
#include "../forward_workers_linux.c"
#include "../capture_workers_linux.c"
int main(void){
 source_fixture sources[2]={0};sl_linux_capture_worker readers[2]={0};
 sl_linux_capture_frame frames[2][2]={0};sl_linux_forward_worker forwards[2]={0};
 sl_linux_notification notifications[2][2]={0};sl_linux_forward_wait waits[2][8]={0};
 sl_linux_forward_iteration iterations[2][3]={0};sl_linux_reader_join raw_joins[2]={0};
 sl_linux_forward_release releases[2]={0};void *result;unsigned index,frame;
 sources[0].blocked=1;
 for(index=0;index<2;index++){
  assert(pthread_mutex_init(&sources[index].mutex,0)==0);assert(pthread_cond_init(&sources[index].changed,0)==0);
  readers[index].controller_original=sources;readers[index].frames=frames[index];readers[index].capacity=2;
  for(frame=0;frame<2;frame++){
   frames[index][frame].prepared=1;frames[index][frame].input_fd=7+index;
   frames[index][frame].writer_original=&sources[index];
  }
  forwards[index].reader_original=&readers[index];forwards[index].publications=notifications[index];
  forwards[index].publication_capacity=2;forwards[index].waits=waits[index];forwards[index].wait_capacity=8;
  forwards[index].iterations=iterations[index];forwards[index].iteration_capacity=3;
  assert(sl_linux_prepare_forward_worker(&forwards[index])==0);
 }
 assert(sl_linux_prepare_raw_readers(readers,2)==0);
 for(index=0;index<2;index++)assert(sl_linux_start_forward_worker(&forwards[index])==0);
 assert(sl_linux_start_raw_readers(readers,2)==0);
 assert(pthread_mutex_lock(&sources[0].mutex)==0);
 while(!sources[0].entered)assert(pthread_cond_wait(&sources[0].changed,&sources[0].mutex)==0);
 assert(pthread_mutex_unlock(&sources[0].mutex)==0);
 while(!atomic_load_explicit(&forwards[1].forwarding_finished,memory_order_acquire))atomic_signal_fence(memory_order_seq_cst);
 assert(!atomic_load_explicit(&forwards[0].forwarding_finished,memory_order_acquire));
 assert(frames[1][0].forwarded==1&&frames[0][0].forwarded==0);
 assert(!pthread_equal(sources[0].writer_thread,sources[1].writer_thread));
 /* Actual reader joins complete while source0 public forwarding is blocked. */
 for(index=0;index<2;index++){
  raw_joins[index].worker_original=&readers[index];raw_joins[index].attempted=1;
  raw_joins[index].result=pthread_join(readers[index].thread,&raw_joins[index].joined_result);
  raw_joins[index].returned=1;assert(raw_joins[index].result==0&&raw_joins[index].joined_result==&readers[index]);
  /* This fixture directly joins; interpret its actual output/body boundary. */
  raw_joins[index].interpretation_result=(raw_joins[index].joined_result!=&readers[index]||!atomic_load_explicit(&readers[index].returned,memory_order_acquire))?-1:raw_joins[index].result;
  raw_joins[index].interpretation_returned=1;
  readers[index].joined_by=&raw_joins[index];
 }
 assert(pthread_mutex_lock(&sources[0].mutex)==0);sources[0].released=1;
 assert(pthread_cond_signal(&sources[0].changed)==0);assert(pthread_mutex_unlock(&sources[0].mutex)==0);
 while(!atomic_load_explicit(&forwards[0].forwarding_finished,memory_order_acquire))atomic_signal_fence(memory_order_seq_cst);
 for(index=0;index<2;index++){
  releases[index].worker_original=&forwards[index];releases[index].raw_join_original=&raw_joins[index];
  assert(sl_linux_release_forward_source(&releases[index])==0);
  assert(pthread_join(forwards[index].thread,&result)==0&&result==&forwards[index]);
  assert(atomic_load_explicit(&forwards[index].returned,memory_order_acquire));
  assert(forwards[index].signal.restored&&frames[index][0].forwarded==1);
 }
 return 0;
}
#endif
