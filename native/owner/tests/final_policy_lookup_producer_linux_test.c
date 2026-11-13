/* UNRUN actual lookup producer -> FINAL helper consumer; no kernel admission. */
#include "final_segment_test_support.h"
typedef struct produced_storage {hashtab_val_t table;hashtab_ptr_t bucket[1];hashtab_node_t node;char key[5],name[5];int datum;} produced_storage;
typedef struct produced_case {bootstrap_case base;sl_linux_policy_parse parse;sl_selinux_te_union custody;sl_selinux_helper_visit helpers[2];sl_sepol_memory_attempt allocation;produced_storage storage;char external_name[5];hashtab_ptr_t external_bucket[1];int producer_result;} produced_case;
int main(void){
 static produced_case cases[5];
 for(unsigned mode=0;mode<5;mode++){
  produced_case *e=&cases[mode];sl_linux_final_capture *r=&e->base.c.final_capture;
  r->prepared_controller=&e->base.c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  sl_sepol_memory_owner *m=&e->parse.parser.memory;m->actual_bytes=(unsigned char*)&e->storage;m->used=m->actual_capacity=sizeof(e->storage);m->actual_attempts=&e->allocation;m->actual_attempt_capacity=m->attempt_count=1;
  e->allocation.operation=SL_SEPOL_ZERO_ALLOCATE;e->allocation.entered=e->allocation.returned=1;e->allocation.result=&e->storage;e->allocation.extent=sizeof(e->storage);
  e->storage.table.htable=e->storage.bucket;e->storage.table.size=e->storage.table.nel=1;e->storage.bucket[0]=&e->storage.node;e->storage.node.key=e->storage.key;e->storage.node.datum=&e->storage.datum;memcpy(e->storage.key,"long",5);
  sl_selinux_te_union *o=&e->custody;o->actual_policy=&e->parse.parser.policy;o->actual_helpers=e->helpers;o->actual_helper_capacity=2;o->actual_string_capacity=mode==3?3:16;o->actual_class_name=sl_selinux_name_file;
  if(mode==1){memcpy(e->storage.name,"file",5);o->actual_class_name=e->storage.name;}if(mode==2){memcpy(e->external_name,"file",5);o->actual_class_name=e->external_name;}
  if(mode==4){e->external_bucket[0]=&e->storage.node;e->storage.table.htable=e->external_bucket;}
  void *found=NULL;e->producer_result=relation_lookup(o,&e->storage.table,&o->actual_class_name,&found);
  assert(o->helper_count==2&&!found);assert(e->producer_result==(mode==3?-1:0));
  assert(e->helpers[1].name_slot_original==&o->actual_class_name&&e->helpers[1].name_original==o->actual_class_name);
  assert(e->helpers[1].name_source_original==((mode==0||mode==3||mode==4)?sl_selinux_lookup_source_for(sl_selinux_name_file):NULL));
  const char **slots[3]={&o->actual_class_name,NULL,NULL};unsigned long count=0,extent=0;unsigned char original[sizeof(e->helpers)];memcpy(original,e->helpers,sizeof(original));long result=0;
  for(unsigned j=0;j<2&&!result;j++)result=final_policy_helper(r,0,&count,&extent,o,m,&e->helpers[j],slots,4,j);
  assert(!r->count&&!r->extent&&!memcmp(original,e->helpers,sizeof(original)));
  if(mode==2||mode==4)assert(result<0);else{assert(!result&&count==14);for(unsigned j=0;j<2;j++)assert(!final_policy_helper(r,1,&count,&extent,o,m,&e->helpers[j],slots,4,j));assert(r->count==14);assert(r->actual_records[13].body==(const unsigned char*)e->storage.key&&r->actual_records[13].length==(mode==3?3:5));}
 }
 return 0;
}