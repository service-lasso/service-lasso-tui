/* UNRUN closed traversal fragment, not parser/kernel/whole-prefix proof. */
#include "final_segment_test_support.h"
typedef struct traversal_case {
 bootstrap_case base;sl_linux_public_startup startup;sl_linux_policy_parse parse;
 sl_linux_migration_labels migration;sl_linux_control_access control;sl_selinux_rule_visit visit;sl_selinux_helper_visit helper;
 sl_sepol_memory_attempt allocation;_Alignas(struct avtab_node) unsigned char node_bytes[sizeof(struct avtab_node)+sizeof(avtab_ptr_t)];
} traversal_case;
int main(void){
 static traversal_case cases[8];
 for(unsigned mode=0;mode<8;mode++){
  traversal_case *e=&cases[mode];sl_linux_final_capture *r=&e->base.c.final_capture;
  r->prepared_controller=&e->base.c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  e->base.c.startup_original=&e->startup;e->startup.actual_control_access=&e->control;e->startup.actual_control_access_capacity=1;e->startup.actual_parses=&e->parse;e->startup.actual_parse_capacity=1;
  e->parse.returned=e->parse.parser.returned=1;sl_sepol_memory_owner *m=&e->parse.parser.memory;
  m->actual_bytes=e->node_bytes;m->used=m->actual_capacity=sizeof(e->node_bytes);m->actual_attempts=&e->allocation;m->attempt_count=m->actual_attempt_capacity=1;m->finished=1;
  e->allocation.operation=SL_SEPOL_ALLOCATE;e->allocation.entered=e->allocation.returned=1;e->allocation.result=e->node_bytes;e->allocation.extent=sizeof(e->node_bytes);
  sl_selinux_te_union *o=&e->control.inventory.custody;o->attempted=o->returned=1;o->policy_original=o->actual_policy=&e->parse.parser.policy;o->actual_visits=&e->visit;o->actual_capacity=o->count=1;o->actual_helpers=&e->helper;o->actual_helper_capacity=o->helper_count=1;
  e->visit.table_original=&e->parse.parser.policy.te_avtab;e->visit.node_original=(struct avtab_node*)e->node_bytes;e->helper.kind=SL_SELINUX_AVTAB_BUCKET;e->helper.avtab_original=e->visit.table_original;e->visit.table_original->htable=(avtab_ptr_t*)(e->node_bytes+sizeof(struct avtab_node));e->visit.table_original->nslot=1;e->visit.table_original->htable[0]=e->visit.node_original;
  if(mode==1)o->returned=0;if(mode==2)o->actual_policy=(policydb_t*)&e->base.bootstrap;if(mode==3)e->allocation.returned=0;
  if(mode==4)e->visit.conditional=1;if(mode==5)o->helper_count=2;if(mode==6)r->actual_records=(sl_linux_final_record*)&e->helper;
  if(mode==7){o->attempted=0;e->startup.migration_labels=&e->migration;e->startup.migration_label_capacity=1;e->migration.labels.returned=1;sl_selinux_te_union *t=&e->migration.labels.targets.custody,*f=&e->migration.labels.filesystem.custody;t->attempted=t->returned=f->attempted=f->returned=1;t->policy_original=t->actual_policy=f->actual_policy=&e->parse.parser.policy;}
  unsigned char original[sizeof(e->helper)];memcpy(original,&e->helper,sizeof(original));unsigned long count=0,extent=0;
  long result=final_policy_traversals(r,0,&count,&extent);assert(!r->count&&!r->extent&&!memcmp(original,&e->helper,sizeof(original)));
  if(!mode){assert(!result&&count==10);assert(!final_policy_traversals(r,1,&count,&extent)&&r->count==10);assert(r->actual_records[4].body==e->node_bytes&&r->actual_records[9].body==(const unsigned char*)&e->helper);unsigned char coordinate[8];final_u64(coordinate,UINT64_MAX-29);assert(!memcmp(r->actual_records[9].header+8,coordinate,8));}
  else if(mode==7){assert(!result&&count==6&&!e->migration.labels.filesystem.custody.policy_original);assert(!final_policy_traversals(r,1,&count,&extent)&&r->count==6);}
  else assert(result<0);
 }
 return 0;
}