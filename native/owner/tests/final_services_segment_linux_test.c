/* UNRUN: every settled family source; not semantic cgroup/native proof. */
#include "final_segment_test_support.h"
typedef struct service_case{bootstrap_case base;sl_linux_service_observation service;}service_case;
int main(void){
 static service_case cases[5];
 for(unsigned m=0;m<5;m++){
  service_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_service_observation *o=&e->service;sl_linux_family_worker *w=&o->family;sl_linux_family_events *f=&w->observation;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  c->service_roster=o;c->service_capacity=1;c->service_index=0;o->attempted=o->returned=1;o->controller_original=c;
  w->attempted=w->create_entered=w->create_returned=1;atomic_init(&w->body_entered,1);atomic_init(&w->body_returned,1);w->root_terminal_original=&o->root;w->joined_by=&o->family_join;o->family_join.attempted=o->family_join.returned=1;o->family_join.worker_original=w;o->family_join.joined_result=f;
  f->attempted=f->returned=1;f->read_count=f->extent=1;f->reads[0].attempted=f->reads[0].returned=1;f->reads[0].count=1;f->reads[0].byte='X';f->reads[0].call.source="family.read.fixture";f->reads[0].call.result=1;atomic_init(&f->reads[0].call.entered,1);atomic_init(&f->reads[0].call.returned,1);
  if(m==1)atomic_store(&f->reads[0].call.returned,0);
  if(m==2)w->joined_by=NULL;
  if(m==3)r->actual_records=(sl_linux_final_record*)o;
  if(m==4)o->selinux.prior_original=(sl_linux_candidate_preexec_inspection*)&e->base.bootstrap;
  unsigned char before[sizeof(*f)];memcpy(before,f,sizeof(before));unsigned long count=0,extent=0;long result=final_services(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,f,sizeof(before)));
  if(!m){assert(!result&&count==14);assert(!final_services(r,1,&count,&extent));assert(r->count==14&&r->actual_records[12].source_original==&f->reads[0].call&&r->actual_records[13].body==&f->reads[0].byte);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 /* Fresh distinct service owners; headers carry coordinates independently of
  * ordering or the serializer-only source_original pointer. */
 static struct {bootstrap_case base;sl_linux_service_observation services[2];} multi;
 sl_linux_public_controller *c=&multi.base.c;sl_linux_final_capture *r=&c->final_capture;
 r->prepared_controller=c;r->actual_records=multi.base.records;r->actual_capacity=100;r->actual_readback=multi.base.bytes;r->actual_bytes=sizeof(multi.base.bytes);r->actual_writes.calls=multi.base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=multi.base.reads;r->actual_reads.capacity=2;
 c->service_roster=multi.services;c->service_capacity=2;
 for(unsigned i=0;i<2;i++){multi.services[i].attempted=multi.services[i].returned=1;multi.services[i].controller_original=c;}
 unsigned long count=0,extent=0;assert(!final_services(r,0,&count,&extent)&&count==10);
 assert(!final_services(r,1,&count,&extent)&&r->count==10);
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<5;j++){
  unsigned char expected[8];final_u64(expected,i);assert(!memcmp(r->actual_records[5*i+j].header+16,expected,8));
  final_u64(expected,j);assert(!memcmp(r->actual_records[5*i+j].header+24,expected,8));
 }
 return 0;
}