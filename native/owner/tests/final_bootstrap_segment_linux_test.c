/* UNRUN: shared bootstrap source segment with truthful nonreturning exec cell. */
#include "final_segment_test_support.h"
int main(void){
 static bootstrap_case cases[4];
 for(unsigned m=0;m<4;m++){
  bootstrap_case *e=&cases[m];sl_linux_final_capture *r=&e->c.final_capture;sl_linux_bootstrap *b=&e->bootstrap;
  r->prepared_controller=&e->c;r->actual_records=e->records;r->actual_capacity=100;r->actual_readback=e->bytes;r->actual_bytes=sizeof(e->bytes);r->actual_writes.calls=e->writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->reads;r->actual_reads.capacity=2;e->c.entry.birth.attempted=1;e->c.entry.birth.bootstrap=b;
  b->execute.source="exec.original";b->execute.number=SYS_execveat;atomic_init(&b->execute.entered,1);atomic_init(&b->execute.returned,0);
  if(m==1){b->create_session.source="unreturned.original";atomic_init(&b->create_session.entered,1);atomic_init(&b->create_session.returned,0);}
  if(m==2){b->authority.credentials_after.status=e->records[0].header;b->authority.credentials_after.length=b->authority.credentials_after.status_capacity=1;}
  if(m==3){b->fd_action_count=1;b->fd_actions=NULL;}
  unsigned char original[sizeof(*b)];memcpy(original,b,sizeof(original));unsigned long count=0,extent=0;
  long result=final_bootstrap(r,0,&count,&extent);assert(!r->count&&!r->extent&&!memcmp(original,b,sizeof(original)));
  if(!m){assert(!result&&count==31&&extent<sizeof(e->bytes));assert(!final_bootstrap(r,1,&count,&extent));assert(r->count==31&&r->actual_records[3].source_original==&b->execute&&decoded(r->actual_records[3].body+64)==1&&!decoded(r->actual_records[3].body+72));assert(!atomic_load(&b->execute.returned));}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}