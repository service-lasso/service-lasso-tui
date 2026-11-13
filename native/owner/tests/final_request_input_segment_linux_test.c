/* UNRUN: settled input fragment originals; no actual pthread/terminal proof. */
#include "final_segment_test_support.h"
typedef struct input_case{bootstrap_case base;sl_linux_terminal_request request;sl_linux_input_frame frame;sl_linux_input_step steps[4];}input_case;
int main(void){
 static input_case cases[5];
 for(unsigned m=0;m<5;m++){
  input_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_terminal_request *q=&e->request;sl_linux_input_worker *w=&q->input;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  c->channel.attempted=1;c->channel.prepared_requests=q;c->channel.prepared_capacity=1;q->prefix.attempted=1;
  w->start_attempted=w->start_returned=w->prepared=w->create_entered=w->create_returned=1;w->owner_original=q;w->mode_original=&q->mode;w->prepared_frames=&e->frame;w->prepared_capacity=w->count=1;w->stop_original=&q->input_stop;w->worker_result=w;
  atomic_init(&w->entered,1);atomic_init(&w->returned,1);q->input_stop.worker_original=w;q->input_stop.join_returned=q->input_stop.interpretation_returned=1;q->input_stop.joined_result=w;
  e->frame.prepared_steps=e->steps;e->frame.prepared_capacity=e->frame.count=4;e->frame.raw_count=e->frame.written=1;e->frame.bytes[0]='X';
  for(unsigned j=0;j<4;j++){e->steps[j].kind=j+1;e->steps[j].call.source="input.step.fixture";e->steps[j].call.result=1;atomic_init(&e->steps[j].call.entered,1);atomic_init(&e->steps[j].call.returned,1);}
  if(m==1)q->input_stop.joined_result=q;
  if(m==2)atomic_store(&e->steps[3].call.returned,0);
  if(m==3)r->actual_records=(sl_linux_final_record*)&e->frame;
  if(m==4)e->frame.written=2;
  unsigned char before[sizeof(e->frame)];memcpy(before,&e->frame,sizeof(before));unsigned long count=0,extent=0;long result=final_request_input(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,&e->frame,sizeof(before)));
  if(!m){assert(!result&&count==23);assert(!final_request_input(r,1,&count,&extent));assert(r->count==23&&r->actual_records[10].body==e->frame.bytes&&r->actual_records[12].source_original==&e->steps[0].call);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}