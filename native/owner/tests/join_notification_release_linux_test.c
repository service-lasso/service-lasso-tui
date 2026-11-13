/* UNRUN source regression: actual join/release bodies, substituted native IO. */
#include "../forward_workers_linux.h"
#include "../schedule_linux.h"
#include <assert.h>
#include <errno.h>
static void *join_output;static unsigned joins;static long wake_result;
static int controlled_tryjoin(pthread_t thread,void **out){(void)thread;joins++;*out=join_output;return 0;}
#define pthread_tryjoin_np controlled_tryjoin
long sl_linux_capture_read(sl_linux_capture_frame *f){(void)f;assert(0);return -1;}
long sl_linux_capture_forward(sl_linux_capture_frame *f){(void)f;assert(0);return -1;}
long sl_linux_write_signal_begin(sl_linux_write_signal *s){(void)s;assert(0);return -1;}
long sl_linux_write_signal_finish(sl_linux_write_signal *s){(void)s;assert(0);return -1;}
long sl_linux_call6(sl_linux_call *o,const char *source,long n,long a,long b,long c,long d,long e,long f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;o->source=source;o->number=n;atomic_store(&o->entered,1);o->result=wake_result;atomic_store(&o->returned,1);return o->result;}
#include "../schedule_linux.c"
#include "../capture_workers_linux.c"
#include "../forward_workers_linux.c"
typedef struct case_original {
 sl_linux_schedule schedule;sl_linux_schedule_wake wake[1];sl_linux_schedule_wait wait[1];
 sl_linux_capture_worker reader;sl_linux_reader_join joined;sl_linux_forward_worker forward;sl_linux_forward_release release;int controller;
}case_original;
int main(void){
 case_original cases[4]={0};
 for(unsigned mode=0;mode<4;mode++){
  case_original *c=&cases[mode];sl_linux_schedule *s=&c->schedule;
  s->controller_original=(struct sl_linux_public_controller*)&c->controller;s->wakes=c->wake;s->wake_capacity=1;s->waits=c->wait;s->wait_capacity=1;s->interval_ns=1;
  c->wake[0].schedule_original=s;c->wake[0].source_original=mode==2?(void*)&c->controller:(void*)&c->reader;c->wait[0].schedule_original=s;
  assert(!sl_linux_prepare_schedule(s));wake_result=mode==0?-EIO:1;
  if(mode!=3)assert(sl_linux_schedule_notify(&c->wake[0])==wake_result);
  c->reader.terminal_wake=&c->wake[0];c->reader.create_returned=1;c->reader.outcome=SL_READER_READ_EOF;atomic_init(&c->reader.returned,1);atomic_init(&c->reader.completed_count,0);
  c->joined.worker_original=&c->reader;join_output=&c->reader;
  long expected=mode==0?-EIO:mode==1?0:-EINVAL;
  assert(sl_linux_observe_raw_reader_join(&c->joined)==expected);
  assert(c->joined.result==0&&c->joined.returned&&c->joined.interpretation_returned&&c->joined.interpretation_result==expected&&c->reader.joined_by==&c->joined);
  c->forward.reader_original=&c->reader;atomic_init(&c->forward.forwarding_finished,1);atomic_init(&c->forward.release_attempt_original,0);
  /* Every unrelated release prerequisite is genuinely satisfied here. */
  c->forward.terminal_notification.attempted=1;c->forward.terminal_notification.lock_returned=1;c->forward.terminal_notification.signal_returned=1;c->forward.terminal_notification.unlock_returned=1;
  assert(!pthread_mutex_init(&c->forward.mutex,0));assert(!pthread_cond_init(&c->forward.changed,0));
  c->release.worker_original=&c->forward;c->release.raw_join_original=&c->joined;
  long result=sl_linux_release_forward_source(&c->release);
  if(mode==1){assert(!result);assert(c->forward.release_original==&c->release);assert(atomic_load(&c->forward.release_attempt_original)==&c->release);assert(c->release.notification.lock_returned&&c->release.notification.signal_returned&&c->release.notification.unlock_returned);}
  else{assert(result==-1);assert(!atomic_load(&c->forward.release_attempt_original));assert(!c->forward.release_original&&!c->release.notification.attempted);}
  unsigned before=joins;assert(sl_linux_observe_raw_reader_join(&c->joined)==-1&&joins==before);assert(c->joined.interpretation_result==expected);
  assert(!pthread_cond_destroy(&c->forward.changed));assert(!pthread_mutex_destroy(&c->forward.mutex));
 }
 assert(cases[0].wake[0].result==-EIO&&cases[0].joined.interpretation_result==-EIO);assert(cases[1].joined.interpretation_result==0);
 return 0;
}