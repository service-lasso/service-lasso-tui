#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "candidate_preexec_linux.h"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <limits.h>
#include <errno.h>
static int preexec_equal(const unsigned char *a,size_t an,const unsigned char *b,size_t bn){if(!a||!b||an!=bn)return 0;for(size_t i=0;i<an;i++)if(a[i]!=b[i])return 0;return 1;}
static int preexec_overlap(uintptr_t first,uintptr_t last,const void *p,size_t n){
 uintptr_t at=(uintptr_t)p;return !p||n>UINTPTR_MAX-at||(at<last&&first<at+n);
}
static int preexec_outside(uintptr_t first,uintptr_t last,const void *p,size_t count,size_t width){
 if(!width||count>SIZE_MAX/width)return 0;
 return !count||!preexec_overlap(first,last,p,count*width);
}
/* Completed source episodes retain their allocation arenas and every reserved
 * diagnostic output, including unused rows. Policy pointers allocated by the
 * redirected parser remain borrowers of its SAME memory arena. This checks
 * storage exclusion only; it does not authenticate any of those pointers. */
static int preexec_episode_storage_outside(uintptr_t first,uintptr_t last,
 const sl_sepol_memory_owner *m,const sl_sepol_diagnostic_owner *d){
 if(!m||!d||
    !preexec_outside(first,last,m->bytes,m->capacity,1)||
    !preexec_outside(first,last,m->attempts,m->attempt_capacity,sizeof(*m->attempts))||
    !preexec_outside(first,last,d->frames,d->capacity,sizeof(*d->frames))||
    !preexec_outside(first,last,m->actual_bytes,m->actual_capacity,1)||
    !preexec_outside(first,last,m->actual_attempts,m->actual_attempt_capacity,sizeof(*m->actual_attempts))||
    !preexec_outside(first,last,d->actual_frames,d->actual_capacity,sizeof(*d->actual_frames)))return 0;
 for(size_t i=0;i<d->capacity;i++){
  const sl_sepol_diagnostic *f=&d->frames[i];
  if(!preexec_outside(first,last,f->arguments,f->argument_capacity,sizeof(*f->arguments))||
     !preexec_outside(first,last,f->output,f->output_capacity,1))return 0;
 }
 for(size_t i=0;i<d->actual_capacity;i++){
  const sl_sepol_diagnostic *f=&d->actual_frames[i];
  if(!preexec_outside(first,last,f->actual_arguments,f->actual_argument_capacity,sizeof(*f->actual_arguments))||
     !preexec_outside(first,last,f->actual_output,f->actual_output_capacity,1))return 0;
 }
 return 1;
}
static int preexec_episode_outside(uintptr_t first,uintptr_t last,
 const sl_sepol_memory_owner *m,const sl_sepol_diagnostic_owner *d){
 return m&&d&&preexec_outside(first,last,m,1,sizeof(*m))&&
  preexec_outside(first,last,d,1,sizeof(*d))&&preexec_episode_storage_outside(first,last,m,d);
}
static int preexec_graph_outside(uintptr_t first,uintptr_t last,const sl_selinux_reachability *g){
 const sl_selinux_transition_matrix *m=&g->matrix;
 const sl_selinux_te_union *u=&m->custody;
 return preexec_outside(first,last,g->reached,g->type_capacity,sizeof(*g->reached))&&
  preexec_outside(first,last,g->actual_reached,g->actual_type_capacity,sizeof(*g->actual_reached))&&
  preexec_outside(first,last,g->steps,g->step_capacity,sizeof(*g->steps))&&
  preexec_outside(first,last,g->actual_steps,g->actual_step_capacity,sizeof(*g->actual_steps))&&
  preexec_outside(first,last,m->edges,m->edge_capacity,sizeof(*m->edges))&&
  preexec_outside(first,last,m->actual_edges,m->actual_edge_capacity,sizeof(*m->actual_edges))&&
  preexec_outside(first,last,m->matches,m->match_capacity,sizeof(*m->matches))&&
  preexec_outside(first,last,m->actual_matches,m->actual_match_capacity,sizeof(*m->actual_matches))&&
  preexec_outside(first,last,u->visits,u->capacity,sizeof(*u->visits))&&
  preexec_outside(first,last,u->actual_visits,u->actual_capacity,sizeof(*u->actual_visits))&&
  preexec_outside(first,last,u->helpers,u->helper_capacity,sizeof(*u->helpers))&&
  preexec_outside(first,last,u->actual_helpers,u->actual_helper_capacity,sizeof(*u->actual_helpers));
}
static int preexec_profile_outside(uintptr_t first,uintptr_t last,const sl_candidate_profile *c){
 if(!c||!c->returned||c->result||!preexec_outside(first,last,c,1,sizeof(*c))||
    !preexec_outside(first,last,c->source,c->source_extent,1)||
    !preexec_outside(first,last,c->original.bytes,c->original.extent,1))return 0;
 for(unsigned i=0;i<5;i++)if(
    !preexec_outside(first,last,c->rows[i],c->capacities[i],sizeof(*c->rows[i]))||
    !preexec_outside(first,last,c->actual_rows[i],c->actual_capacities[i],sizeof(*c->actual_rows[i])))return 0;
 return 1;
}
static int preexec_status_outside(uintptr_t first,uintptr_t last,const sl_linux_selinux_status_sample *sample){
 if(!sample||!sample->returned||sample->result||!sample->actual_status)return 0;
 const sl_linux_selinux_status *s=sample->actual_status;
 return preexec_outside(first,last,s,1,sizeof(*s))&&
  preexec_outside(first,last,(const void *)(uintptr_t)s->actual_words,5,sizeof(uint32_t));
 /* Exclusion reads no kernel words. Their actual sequence/currentness/native
  * mapping and writer lifetime remain the existing status consumers' work. */
}
static int preexec_inputs_outside(uintptr_t first,uintptr_t last,
 const sl_linux_candidate_payload_context *p,const sl_linux_selinux_context *owner){
 if(!p||!owner||!p->returned||p->result||!p->context.returned||p->context.result||
    !owner->returned||owner->result||!owner->actual_parse||
    owner->actual_parse!=p->context.actual_parse)return 0;
 const sl_linux_policy_parse *parse=owner->actual_parse;
 const sl_linux_candidate_policy *policy=p->actual_policy;
 if(!parse->returned||parse->result||!parse->parser.returned||parse->parser.result||
    !policy||!policy->returned||policy->result||policy->actual_parse!=parse||
    !preexec_outside(first,last,policy,1,sizeof(*policy))||
    !preexec_profile_outside(first,last,policy->actual_candidate)||
    !preexec_outside(first,last,p,1,sizeof(*p))||
    !preexec_outside(first,last,owner,1,sizeof(*owner))||
    !preexec_outside(first,last,parse,1,sizeof(*parse))||
    !preexec_outside(first,last,parse->parser.actual_body,parse->parser.actual_extent,1)||
    !preexec_outside(first,last,owner->actual_body,owner->actual_extent,1)||
    !preexec_outside(first,last,p->context.actual_body,p->context.actual_extent,1)||
    !preexec_outside(first,last,p->desired_original.bytes,p->desired_original.extent,1)||
    !preexec_episode_outside(first,last,&parse->parser.memory,&parse->parser.diagnostics)||
    !preexec_episode_outside(first,last,&owner->memory,&owner->diagnostics)||
    !preexec_episode_outside(first,last,&p->context.memory,&p->context.diagnostics)||
    !preexec_graph_outside(first,last,&p->reachable.graph)||
    !preexec_status_outside(first,last,&parse->after_parse)||
    !preexec_status_outside(first,last,&owner->after_context)||
    !preexec_status_outside(first,last,&p->context.after_context)||
    !preexec_status_outside(first,last,&p->reachable.after_graph))return 0;
 return 1;
}
long sl_linux_prepare_candidate_preexec(sl_linux_candidate_preexec *o){
 if(!o||o->attempted)return -EINVAL;
 /* Reject aliased containing metadata before writing the occurrence itself.
  * The full actual extent is rechecked after its real child inputs exist. */
 if(o->containing_range.mapping_original){
  uintptr_t occurrence=(uintptr_t)o;
  if(sizeof(*o)>UINTPTR_MAX-occurrence||preexec_overlap(occurrence,occurrence+sizeof(*o),o->containing_range.mapping_original,sizeof(sl_linux_call)))return -EINVAL;
  void *backing=sl_linux_shared_range_address(&o->containing_range,1,_Alignof(sl_linux_candidate_preexec_shared));
  if(!backing)return -EINVAL;
  uintptr_t first=(uintptr_t)backing,last=first+o->containing_range.bytes;
  if(!preexec_inputs_outside(first,last,o->payload_original,o->owner_original)||
     !preexec_inputs_outside(occurrence,occurrence+sizeof(*o),o->payload_original,o->owner_original))return -EINVAL;
  if(!preexec_outside(first,last,o->bounds.visited,o->bounds.capacity,sizeof(*o->bounds.visited))||
     !preexec_outside(occurrence,occurrence+sizeof(*o),o->bounds.visited,o->bounds.capacity,sizeof(*o->bounds.visited))||
     !preexec_episode_outside(first,last,&o->child_context.memory,&o->child_context.diagnostics)||
     !preexec_episode_storage_outside(occurrence,occurrence+sizeof(*o),&o->child_context.memory,&o->child_context.diagnostics))return -EINVAL;
  if(preexec_overlap(first,last,o,sizeof(*o))||
     preexec_overlap(first,last,o->payload_original,sizeof(*o->payload_original))||
     preexec_overlap(first,last,o->owner_original,sizeof(*o->owner_original))||
     preexec_overlap(first,last,o->bootstrap_original,sizeof(*o->bootstrap_original))||
     preexec_overlap(first,last,o->containing_range.mapping_original,sizeof(sl_linux_call)))return -EINVAL;
 }
 o->attempted=1;o->result=-EINVAL;
 o->actual_payload=o->payload_original;o->actual_owner=o->owner_original;o->actual_bootstrap=o->bootstrap_original;o->actual_query_capacity=o->query_capacity;
 sl_linux_candidate_payload_context *p=o->actual_payload;sl_linux_selinux_context *owner=o->actual_owner;
 if(!p||!p->returned||p->result||!p->context.returned||p->context.result||!p->context.actual_parse||!owner||!owner->self_attempted||!owner->self_returned||owner->self_result||!owner->actual_self||!owner->returned||owner->result||!owner->actual_parse||owner->actual_parse!=p->context.actual_parse||!o->actual_bootstrap||o->shared||o->bounds.attempted||o->bounds.policy_original||o->bounds.old_type||o->bounds.new_type||o->actual_query_capacity<sizeof(sl_linux_lsm_header)+2||o->actual_query_capacity>UINT32_MAX)goto done;
 if(!p->desired_original.bytes||!p->desired_original.extent||p->desired_original.bytes!=p->context.actual_body||p->desired_original.extent!=p->context.actual_extent)goto done;
 if(!sl_linux_equal_selinux_status(&owner->after_context,&p->context.after_context))goto done;
 o->bounds.policy_original=&owner->actual_parse->parser.policy;o->bounds.old_type=owner->type;o->bounds.new_type=p->context.type;
 o->bounds_result=sl_selinux_interpret_bounds(&o->bounds);if(o->bounds_result){o->result=o->bounds_result;goto done;}
 if(p->desired_original.extent>ULONG_MAX-sizeof(sl_linux_lsm_header)-8)goto done;
 unsigned long input=(sizeof(sl_linux_lsm_header)+(unsigned long)p->desired_original.extent+1+7)&~7UL;
 o->query_stride=(o->actual_query_capacity+7)&~7UL;
 if(input>UINT32_MAX||o->query_stride>(ULONG_MAX-sizeof(sl_linux_candidate_preexec_shared)-input)/3)goto done;
 o->mapping_extent=sizeof(sl_linux_candidate_preexec_shared)+3*o->query_stride+input;
 if(o->containing_range.mapping_original){
  /* The actual single prebirth acquisition remains the origin. Do not write
   * mapping/raw_mapping as though a nested mmap had occurred. */
  if(o->mapping.source||o->mapping.number||o->raw_mapping||atomic_load(&o->mapping.entered)||atomic_load(&o->mapping.returned))goto done;
  void *backing=sl_linux_shared_range_address(&o->containing_range,o->mapping_extent,_Alignof(sl_linux_candidate_preexec_shared));
  if(!backing)goto done;
  uintptr_t first=(uintptr_t)backing,last=first+o->containing_range.bytes;
  if(preexec_overlap(first,last,o,sizeof(*o))||preexec_overlap(first,last,p,sizeof(*p))||
     preexec_overlap(first,last,owner,sizeof(*owner))||preexec_overlap(first,last,o->actual_bootstrap,sizeof(*o->actual_bootstrap))||
     preexec_overlap(first,last,o->containing_range.mapping_original,sizeof(sl_linux_call)))goto done;
  uintptr_t desired=(uintptr_t)p->desired_original.bytes;
  if(p->desired_original.extent>UINTPTR_MAX-desired||
     (desired<last&&first<desired+p->desired_original.extent))goto done;
  /* Once-only binding requires untouched output backing; examine before any
   * shared child-output publication or native query initialization. */
  const unsigned char *fresh=backing;
  for(unsigned long i=0;i<o->containing_range.bytes;i++)if(fresh[i])goto done;
  o->shared=backing;
 }else{
  atomic_init(&o->mapping.entered,0);atomic_init(&o->mapping.returned,0);
  o->raw_mapping=sl_linux_call6(&o->mapping,"candidate.preexec.shared.mmap",SYS_mmap,0,o->mapping_extent,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
  if(o->raw_mapping<0){o->result=o->raw_mapping;goto done;}if(!o->raw_mapping)goto done;
  o->shared=(sl_linux_candidate_preexec_shared *)o->raw_mapping;
 }
 sl_linux_candidate_preexec_shared *s=o->shared;s->bootstrap_original=o->actual_bootstrap;s->owner_original=owner;s->payload_original=p;
 s->before.bytes=(unsigned char *)(s+1);s->before.capacity=o->actual_query_capacity;s->after.bytes=s->before.bytes+o->query_stride;s->after.capacity=o->actual_query_capacity;
 s->pending_exec.bytes=s->after.bytes+o->query_stride;s->pending_exec.capacity=o->actual_query_capacity;
 s->input=s->pending_exec.bytes+o->query_stride;s->input_extent=input;
 s->input_header=(sl_linux_lsm_header){101,0,input,p->desired_original.extent+1};*(sl_linux_lsm_header *)s->input=s->input_header;
 for(size_t i=0;i<p->desired_original.extent;i++)s->input[sizeof(sl_linux_lsm_header)+i]=p->desired_original.bytes[i];
 if(sl_linux_prepare_selinux_self(&s->before)||sl_linux_prepare_selinux_self(&s->after)||sl_linux_prepare_selinux_self(&s->pending_exec))goto done;
 atomic_init(&s->mutation.entered,0);atomic_init(&s->mutation.returned,0);atomic_init(&s->returned,0);o->result=0;
 done:o->returned=1;return o->result;
}
long sl_linux_apply_candidate_preexec(sl_linux_candidate_preexec_shared *s){
 if(!s||s->attempted)return -EINVAL;s->attempted=1;s->result=-EINVAL;
 if(!s->bootstrap_original||s->bootstrap_original->candidate_preexec!=s||!atomic_load_explicit(&s->bootstrap_original->authority.returned,memory_order_acquire)||s->bootstrap_original->authority.result||!s->owner_original||!s->payload_original||!s->input||!s->input_extent)goto done;
 s->before_result=sl_linux_observe_selinux_self(&s->before);if(s->before_result){s->result=s->before_result;goto done;}
 if(!preexec_equal(s->before.context,s->before.extent,s->owner_original->actual_body,s->owner_original->actual_extent))goto done;
 s->mutation_result=sl_linux_call6(&s->mutation,"bootstrap.selinux.bounded.current",460,100,(long)s->input,s->input_extent,0,0,0);
 /* After observation is independent even if the mutation returned failure. */
 s->after_result=sl_linux_observe_selinux_self(&s->after);
 s->exec_result=sl_linux_observe_selinux_exec(&s->pending_exec);
 if(s->mutation_result){s->result=s->mutation_result;goto done;}
 if(s->after_result){s->result=s->after_result;goto done;}
 if(s->exec_result){s->result=s->exec_result;goto done;}
 if(s->pending_exec.observed.ctx_len||s->pending_exec.context||s->pending_exec.extent)goto done;
 if(!preexec_equal(s->after.context,s->after.extent,s->payload_original->desired_original.bytes,s->payload_original->desired_original.extent))goto done;
 s->result=0;
 done:atomic_store_explicit(&s->returned,1,memory_order_release);return s->result;
}
#endif
