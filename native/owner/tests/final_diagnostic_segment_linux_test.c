/* UNRUN: exact diagnostic frame/output backing, no formatter/parser execution. */
#include "final_segment_test_support.h"
typedef struct diagnostic_case {bootstrap_case base;sl_linux_public_startup startup;sl_linux_policy_parse parse;sl_sepol_diagnostic frame;sl_sepol_argument argument;unsigned char output[4],copy[6];sl_sepol_memory_attempt allocation;} diagnostic_case;
int main(void){
 static diagnostic_case cases[11];
 for(unsigned m=0;m<11;m++){
  diagnostic_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_sepol_diagnostic_owner *o=&e->parse.parser.diagnostics;sl_sepol_diagnostic *f=&e->frame;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;c->startup_original=&e->startup;e->startup.actual_parses=&e->parse;e->startup.actual_parse_capacity=1;
  e->parse.attempted=e->parse.returned=e->parse.parser.returned=1;o->actual_frames=f;o->actual_capacity=o->count=1;o->actual_format_capacity=3;o->finished=1;o->first_failure=f;
  f->entered=f->returned=f->arguments_complete=f->format_entered=f->format_returned=1;f->format="%s";f->format_length=2;f->actual_arguments=&e->argument;f->actual_argument_capacity=f->argument_count=1;e->argument.kind=SL_ARG_STRING;e->argument.value.string="value";f->actual_output=(char*)e->output;f->actual_output_capacity=4;e->output[0]='x';f->format_result=-77;f->result=-1;
  sl_sepol_memory_owner *memory=&e->parse.parser.memory;o->actual_memory=memory;memory->actual_bytes=e->copy;memory->actual_capacity=memory->used=6;memory->actual_attempts=&e->allocation;memory->actual_attempt_capacity=memory->attempt_count=1;e->allocation.entered=e->allocation.returned=1;e->allocation.operation=SL_SEPOL_ALLOCATE;e->allocation.result=e->copy;e->allocation.extent=6;memcpy(e->copy,"value",6);
  e->argument.referent_owner=memory;e->argument.referent_original=e->argument.value.string;e->argument.referent_copy=e->copy;e->argument.referent_extent=e->argument.referent_copy_extent=6;e->argument.referent_entered=e->argument.referent_returned=e->argument.referent_terminated=1;
  if(m==7)e->argument.referent_copy=e->output;if(m==8)e->argument.referent_returned=0;if(m==9)e->argument.referent_owner=(sl_sepol_memory_owner*)&e->base.bootstrap;if(m==10){memory->actual_bytes=(unsigned char*)r->actual_records;e->allocation.result=r->actual_records;e->argument.referent_copy=(unsigned char*)r->actual_records;}
  if(m==1)f->returned=0;if(m==2)o->active=1;if(m==3)f->actual_output=(char*)r->actual_records;if(m==4)f->argument_count=2;if(m==5)f->format_length=4;
  if(m==6){o->count=0;o->exhausted=1;o->first_failure=&o->exhaustion;o->exhaustion.entered=o->exhaustion.returned=1;o->exhaustion.result=-1;}
  unsigned char output_before[6];memcpy(output_before,r->actual_records,6);unsigned char before[sizeof(*f)];memcpy(before,f,sizeof(before));unsigned long count=0,extent=0;long result=final_parser_diagnostics(r,0,&count,&extent);assert(!r->count&&!r->extent&&!memcmp(before,f,sizeof(before))&&!memcmp(output_before,r->actual_records,6));
  if(!m){assert(!result&&count==10);assert(!final_parser_diagnostics(r,1,&count,&extent)&&r->count==10);assert(r->actual_records[6].body==e->output&&r->actual_records[6].length==4&&f->format_result==-77);assert(r->actual_records[5].body==(const unsigned char*)&e->argument);assert(r->actual_records[9].body==e->copy&&r->actual_records[9].length==6);unsigned char expected[8];final_u64(expected,28);assert(!memcmp(r->actual_records[9].header,expected,8));}
  else if(m==6){assert(!result&&count==4);assert(!final_parser_diagnostics(r,1,&count,&extent)&&r->count==4&&o->first_failure==&o->exhaustion);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}
