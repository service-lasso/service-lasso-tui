/* UNRUN source regression. No binary parser/kernel enforcement qualification. */
#include "../sepol_diagnostic_owner.h"
#include "../sepol_source_owner.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
typedef struct fixture {sl_sepol_diagnostic_owner owner;sl_sepol_diagnostic frames[2];sl_sepol_argument args[2][12];char output[2][128];sl_sepol_memory_owner memory;sl_sepol_memory_attempt attempts[8];_Alignas(max_align_t) unsigned char bytes[1024];} fixture;
static void prepare(fixture *f,size_t output_capacity,size_t memory_capacity){
 atomic_init(&f->owner.claimed,0);atomic_init(&f->memory.claimed,0);
 f->memory.bytes=f->bytes;f->memory.capacity=memory_capacity;f->memory.attempts=f->attempts;f->memory.attempt_capacity=8;assert(!sl_sepol_memory_begin(&f->memory));
 f->owner.frames=f->frames;f->owner.capacity=2;f->owner.format_capacity=128;
 for(unsigned i=0;i<2;i++){f->frames[i].arguments=f->args[i];f->frames[i].argument_capacity=12;f->frames[i].output=f->output[i];f->frames[i].output_capacity=output_capacity;}
 assert(!sl_sepol_diagnostics_begin(&f->owner));
}
int main(void){
 static fixture cases[10];
 for(unsigned mode=0;mode<10;mode++){
  fixture *f=&cases[mode];prepare(f,mode==2?2:128,mode==8?1:1024);char name[]="original";errno=EIO;
  if(!mode){
   f->owner.frames=NULL;f->frames[0].output=NULL;f->frames[0].arguments=NULL;
   sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%s:%*.*u:%zu",name,4,2,7u,(size_t)9);
   assert(errno==EIO&&!f->frames[0].result&&f->frames[0].returned&&f->frames[0].actual_arguments==f->args[0]);
   assert(f->args[0][0].value.string==name&&f->args[0][0].referent_owner==&f->memory&&f->args[0][0].referent_extent==sizeof(name));
   name[0]='X';assert(!strcmp((char*)f->args[0][0].referent_copy,"original")&&!strcmp(f->output[0],"original:  07:9"));
   sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","literal");assert(!sl_sepol_diagnostics_finish(&f->owner));
  }else if(mode==1){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%*2$s");assert(f->frames[0].result==-1&&!f->frames[0].format_entered&&!f->frames[0].argument_count);assert(sl_sepol_diagnostics_finish(&f->owner)==-1);
  }else if(mode==2){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","long");assert(f->frames[0].format_result==4&&f->frames[0].format_returned&&f->frames[0].result==-1);assert(sl_sepol_diagnostics_finish(&f->owner)==-1);
  }else if(mode==3){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","one");sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","two");sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","three");assert(f->owner.exhausted&&f->owner.first_failure==&f->owner.exhaustion);assert(sl_sepol_diagnostics_finish(&f->owner)==-1);
  }else if(mode==4){char finite[2]={'a','b'};sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%.*s",2,finite);sl_sepol_argument *a=&f->args[0][1];assert(!f->frames[0].result&&a->referent_extent==2&&!a->referent_terminated&&!memcmp(a->referent_copy,finite,2));assert(!sl_sepol_diagnostics_finish(&f->owner));
  }else if(mode==5){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%.0s",name);assert(!f->frames[0].result&&!f->args[0][0].referent_extent&&!f->args[0][0].referent_copy&&!f->memory.attempt_count);assert(!sl_sepol_diagnostics_finish(&f->owner));
  }else if(mode==6){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%.*s",-1,name);assert(!f->frames[0].result&&!f->args[0][1].precision_present&&f->args[0][1].referent_terminated&&f->args[0][1].referent_extent==sizeof(name));assert(!sl_sepol_diagnostics_finish(&f->owner));
  }else if(mode==7){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%s",(char*)NULL);assert(!f->args[0][0].referent_original&&!f->args[0][0].referent_copy&&!f->args[0][0].referent_result);assert(!sl_sepol_diagnostics_finish(&f->owner));
  }else if(mode==8){sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%s",name);assert(f->args[0][0].referent_result==-1&&f->args[0][0].referent_extent==1&&f->frames[0].format_returned&&!strcmp(f->output[0],name)&&f->frames[0].result==-1);assert(sl_sepol_diagnostics_finish(&f->owner)==-1);
  }else{sl_sepol_diagnostic_emit(NULL,1,"libsepol","read","%ls",L"wide");assert(f->frames[0].format_returned&&f->args[0][0].referent_result==-1&&f->frames[0].result==-1);assert(sl_sepol_diagnostics_finish(&f->owner)==-1);}
  assert(!sl_sepol_memory_finish(&f->memory));
 }
 return 0;
}
