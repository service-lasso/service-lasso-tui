/* UNRUN: full join roster retains EBUSY before successful native result. */
#include "final_segment_test_support.h"
typedef struct collector_case{bootstrap_case base;sl_linux_collector_join joins[2];}collector_case;
int main(void){
 static collector_case cases[6];
 for(unsigned m=0;m<6;m++){
  collector_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  c->collector_create_entered=c->collector_create_returned=1;c->collector_join_roster=e->joins;c->collector_join_capacity=2;c->collector_joined_by=&e->joins[1];c->collector_join_index=0;
  atomic_init(&c->collector_entered,1);atomic_init(&c->collector_returned,1);
  for(unsigned j=0;j<2;j++){e->joins[j].controller_original=c;e->joins[j].attempted=e->joins[j].returned=e->joins[j].interpretation_returned=1;}
  e->joins[0].result=e->joins[0].interpretation_result=EBUSY;e->joins[1].joined_result=&c->collection;e->joins[1].channel_original=&c->channel;e->joins[1].collection_original=&c->collection;e->joins[1].channel_result=e->joins[1].collection_result=8;
  if(m==1)e->joins[0].controller_original=NULL;
  if(m==2)r->actual_records=(sl_linux_final_record*)e->joins;
  if(m==3)c->collector_joined_by=(sl_linux_collector_join*)&e->base.bootstrap;
  if(m==4)e->joins[1].channel_original=(sl_linux_owner_channel*)&e->base.bootstrap;
  if(m==5)e->joins[1].interpretation_returned=0;
  unsigned char before[sizeof(e->joins)];memcpy(before,e->joins,sizeof(before));unsigned long count=0,extent=0;long result=final_collector(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,e->joins,sizeof(before)));
  if(!m){assert(!result&&count==8);assert(!final_collector(r,1,&count,&extent));assert(r->count==8&&r->actual_records[4].source_original==&e->joins[0]&&r->actual_records[6].source_original==&e->joins[1]&&e->joins[0].result==EBUSY);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}