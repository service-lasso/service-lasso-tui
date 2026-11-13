/* UNRUN: full prepared request frontier, not protocol/LOCAL authenticity. */
#include "final_segment_test_support.h"
typedef struct channel_case{bootstrap_case base;sl_linux_terminal_request requests[2];sl_linux_call receive;}channel_case;
int main(void){
 static channel_case cases[5];
 for(unsigned m=0;m<5;m++){
  channel_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_owner_channel *o=&c->channel;sl_linux_terminal_request *q=&e->requests[1];
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  o->attempted=o->prepared=o->returned=1;o->prepared_collection=&c->collection;o->prepared_terminal=&c->terminal;o->prepared_requests=e->requests;o->prepared_capacity=2;o->prepared_endpoint=9;
  /* Returned count deliberately is not the original-custody frontier. */
  o->count=0;o->local_prefix_original=&q->prefix;c->collection.header_original=&q->prefix;
  q->prefix.attempted=q->prefix.prepared=q->prefix.returned=1;q->prefix.prepared_endpoint=9;q->prefix.prepared_receive.attempts=&e->receive;q->prefix.prepared_receive.capacity=q->prefix.prepared_receive.count=1;q->prefix.prepared_receive.completed=8;
  e->receive.source="prefix.fixture";atomic_init(&e->receive.entered,1);atomic_init(&e->receive.returned,1);
  if(m==1)atomic_store(&e->receive.returned,0);
  if(m==2)r->actual_records=(sl_linux_final_record*)e->requests;
  if(m==3)c->collection.header_original=&e->requests[0].prefix;
  if(m==4)q->prefix.prepared_receive.completed=9;
  unsigned char before[sizeof(e->requests)];memcpy(before,e->requests,sizeof(before));unsigned long count=0,extent=0;long result=final_channel(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,e->requests,sizeof(before)));
  if(!m){assert(!result&&count==17);assert(!final_channel(r,1,&count,&extent));assert(r->count==17&&r->actual_records[13].source_original==&e->receive&&r->actual_records[15].body==q->prefix.bytes);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}