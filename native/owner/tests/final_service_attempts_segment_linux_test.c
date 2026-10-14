/* UNRUN source fragment: full earlier service outcomes, no native join proof. */
#include "final_segment_test_support.h"
typedef struct attempts_case {
 bootstrap_case base;sl_linux_service_observation services[2];
 sl_linux_reader_join raw[2];sl_linux_forward_join forwards[2];sl_linux_forward_release releases[2];
 sl_linux_capture_worker reader;sl_linux_forward_worker forward;
} attempts_case;
int main(void){
 static attempts_case cases[6];
 for(unsigned m=0;m<6;m++){
  attempts_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  c->service_roster=e->services;c->service_capacity=2;c->readers=&e->reader;c->forwarders=&e->forward;c->reader_count=1;
  for(unsigned i=0;i<2;i++){
   sl_linux_service_observation *o=&e->services[i];o->prepared=o->attempted=o->returned=1;o->controller_original=c;o->capacity=1;o->raw_joins=&e->raw[i];o->forward_joins=&e->forwards[i];o->forward_releases=&e->releases[i];
   e->raw[i].worker_original=&e->reader;e->raw[i].attempted=e->raw[i].returned=e->raw[i].interpretation_returned=1;
  }
  e->raw[0].result=e->raw[0].interpretation_result=EBUSY;
  e->raw[1].joined_result=&e->reader;e->reader.joined_by=&e->raw[1];
  e->forwards[1].worker_original=&e->forward;e->forwards[1].attempted=e->forwards[1].returned=e->forwards[1].interpretation_returned=1;e->forwards[1].result=e->forwards[1].interpretation_result=EBUSY;
  e->releases[1].worker_original=&e->forward;e->releases[1].raw_join_original=&e->raw[0];e->releases[1].attempted=e->releases[1].returned=1;e->releases[1].result=-77;e->releases[1].notification.attempted=e->releases[1].notification.lock_returned=1;e->releases[1].notification.lock_result=77;
  if(m==1)e->raw[0].worker_original=NULL;
  if(m==2)e->raw[0].interpretation_returned=0;
  if(m==3)r->actual_records=(sl_linux_final_record*)e->raw;
  if(m==4)e->releases[1].raw_join_original=(sl_linux_reader_join*)&e->base.bootstrap;
  if(m==5)e->services[0].capacity=2;
  unsigned char before[sizeof(e->raw)];memcpy(before,e->raw,sizeof(before));unsigned long count=0,extent=0;
  long result=final_service_attempts(r,0,&count,&extent);assert(!r->count&&!r->extent&&!memcmp(before,e->raw,sizeof(before)));
  if(!m){assert(!result&&count==5);assert(!final_service_attempts(r,1,&count,&extent)&&r->count==5);unsigned char expected[8];final_u64(expected,EBUSY);assert(!memcmp(r->actual_records[0].body+24,expected,8));final_u64(expected,0);assert(!memcmp(r->actual_records[3].body+32,expected,8));assert(e->raw[0].result==EBUSY&&e->releases[1].result==-77);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}
