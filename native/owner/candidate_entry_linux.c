#include "candidate_entry_linux.h"
#include <errno.h>
static int entry_meaning(sl_candidate_span s,const char *p,size_t n){if(!s.bytes||s.extent!=n)return 0;for(size_t i=0;i<n;i++)if(s.bytes[i]!=(unsigned char)p[i])return 0;return 1;}
static long interpret_entry_permission(sl_linux_candidate_ptrace *o,unsigned kind,unsigned payload_target){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=o->primary_result=-EINVAL;
 o->permission_kind=kind;o->class_original=kind==3?sl_selinux_name_file:sl_selinux_name_process;o->permission_original=kind==0?sl_selinux_name_ptrace:kind==1?sl_selinux_name_transition:kind==2?sl_selinux_name_dyntransition:sl_selinux_name_read;
 o->actual_roles=o->roles_original;o->actual_owner=o->owner_original;o->actual_payload=o->payload_original;o->actual_sources=o->sources;o->actual_capacity=o->capacity;
 sl_linux_candidate_roles *r=o->actual_roles;sl_linux_selinux_context *owner=o->actual_owner;sl_linux_context_reachability *payload=o->actual_payload;
 if(!r||!r->returned||r->result||r->status_result||!r->policy_original||(r->count&&!r->actual_bindings)||!owner||!owner->self_attempted||!owner->self_returned||owner->self_result||!owner->actual_self||!owner->returned||owner->result||owner->status_result||!owner->actual_parse||&owner->actual_parse->parser.policy!=r->policy_original||!owner->type||owner->type>r->policy_original->p_types.nprim||!payload||!payload->returned||payload->result||payload->status_result||payload->graph.actual_policy!=r->policy_original||!payload->graph.actual_reached||payload->graph.actual_type_capacity<r->policy_original->p_types.nprim)goto done;
 if(!sl_linux_equal_selinux_status(&r->after_roles,&owner->after_context)||!sl_linux_equal_selinux_status(&r->after_roles,&payload->after_graph)||o->after_ptrace.attempted||o->after_ptrace.status_original||o->inventory.attempted||o->inventory.custody.policy_original||o->inventory.custody.target_type)goto done;
 o->actual_target=payload_target?o->target_original:owner;
 sl_linux_selinux_context *target=o->actual_target;
 if(!target||!target->returned||target->result||target->status_result||target->actual_parse!=owner->actual_parse||
    !target->type||target->type>r->policy_original->p_types.nprim||
    (payload_target&&(payload->actual_context!=target||payload->graph.seed!=target->type))||
    !sl_linux_equal_selinux_status(&r->after_roles,&target->after_context))goto done;
 o->after_ptrace.status_original=r->after_roles.status_original;
 o->inventory.custody.policy_original=r->policy_original;o->inventory.custody.target_type=target->type;
 o->inventory_result=sl_selinux_interpret_permission_sources(&o->inventory,o->class_original,o->permission_original);
 if(o->inventory_result){o->primary_result=o->inventory_result;goto sampled;}
 if((o->inventory.source_count&&!o->inventory.actual_sources)||o->inventory.source_count>o->actual_capacity||(o->actual_capacity&&!o->actual_sources)||!r->policy_original->type_val_to_struct)goto sampled;
 for(size_t i=0;i<o->actual_capacity;i++)if(o->actual_sources[i].attempted||o->actual_sources[i].source_original)goto sampled;
 for(size_t i=0;i<o->inventory.source_count;i++){
  sl_candidate_route_source *s=&o->actual_sources[o->count++];s->attempted=1;s->result=-EINVAL;s->source_original=&o->inventory.actual_sources[i];s->observed_type=s->source_original->type;
  if(!s->observed_type||s->observed_type>r->policy_original->p_types.nprim||!s->source_original->type_original||s->source_original->type_original!=r->policy_original->type_val_to_struct[s->observed_type-1]){s->returned=1;goto sampled;}
  s->payload_reachable=payload->graph.actual_reached[s->observed_type-1];
  if(s->source_original->possible){
   if(s->payload_reachable){
    if(!payload_target){s->returned=1;goto sampled;}
    /* A source already in the SAME payload closure is an internal edge.
     * Do not assign it a maintenance role or use it as an outside entrant. */
    s->internal_payload_closure=1;s->result=0;s->returned=1;continue;
   }
   if(s->observed_type!=owner->type){
    for(size_t j=0;j<r->count;j++){
     sl_candidate_role_binding *b=&r->actual_bindings[j];if(!b->returned||b->result||b->type_original!=s->source_original->type_original)continue;
     if(!entry_meaning(b->actor_meaning_original,"service-lasso.maintenance.whole-type.v1",sizeof("service-lasso.maintenance.whole-type.v1")-1)||!entry_meaning(b->episode_meaning_original,"service-lasso.maintenance.enforcement-until-disposition.v1",sizeof("service-lasso.maintenance.enforcement-until-disposition.v1")-1)){s->unsupported_scope_roles++;continue;}
     if(!s->matching_roles)s->first_role_original=b;s->matching_roles++;s->role_original=b;s->evidence_original=b->evidence_original;s->statement_original=b->statement_original;
    }
    if(!s->matching_roles){s->returned=1;goto sampled;}
   }
  }
  s->result=0;s->returned=1;
 }
 o->primary_result=0;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_ptrace);if(!o->status_result&&!sl_linux_equal_selinux_status(&r->after_roles,&o->after_ptrace))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
long sl_linux_interpret_candidate_ptrace(sl_linux_candidate_ptrace *o){return interpret_entry_permission(o,0,0);}
long sl_linux_interpret_candidate_owner_transition(sl_linux_candidate_ptrace *o){return interpret_entry_permission(o,1,0);}
long sl_linux_interpret_candidate_owner_dyntransition(sl_linux_candidate_ptrace *o){return interpret_entry_permission(o,2,0);}

long sl_linux_interpret_candidate_payload_transition(sl_linux_candidate_ptrace *o){return interpret_entry_permission(o,1,1);}
long sl_linux_interpret_candidate_payload_dyntransition(sl_linux_candidate_ptrace *o){return interpret_entry_permission(o,2,1);}

long sl_linux_interpret_candidate_owner_proc_read(sl_linux_candidate_ptrace *o){return interpret_entry_permission(o,3,0);}
