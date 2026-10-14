/* UNRUN: namespace/realm fragment source ownership; not kernel authority. */
#include "final_segment_test_support.h"
typedef struct namespace_case {bootstrap_case base;sl_linux_call read;unsigned char raw[1];} namespace_case;
int main(void){
 static namespace_case cases[4];
 for(unsigned m=0;m<4;m++){
  namespace_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_parent_namespace *o=&c->namespace_observation;sl_linux_pidfd_proc *info=&o->pidfd_info;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  o->attempted=o->returned=1;o->birth_original=&c->entry.birth;info->returned=info->eof=info->pid_seen=1;info->birth_original=o->birth_original;info->procfs_original=&o->views[0];info->actual_bytes=e->raw;info->length=info->actual_capacity=1;info->actual_reads=&e->read;info->read_count=info->actual_read_capacity=1;e->raw[0]='x';
  e->read.source="returned.fixture";atomic_init(&e->read.entered,1);atomic_init(&e->read.returned,1);
  if(m==1)info->actual_bytes=e->base.records[0].header;
  if(m==2)atomic_store(&e->read.returned,0);
  if(m==3)info->procfs_original=&o->views[1];
  unsigned char before[sizeof(*o)];memcpy(before,o,sizeof(before));unsigned long count=0,extent=0;long result=final_namespace(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,o,sizeof(before)));
  if(!m){assert(!result&&count==28);assert(!final_namespace(r,1,&count,&extent));assert(r->count==28&&r->actual_records[9].source_original==&e->read&&r->actual_records[15].body==e->raw);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}