/* UNRUN helper referents, no parser/kernel/whole-prefix admission. */
#include "final_segment_test_support.h"
typedef struct helper_storage {hashtab_val_t table;hashtab_node_t node;char key[5];} helper_storage;
typedef struct helper_final_case {bootstrap_case base;sl_linux_policy_parse parse;sl_selinux_te_union custody;sl_selinux_helper_visit helper;sl_sepol_memory_attempt allocation;helper_storage storage;char external_name[5];} helper_final_case;
int main(void){
 static helper_final_case cases[11];
 for(unsigned mode=0;mode<11;mode++){
  helper_final_case *e=&cases[mode];sl_linux_final_capture *r=&e->base.c.final_capture;
  r->prepared_controller=&e->base.c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  sl_sepol_memory_owner *m=&e->parse.parser.memory;m->actual_bytes=(unsigned char*)&e->storage;m->actual_capacity=m->used=sizeof(e->storage);m->actual_attempts=&e->allocation;m->actual_attempt_capacity=m->attempt_count=1;
  e->allocation.operation=SL_SEPOL_ZERO_ALLOCATE;e->allocation.entered=e->allocation.returned=1;e->allocation.result=&e->storage;e->allocation.extent=sizeof(e->storage);
  e->custody.actual_policy=&e->parse.parser.policy;e->custody.actual_string_capacity=16;
  sl_selinux_helper_visit *v=&e->helper;v->kind=SL_SELINUX_HASH_NODE;v->table_original=&e->storage.table;v->hash_original=&e->storage.node;e->custody.actual_class_name=sl_selinux_name_file;v->name_original=e->custody.actual_class_name;v->name_slot_original=&e->custody.actual_class_name;v->name_source_original=sl_selinux_lookup_source_for(v->name_original);v->key_original=e->storage.key;memcpy(e->storage.key,"long",5);e->storage.node.key=e->storage.key;v->compared=v->key_read_extent=5;v->name_read_extent=5;v->name_terminated=v->key_terminated=1;
  if(mode==1)e->allocation.returned=0;if(mode==2)v->key_original="long";if(mode==3)v->key_read_extent=4;if(mode==4)v->name_read_extent=17;if(mode==5)r->actual_records=(sl_linux_final_record*)&e->storage;if(mode==6)v->name_slot_original=&e->custody.actual_permission_name;if(mode==7)v->name_read_extent=6;if(mode==8)v->name_source_original=(const sl_selinux_lookup_source*)&e->allocation;if(mode==9){e->custody.actual_class_name=v->name_original=e->storage.key;v->name_source_original=NULL;}if(mode==10){memcpy(e->external_name,"file",5);e->custody.actual_class_name=v->name_original=e->external_name;v->name_source_original=NULL;}
  const char **slots[3]={&e->custody.actual_class_name,NULL,NULL};unsigned char original[sizeof(e->storage)];memcpy(original,&e->storage,sizeof(original));unsigned long count=0,extent=0;long result=final_policy_helper(r,0,&count,&extent,&e->custody,m,v,slots,9,7);
  assert(!r->count&&!r->extent&&!memcmp(original,&e->storage,sizeof(original)));
  if(!mode||mode==9){assert(!result&&count==9);assert(!final_policy_helper(r,1,&count,&extent,&e->custody,m,v,slots,9,7)&&r->count==9);assert(r->actual_records[6].body==(const unsigned char*)v->name_original&&r->actual_records[6].length==5);assert(r->actual_records[8].body==(const unsigned char*)e->storage.key&&r->actual_records[8].length==5);unsigned char expected[8];final_u64(expected,33);assert(!memcmp(r->actual_records[8].header,expected,8));final_u64(expected,7);assert(!memcmp(r->actual_records[8].header+16,expected,8));}
  else assert(result<0);
 }
 return 0;
}