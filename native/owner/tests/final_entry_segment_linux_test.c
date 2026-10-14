/* UNRUN: actual embedded-entry preflight/emission; not complete prefix. */
#include "final_segment_test_support.h"
int main(void){
 static gate_case cases[4];static unsigned char labels[4][2][1];
 for(unsigned m=0;m<4;m++){
  gate_case *e=&cases[m];sl_linux_final_capture *r=&e->c.final_capture;sl_linux_entry *entry=&e->c.entry;
  r->prepared_controller=&e->c;r->actual_records=e->records;r->actual_capacity=24;r->actual_readback=e->bytes;r->actual_bytes=sizeof(e->bytes);r->actual_writes.calls=e->writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->reads;r->actual_reads.capacity=2;entry->attempted=1;
  for(unsigned i=0;i<2;i++){sl_linux_socket_peer *p=&entry->custody_peers[i];p->returned=p->framing_complete=1;p->actual_bytes=labels[m][i];p->actual_capacity=p->extent=1;labels[m][i][0]='a'+i;entry->custody_states[i].returned=entry->custody_states[i].framing_complete=1;}
  if(m==1)entry->custody_peers[1].actual_bytes=e->records[0].header;
  if(m==2)entry->custody_states[1].returned=0;
  if(m==3){r->actual_records=(sl_linux_final_record*)entry;r->actual_capacity=1;}
  unsigned char original[sizeof(*entry)];memcpy(original,entry,sizeof(original));unsigned long count=0,extent=0;
  long result=final_entry(r,0,&count,&extent);assert(!r->count&&!r->extent&&!memcmp(original,entry,sizeof(original)));
  if(!m){assert(!result&&count==18&&extent==12*112+6*32+2+4*sizeof(struct sockaddr_un));assert(!final_entry(r,1,&count,&extent));assert(r->count==18&&r->actual_records[12].body==labels[m][0]&&r->actual_records[15].body==labels[m][1]);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}