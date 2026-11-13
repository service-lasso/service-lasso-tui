/* UNRUN: terminal/route fragment source preservation, not terminal proof. */
#include "final_segment_test_support.h"
int main(void){
 static bootstrap_case cases[4];
 for(unsigned m=0;m<4;m++){
  bootstrap_case *e=&cases[m];sl_linux_public_controller *c=&e->c;sl_linux_final_capture *r=&c->final_capture;
  r->prepared_controller=c;r->actual_records=e->records;r->actual_capacity=100;r->actual_readback=e->bytes;r->actual_bytes=sizeof(e->bytes);r->actual_writes.calls=e->writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->reads;r->actual_reads.capacity=2;
  c->terminal.attempted=1;c->terminal.input_original=&c->entry.public_streams[0];c->terminal.output_original=&c->entry.public_streams[1];
  c->terminal.input_query.source="terminal.fixture";atomic_init(&c->terminal.input_query.entered,1);atomic_init(&c->terminal.input_query.returned,1);
  for(unsigned i=0;i<3;i++){c->routes[i].attempted=1;c->routes[i].stream_index=(int)i;c->routes[i].public_original=&c->entry.public_streams[i];}
  if(m==1)c->routes[2].public_original=&c->entry.public_streams[1];
  if(m==2)atomic_store(&c->terminal.input_query.returned,0);
  if(m==3)r->actual_records=(sl_linux_final_record*)&c->private_state;
  unsigned char before[sizeof(*c)];memcpy(before,c,sizeof(before));unsigned long count=0,extent=0;long result=final_terminal(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,c,sizeof(before)));
  if(!m){assert(!result&&count==23);assert(!final_terminal(r,1,&count,&extent));assert(r->count==23&&r->actual_records[8].source_original==&c->terminal.input_query&&r->actual_records[9].body==(const unsigned char*)&c->terminal.input_state);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}