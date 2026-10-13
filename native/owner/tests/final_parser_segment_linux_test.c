/* UNRUN: parser metadata/original attempts, no parser invocation or qualification. */
#include "final_segment_test_support.h"
typedef struct parser_case {bootstrap_case base;sl_linux_public_startup startup;sl_linux_policy_parse parse;sl_linux_selinux_snapshot snapshot;sl_sepol_memory_attempt attempts[2];unsigned char input[1],arena[2];} parser_case;
int main(void){
 static parser_case cases[9];
 for(unsigned m=0;m<9;m++){
  parser_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_policy_parse *root=&e->parse;sl_sepol_parse_owner *p=&root->parser;sl_sepol_memory_owner *memory=&p->memory;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;c->startup_original=&e->startup;
  e->startup.actual_parses=root;e->startup.actual_parse_capacity=1;e->startup.actual_policies=&e->snapshot;e->startup.actual_policy_capacity=1;
  root->attempted=root->returned=p->attempted=p->returned=1;root->snapshot_original=root->actual_snapshot=&e->snapshot;e->snapshot.actual_body=e->input;e->snapshot.actual_body_capacity=e->snapshot.length=1;p->body=p->actual_body=e->input;p->extent=p->actual_extent=1;p->read_cursor=(char*)e->input+1;
  memory->actual_bytes=e->arena;memory->actual_capacity=2;memory->actual_attempts=e->attempts;memory->actual_attempt_capacity=memory->attempt_count=2;memory->used=1;memory->finished=1;
  e->attempts[0].operation=SL_SEPOL_ALLOCATE;e->attempts[0].source="parser.fixture";e->attempts[0].result=e->arena;e->attempts[0].extent=1;e->attempts[0].entered=e->attempts[0].returned=1;
  e->attempts[1].operation=SL_SEPOL_LOGICAL_FREE;e->attempts[1].source="parser.fixture";e->attempts[1].input=e->arena;e->attempts[1].previous_allocation=&e->attempts[0];e->attempts[1].entered=e->attempts[1].returned=1;e->attempts[1].error=EINVAL;memory->first_failure=&e->attempts[1];
  if(m==1)root->returned=0;if(m==2)memory->active=1;if(m==3)p->read_cursor=(char*)&e->base.bootstrap;if(m==4)r->actual_records=(sl_linux_final_record*)e->attempts;if(m==5)e->attempts[1].previous_allocation=(sl_sepol_memory_attempt*)&e->base.bootstrap;
  if(m>=6){e->attempts[0].operation=SL_SEPOL_DUPLICATE;e->attempts[0].extent=2;memory->used=2;e->arena[0]='x';e->arena[1]=0;}
  if(m==7)e->attempts[0].result=e->input;if(m==8)e->attempts[0].extent=3;
  unsigned char before[sizeof(e->attempts)];memcpy(before,e->attempts,sizeof(before));unsigned long count=0,extent=0;long result=final_parsers(r,0,&count,&extent);assert(!r->count&&!r->extent&&!memcmp(before,e->attempts,sizeof(before)));
  if(!m){assert(!result&&count==14);assert(!final_parsers(r,1,&count,&extent)&&r->count==14);unsigned char expected[8];final_u64(expected,0);assert(!memcmp(r->actual_records[11].body+56,expected,8));final_u64(expected,EINVAL);assert(!memcmp(r->actual_records[11].body+64,expected,8));assert(e->attempts[1].error==EINVAL&&memory->first_failure==&e->attempts[1]);}
  else if(m==6){assert(!result&&count==15);assert(!final_parsers(r,1,&count,&extent)&&r->count==15);assert(r->actual_records[10].body==e->arena&&r->actual_records[10].length==2&&!memcmp(r->actual_records[10].body,"x",2));}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}
