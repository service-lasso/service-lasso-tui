/* UNRUN current datum body custody, not policy/native/full-prefix proof. */
#include "final_segment_test_support.h"
typedef struct datum_storage {class_datum_t class_value;perm_datum_t permission;common_datum_t common;ebitmap_t source,target;} datum_storage;
typedef struct datum_case {bootstrap_case base;sl_linux_policy_parse parse;sl_selinux_te_union custody;sl_sepol_memory_attempt allocation;datum_storage storage;perm_datum_t external;} datum_case;
int main(void){
 static datum_case cases[6];
 for(unsigned mode=0;mode<6;mode++){
  datum_case *e=&cases[mode];sl_linux_final_capture *r=&e->base.c.final_capture;
  r->prepared_controller=&e->base.c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  sl_sepol_memory_owner *m=&e->parse.parser.memory;m->actual_bytes=(unsigned char*)&e->storage;m->used=m->actual_capacity=sizeof(e->storage);m->actual_attempts=&e->allocation;m->actual_attempt_capacity=m->attempt_count=1;
  e->allocation.operation=SL_SEPOL_ZERO_ALLOCATE;e->allocation.entered=e->allocation.returned=1;e->allocation.result=&e->storage;e->allocation.extent=sizeof(e->storage);
  sl_selinux_te_union *o=&e->custody;o->actual_policy=&e->parse.parser.policy;o->class_original=&e->storage.class_value;o->permission_original=&e->storage.permission;o->common_original=&e->storage.common;o->source_attributes=&e->storage.source;o->target_attributes=&e->storage.target;
  if(mode==1)e->allocation.returned=0;if(mode==2)o->permission_original=&e->external;if(mode==3)e->allocation.extent=sizeof(class_datum_t);if(mode==4)r->actual_records=(sl_linux_final_record*)&e->storage;if(mode==5)o->class_original=(class_datum_t*)((unsigned char*)&e->storage.class_value+1);
  datum_storage original;memcpy(&original,&e->storage,sizeof(original));unsigned long count=0,extent=0;long result=final_policy_datums(r,0,&count,&extent,o,m,3);
  assert(!r->count&&!r->extent&&!memcmp(&original,&e->storage,sizeof(original)));
  if(!mode){assert(!result&&count==10);assert(!final_policy_datums(r,1,&count,&extent,o,m,3));assert(r->count==10&&r->actual_records[1].body==(const unsigned char*)&e->storage.class_value&&r->actual_records[9].body==(const unsigned char*)&e->storage.target);}
  else assert(result<0);
 }
 return 0;
}