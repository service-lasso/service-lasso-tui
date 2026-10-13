/* UNRUN controlled native child call order, no real credential effects. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_preexec_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
static sl_linux_candidate_preexec_shared *expected;static long before_failure,after_failure,mutation_failure,exec_failure;static unsigned observations,mutations,exec_observations;
int sl_selinux_interpret_bounds(sl_selinux_bounds *b){(void)b;return -EINVAL;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
int sl_linux_prepare_selinux_self(sl_linux_lsm_self *s){(void)s;return 0;}
long sl_linux_observe_selinux_self(sl_linux_lsm_self *s){observations++;s->attempted=s->returned=1;if(s==&expected->before){s->context=expected->owner_original->actual_body;s->extent=expected->owner_original->actual_extent;return before_failure;}assert(s==&expected->after);s->context=expected->payload_original->desired_original.bytes;s->extent=expected->payload_original->desired_original.extent;return after_failure;}
long sl_linux_observe_selinux_exec(sl_linux_lsm_self *s){assert(s==&expected->pending_exec);exec_observations++;s->attempted=s->returned=s->framing_complete=1;s->observed.ctx_len=exec_failure==1?2:0;return exec_failure<0?exec_failure:0;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){assert(expected&&c==&expected->mutation&&number==460&&a==100&&b==(long)expected->input&&d==(long)expected->input_extent&&!e&&!f&&!g);mutations++;c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->result=mutation_failure;atomic_store(&c->entered,1);atomic_store(&c->returned,1);return mutation_failure;}
#include "../candidate_preexec_linux.c"
typedef struct{sl_linux_candidate_preexec_shared shared;sl_linux_bootstrap bootstrap;sl_linux_selinux_context owner;sl_linux_candidate_payload_context payload;unsigned char before[3],after[3],input[40];} fixture;
int main(void){fixture cases[8]={0};for(unsigned i=0;i<8;i++){
 fixture *f=&cases[i];expected=&f->shared;f->shared.bootstrap_original=&f->bootstrap;f->bootstrap.candidate_preexec=&f->shared;atomic_init(&f->bootstrap.authority.returned,1);atomic_init(&f->shared.returned,0);atomic_init(&f->shared.mutation.entered,0);atomic_init(&f->shared.mutation.returned,0);
 f->shared.owner_original=&f->owner;f->shared.payload_original=&f->payload;f->shared.input=f->input;f->shared.input_extent=40;f->owner.actual_body=f->before;f->owner.actual_extent=3;f->payload.desired_original=(sl_candidate_span){f->after,3};
 exec_failure=i==6?-65:i==7?1:0;before_failure=i==1?-61:0;mutation_failure=(i==2||i==3)?-62:0;after_failure=(i==3||i==4)?-63:0;if(i==5)f->bootstrap.authority.result=-64;
 unsigned ob=observations,mb=mutations,eb=exec_observations;long result=sl_linux_apply_candidate_preexec(&f->shared);assert(atomic_load(&f->shared.returned));
 if(i==5)assert(result==-EINVAL&&observations==ob&&mutations==mb);else if(i==1)assert(result==-61&&observations==ob+1&&mutations==mb);else{assert(observations==ob+2&&mutations==mb+1);assert(result==((i==2||i==3)?-62:i==4?-63:i==6?-65:i==7?-EINVAL:0));}
 assert(exec_observations==eb+((i==1||i==5)?0:1));
 if(i==3)assert(f->shared.mutation_result==-62&&f->shared.after_result==-63&&f->shared.mutation.result==-62);
 ob=observations;mb=mutations;assert(sl_linux_apply_candidate_preexec(&f->shared)==-EINVAL&&observations==ob&&mutations==mb);
}
 /* Actual preparation preflight against fixture-owned disjoint originals.
  * Valid storage reaches a later deliberate missing-self refusal; malformed
  * storage must leave the entire preparation occurrence untouched. */
 typedef struct {
  sl_linux_candidate_preexec preparation;sl_linux_candidate_payload_context payload;
  sl_linux_selinux_context owner;sl_linux_policy_parse parse;sl_linux_bootstrap bootstrap;
  sl_linux_candidate_policy policy;sl_candidate_profile profile;
  sl_linux_selinux_status status;uint32_t status_words[5];
  sl_linux_call mapping;sl_sepol_diagnostic diagnostic;unsigned char source[8];
  _Alignas(sl_linux_candidate_preexec_shared) unsigned char output[sizeof(sl_linux_candidate_preexec_shared)+128];
 } storage_fixture;
 static storage_fixture storage[9];
 for(unsigned i=0;i<9;i++){
  storage_fixture *f=&storage[i];sl_linux_candidate_preexec *o=&f->preparation;
  o->payload_original=&f->payload;o->owner_original=&f->owner;o->bootstrap_original=&f->bootstrap;
  f->payload.returned=f->payload.context.returned=f->owner.returned=f->parse.returned=f->parse.parser.returned=1;
  f->payload.context.actual_parse=f->owner.actual_parse=&f->parse;
  f->payload.actual_policy=&f->policy;f->policy.actual_parse=&f->parse;
  f->policy.actual_candidate=&f->profile;f->policy.returned=f->profile.returned=1;
  f->profile.source=f->profile.original.bytes=f->source;
  f->profile.source_extent=f->profile.original.extent=sizeof f->source;
  f->status.actual_words=f->status_words;
  sl_linux_selinux_status_sample *samples[]={&f->parse.after_parse,&f->owner.after_context,&f->payload.context.after_context,&f->payload.reachable.after_graph};
  for(unsigned j=0;j<4;j++){samples[j]->returned=1;samples[j]->actual_status=&f->status;}
  f->payload.desired_original=(sl_candidate_span){f->source,sizeof f->source};
  f->payload.context.actual_body=f->owner.actual_body=f->parse.parser.actual_body=f->source;
  f->payload.context.actual_extent=f->owner.actual_extent=f->parse.parser.actual_extent=sizeof f->source;
  f->mapping.source="synthetic.containing.map";f->mapping.number=SYS_mmap;
  f->mapping.arguments[1]=sizeof f->output;f->mapping.arguments[2]=PROT_READ|PROT_WRITE;
  f->mapping.arguments[3]=MAP_SHARED|MAP_ANONYMOUS;f->mapping.arguments[4]=-1;
  f->mapping.result=(long)(uintptr_t)f->output;
  atomic_init(&f->mapping.entered,1);atomic_init(&f->mapping.returned,1);
  o->containing_range=(sl_linux_shared_range){&f->mapping,0,sizeof f->output};
  if(i==1){f->parse.parser.memory.actual_bytes=f->output;f->parse.parser.memory.actual_capacity=1;}
  if(i==2){f->owner.diagnostics.actual_frames=&f->diagnostic;f->owner.diagnostics.actual_capacity=1;f->diagnostic.actual_output=(char*)f->output;f->diagnostic.actual_output_capacity=1;}
  if(i==3){o->bounds.visited=(type_datum_t**)f->output;o->bounds.capacity=1;}
  if(i==4){f->payload.reachable.graph.matrix.actual_edges=(sl_selinux_transition_edge*)f->output;f->payload.reachable.graph.matrix.actual_edge_capacity=1;}
  if(i==5){f->parse.parser.actual_body=f->output;f->parse.parser.actual_extent=1;}
  if(i==6){f->owner.memory.actual_attempts=(sl_sepol_memory_attempt*)f->source;f->owner.memory.actual_attempt_capacity=SIZE_MAX;}
  if(i==7){f->profile.source=f->output;f->profile.source_extent=1;}
  if(i==8){f->status.actual_words=(volatile uint32_t*)f->output;}
  unsigned char original[sizeof(*o)],backing[sizeof f->output];
  memcpy(original,o,sizeof original);memcpy(backing,f->output,sizeof backing);
  assert(sl_linux_prepare_candidate_preexec(o)==-EINVAL);
  if(i)assert(!memcmp(original,o,sizeof original));else assert(o->attempted&&o->returned);
  assert(!memcmp(backing,f->output,sizeof backing));
 }
 return 0;}
