/* UNRUN actual owning call chain with controlled stage outcomes. No gate,
 * kernel, complete prefix, seal, ACK or process-exit qualification. */
#include "../public_main_linux.h"
#include <assert.h>
#include <errno.h>
#include "../outer_birth_linux.h"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <string.h>
static sl_linux_candidate_arena arenas[8];
static sl_linux_public_startup startups[8];
static sl_linux_public_controller controllers[8];
static unsigned mode,constructs,acquires,births,captures;
static sl_linux_candidate_arena *construct(void){constructs++;return &arenas[mode];}
long sl_linux_acquire_public_startup(sl_linux_public_startup *s){
 acquires++;assert(s==&startups[mode]);s->returned=1;s->stage=SL_STARTUP_ACQUIRED;return s->result=mode==2?-EIO:0;
}
long sl_linux_construct_contained_producer(sl_linux_public_controller *c){
 births++;assert(c==&controllers[mode]);c->construction_returned=mode!=5;
 c->construction_body_returned=1;c->construction_body_completed=mode!=6;c->birth_result=42;
 c->construction_result=mode==3?-EPERM:0;
 return mode==3?-EPERM:mode==7?43:42;
}
long sl_linux_capture_drained_sources(sl_linux_public_controller *c){
 captures++;assert(c==&controllers[mode]);c->normal_capture_returned=1;return c->normal_capture_result=mode==4?-ENOSPC:0;
}
long sl_linux_admit_candidate_birth(sl_linux_public_controller *c){(void)c;assert(0);return -EINVAL;}
long sl_linux_deliver_gate_token(sl_linux_gate_delivery *g,sl_linux_public_controller *c){(void)g;(void)c;assert(0);return -EINVAL;}
int main(void){
 static sl_linux_public_main_owner owners[8];
 static char zero[]="service-lasso-tui",argument[]="--help",environment[]="TEST=original";
 static char *argv[]={zero,argument,0},*env[]={environment,0};
 for(mode=0;mode<8;mode++){
  sl_linux_public_main_owner *o=&owners[mode];sl_linux_candidate_arena *a=&arenas[mode];
  a->returned=a->bound=1;a->startup=&startups[mode];a->result=mode==1?-ENOMEM:0;
  startups[mode].controller_original=&controllers[mode];controllers[mode].startup_original=&startups[mode];
  char **actual_env=mode==0?0:env;char **actual_argv=mode==0?0:argv;int actual_argc=mode==0?0:2;
  o->argc=actual_argc;o->argv=actual_argv;o->environment=actual_env;o->construct_arena_original=construct;
  unsigned ac=acquires,bi=births,ca=captures,co=constructs;
  long result=sl_linux_construct_public_main_owner(o);
  assert(o->returned&&o->arena_original==a&&o->argv==actual_argv&&o->environment==actual_env);
  assert(result==(mode==1?-ENOMEM:mode==2?-EIO:mode==3?-EPERM:mode>=5?-EINVAL:0));
  assert(acquires==ac+(mode!=1)&&births==bi+(mode==0||mode>=3)&&captures==ca);
  assert(!o->capture_returned); /* no pre-gate drain, even on positive birth */
  if(mode!=1)assert(startups[mode].argv==actual_argv&&startups[mode].environment==actual_env&&startups[mode].argc==actual_argc);
  assert(sl_linux_construct_public_main_owner(o)==-EINVAL&&o->result==result&&constructs==co+1);
 }
 /* Separate fixture-owned occurrence: actual prepared consumer must not call
  * the historical constructor or fabricate its return. The map fields below
  * are synthetic representation inputs, never real mmap/birth evidence. */
 static sl_linux_candidate_arena prepared;
 static sl_linux_public_main_owner prepared_owner;
 static sl_linux_outer_birth prepared_birth;
 prepared.returned=prepared.bound=prepared.layout_complete=1;
 prepared.public_main_owner=&prepared_owner;prepared.outer_birth=&prepared_birth;
 prepared.startup=&startups[0];prepared.layout.public_main_owner.bytes=sizeof(prepared_owner);
 prepared.mapping.source="synthetic.prepared.map";prepared.mapping.number=SYS_mmap;
 prepared.mapping.arguments[1]=sizeof(prepared_owner);
 prepared.mapping.arguments[2]=PROT_READ|PROT_WRITE;
 prepared.mapping.arguments[3]=MAP_SHARED|MAP_ANONYMOUS;
 prepared.mapping.arguments[4]=-1;
 prepared.mapping.result=(long)(uintptr_t)&prepared_owner;
 atomic_init(&prepared.mapping.entered,1);atomic_init(&prepared.mapping.returned,1);
 unsigned char original[sizeof(prepared_owner)];
 memcpy(original,&prepared_owner,sizeof original);
 prepared_birth.attempted=1;
 assert(sl_linux_attach_public_main_owner(&prepared_owner,&prepared,2,argv,env)==-EINVAL);
 assert(!memcmp(original,&prepared_owner,sizeof original));
 prepared_birth.attempted=0;
 assert(sl_linux_attach_public_main_owner(&prepared_owner,&prepared,2,argv,env)==0);
 memcpy(original,&prepared_owner,sizeof original);
 assert(sl_linux_attach_public_main_owner(&prepared_owner,&prepared,2,argv,env)==-EINVAL);
 assert(!memcmp(original,&prepared_owner,sizeof original));
 mode=0;unsigned co=constructs,ac=acquires,bi=births,ca=captures;
 assert(sl_linux_construct_public_main_owner(&prepared_owner)==0);
 assert(prepared_owner.arena_original==&prepared&&!prepared_owner.arena_returned);
 assert(prepared_owner.prepared_arena_original==&prepared&&!prepared_owner.construct_arena_original);
 assert(constructs==co&&acquires==ac+1&&births==bi+1&&captures==ca);
 assert(startups[0].argv==argv&&startups[0].environment==env);
 return 0;
}
