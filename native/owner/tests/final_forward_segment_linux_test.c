/* UNRUN: forwarding original-state fragment, not native-thread acceptance. */
#include "final_segment_test_support.h"
typedef struct forward_case{bootstrap_case base;sl_linux_source_cut cut;sl_linux_capture_worker raw;sl_linux_capture_frame frame;sl_linux_forward_worker forward;sl_linux_reader_join raw_join;sl_linux_forward_join forward_join;sl_linux_forward_release release;sl_linux_notification publication;sl_linux_forward_wait wait;}forward_case;
int main(void){
 static forward_case cases[5];
 for(unsigned m=0;m<5;m++){
  forward_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_forward_worker *w=&e->forward;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  c->drained_by=&e->cut;c->readers=&e->raw;c->forwarders=w;c->reader_count=e->cut.reader_count=1;e->cut.readers_original=c->readers;e->cut.forwarders_original=w;
  e->raw.forward_original=w;e->raw.joined_by=&e->raw_join;e->raw.prepared_frames=&e->frame;e->raw.prepared_capacity=1;w->reader_original=&e->raw;w->joined_by=&e->forward_join;w->release_original=&e->release;w->prepared_frames=&e->frame;w->prepared_frame_capacity=1;w->prepared_publications=&e->publication;w->prepared_waits=&e->wait;w->prepared_wait_capacity=1;
  e->raw_join.worker_original=&e->raw;e->raw_join.returned=e->raw_join.interpretation_returned=1;e->raw_join.joined_result=&e->raw;e->forward_join.worker_original=w;e->forward_join.returned=e->forward_join.interpretation_returned=1;e->forward_join.joined_result=w;e->release.worker_original=w;e->release.raw_join_original=&e->raw_join;
  atomic_init(&e->raw.entered,1);atomic_init(&e->raw.returned,1);atomic_init(&e->raw.completed_count,0);atomic_init(&w->entered,1);atomic_init(&w->forwarding_finished,1);atomic_init(&w->returned,1);atomic_init(&w->release_attempt_original,&e->release);
  if(m==1)e->forward_join.interpretation_result=-77;
  if(m==2){w->wait_count=1;e->wait.attempted=1;}
  if(m==3)r->actual_records=(sl_linux_final_record*)&w->signal.original_disposition;
  if(m==4)w->prepared_publications=NULL;
  unsigned char before[sizeof(*w)];memcpy(before,w,sizeof(before));unsigned long count=0,extent=0;long result=final_forward(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,w,sizeof(before)));
  if(!m){assert(!result&&count==11);assert(!final_forward(r,1,&count,&extent));assert(r->count==11&&r->actual_records[1].source_original==&e->raw_join&&r->actual_records[4].source_original==&e->forward_join);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}