/* UNRUN: notification native-return distinction and fixed-source lineage. */
#include "final_segment_test_support.h"
typedef struct schedule_case{bootstrap_case base;sl_linux_schedule_wake wake;sl_linux_schedule_wait wait;unsigned other;}schedule_case;
int main(void){
 static schedule_case cases[5];
 for(unsigned m=0;m<5;m++){
  schedule_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_schedule *s=&c->schedule;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  s->prepared=s->prepare_attempted=1;s->controller_original=c;s->actual_wakes=&e->wake;s->actual_wake_capacity=1;s->actual_waits=&e->wait;s->actual_wait_capacity=1;atomic_init(&s->generation,1);
  e->wake.schedule_original=s;e->wake.source_original=&c->channel;e->wake.generation_after=1;atomic_init(&e->wake.claimed,1);atomic_init(&e->wake.returned,1);e->wake.call.source="wake.fixture";atomic_init(&e->wake.call.entered,1);atomic_init(&e->wake.call.returned,1);
  e->wait.schedule_original=s;e->wait.observed=e->wait.returned=1;e->wait.result=-EINTR;atomic_init(&e->wait.claimed,1);e->wait.call.source="wait.fixture";e->wait.call.result=-EINTR;atomic_init(&e->wait.call.entered,1);atomic_init(&e->wait.call.returned,1);
  if(m==1)e->wake.source_original=&e->other;
  if(m==2)atomic_store(&e->wake.call.returned,0);
  if(m==3)r->actual_records=(sl_linux_final_record*)&e->wake;
  if(m==4)atomic_store(&e->wait.claimed,0);
  unsigned char before[sizeof(e->wake)];memcpy(before,&e->wake,sizeof(before));unsigned long count=0,extent=0;long result=final_schedule(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,&e->wake,sizeof(before)));
  if(!m){assert(!result&&count==6);assert(!final_schedule(r,1,&count,&extent));assert(r->count==6&&r->actual_records[2].source_original==&e->wake.call&&r->actual_records[4].source_original==&e->wait.call&&e->wait.result==-EINTR);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}