/* UNRUN: actual gate segment preflight/emission, no filesystem/native proof. */
#include "final_segment_test_support.h"
int main(void){
 static gate_case cases[4];
 for(unsigned m=0;m<4;m++){
  gate_case *e=&cases[m];sl_linux_final_capture *r=&e->c.final_capture;sl_linux_gate_delivery *g=&e->gate;
  r->prepared_controller=&e->c;r->actual_records=e->records;r->actual_capacity=24;r->actual_readback=e->bytes;r->actual_bytes=sizeof(e->bytes);r->actual_writes.calls=e->writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->reads;r->actual_reads.capacity=2;r->gate_original=g;
  g->controller_original=&e->c;g->birth_original=&e->c.entry.birth;g->bootstrap_original=&e->bootstrap;e->c.entry.birth.bootstrap=&e->bootstrap;g->bytes_original=e->bootstrap.gate_expected;memcpy(e->bootstrap.gate_expected,"12345678",8);
  g->attempted=g->returned=g->close_attempted=1;g->count=1;g->offset=8;
  g->writes[0].source="write";atomic_init(&g->writes[0].entered,1);atomic_init(&g->writes[0].returned,1);g->writes[0].result=8;
  g->close.source="close";atomic_init(&g->close.entered,1);atomic_init(&g->close.returned,1);
  if(m==1)atomic_store(&g->writes[0].returned,0);
  if(m==2){r->actual_records=(sl_linux_final_record*)&e->gate;r->actual_capacity=1;}
  if(m==3)r->actual_capacity=1;
  unsigned char original[sizeof(e->gate)];memcpy(original,g,sizeof(original));unsigned long count=0,extent=0;
  long result=final_gate(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(original,g,sizeof(original)));
  if(!m){assert(!result&&count==7&&extent==3*112+2*112+40+32+sizeof(g->signal.original_disposition));assert(!final_gate(r,1,&count,&extent));assert(r->count==7&&r->actual_records[3].source_original==&g->writes[0]&&r->actual_records[5].body==g->bytes_original);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}