/* UNRUN actual owned diagnostic producer -> FINAL failure consumer. */
#include "final_segment_test_support.h"
#include "../sepol_source_owner.c"
#include "../sepol_diagnostic_owner.c"
typedef struct failure_case {bootstrap_case base;sl_linux_public_startup startup;sl_linux_policy_parse parse;sl_sepol_diagnostic diagnostic;sl_sepol_argument argument;sl_sepol_memory_attempt attempts[3];_Alignas(max_align_t) unsigned char memory[16];char output[128];} failure_case;
int main(void){
 static failure_case cases[3];
 for(unsigned mode=0;mode<3;mode++){
  failure_case *e=&cases[mode];sl_sepol_parse_owner *p=&e->parse.parser;sl_sepol_memory_owner *m=&p->memory;sl_sepol_diagnostic_owner *d=&p->diagnostics;sl_linux_final_capture *r=&e->base.c.final_capture;
  atomic_init(&m->claimed,0);atomic_init(&d->claimed,0);m->bytes=e->memory;m->capacity=mode?16:1;m->attempts=e->attempts;m->attempt_capacity=3;assert(!sl_sepol_memory_begin(m));
  if(mode)assert(sl_sepol_source_malloc(16,__FILE__,__LINE__)==e->memory);
  d->frames=&e->diagnostic;d->capacity=1;d->format_capacity=8;e->diagnostic.arguments=&e->argument;e->diagnostic.argument_capacity=1;e->diagnostic.output=e->output;e->diagnostic.output_capacity=sizeof(e->output);assert(!sl_sepol_diagnostics_begin(d));
  if(mode==2){assert(!sl_sepol_source_malloc(1,__FILE__,__LINE__));assert(!sl_sepol_source_current_owner()&&d->actual_memory==m);}
  sl_sepol_diagnostic_emit(NULL,1,"libsepol","failure","%s","abc");
  assert(e->argument.referent_owner==m&&e->argument.referent_entered&&e->argument.referent_returned&&e->argument.referent_result==-1&&!e->argument.referent_copy&&!e->argument.referent_copy_extent);
  assert(e->argument.referent_extent==(mode?4:1)&&e->diagnostic.format_returned&&e->diagnostic.format_result==3&&!strcmp(e->output,"abc"));
  assert(sl_sepol_diagnostics_finish(d)==-1);assert(sl_sepol_memory_finish(m)==(mode?-1:0));
  if(mode)assert(m->attempt_count==(mode==2?3:2)&&e->attempts[1].returned&&e->attempts[1].error==ENOMEM&&!e->attempts[1].result);
  e->parse.attempted=e->parse.returned=p->returned=1;e->startup.actual_parses=&e->parse;e->startup.actual_parse_capacity=1;e->base.c.startup_original=&e->startup;r->prepared_controller=&e->base.c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  unsigned long count=0,extent=0;assert(!final_parser_diagnostics(r,0,&count,&extent)&&count==9&&!r->count&&!r->extent);assert(!final_parser_diagnostics(r,1,&count,&extent)&&r->count==9);
  unsigned char expected[8];final_u64(expected,mode?4:1);assert(!memcmp(r->actual_records[7].body+32,expected,8));final_u64(expected,0);assert(!memcmp(r->actual_records[8].body+16,expected,8));assert(e->argument.referent_result==-1&&!e->argument.referent_copy);
 }
 return 0;
}
