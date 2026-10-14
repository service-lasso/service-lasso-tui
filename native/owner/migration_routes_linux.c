#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
static int migration_meaning(sl_candidate_span s,const char *word,size_t n){if(!s.bytes||s.extent!=n)return 0;for(size_t i=0;i<n;i++)if(s.bytes[i]!=(unsigned char)word[i])return 0;return 1;}
long sl_linux_interpret_migration_routes(sl_linux_migration_routes *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->primary_result=o->result=-EINVAL;
 sl_linux_public_startup *s=o->startup_original;sl_linux_migration_access *access=o->access_original;
 sl_linux_candidate_roles *roles=o->roles_original;sl_linux_selinux_context *owner=o->owner_original;sl_linux_context_reachability *payload=o->payload_original;
 if(!s||!s->migration_routes||o->index>=s->migration_route_capacity||o!=&s->migration_routes[o->index]||
    !access||!access->returned||access->result||access->primary_result||access->status_result||!access->inventory.returned||access->inventory.result||
    !roles||!roles->returned||roles->result||roles->status_result||!roles->policy_original||(roles->count&&!roles->actual_bindings)||
    !owner||!owner->returned||owner->result||owner->status_result||!owner->self_returned||owner->self_result||!owner->actual_self||!owner->actual_parse||
    &owner->actual_parse->parser.policy!=roles->policy_original||!owner->type||owner->type>roles->policy_original->p_types.nprim||
    !payload||!payload->returned||payload->result||payload->status_result||!payload->graph.returned||payload->graph.result||
    payload->graph.actual_policy!=roles->policy_original||!payload->graph.actual_reached||payload->graph.actual_type_capacity<roles->policy_original->p_types.nprim||
    access->inventory.custody.actual_policy!=roles->policy_original||!roles->policy_original->type_val_to_struct||
    !sl_linux_equal_selinux_status(&roles->after_roles,&access->after_access)||!sl_linux_equal_selinux_status(&roles->after_roles,&owner->after_context)||
    !sl_linux_equal_selinux_status(&roles->after_roles,&payload->after_graph)||o->after_routes.attempted||o->after_routes.status_original)goto done;
 o->after_routes.status_original=roles->after_roles.status_original;
 if(access->inventory.source_count!=roles->policy_original->p_types.nprim||access->inventory.actual_source_capacity<access->inventory.source_count||access->inventory.source_count>o->capacity||(o->capacity&&!o->sources)||(access->inventory.source_count&&!access->inventory.actual_sources)||o->count)goto sampled;
 for(unsigned long i=0;i<o->capacity;i++)if(o->sources[i].attempted||o->sources[i].source_original)goto sampled;
 for(unsigned long i=0;i<access->inventory.source_count;i++){
  sl_candidate_route_source *r=&o->sources[o->count++];r->attempted=1;r->result=-EINVAL;r->source_original=&access->inventory.actual_sources[i];r->observed_type=r->source_original->type;
  if(!r->observed_type||r->observed_type>roles->policy_original->p_types.nprim||!r->source_original->type_original||r->source_original->type_original!=roles->policy_original->type_val_to_struct[r->observed_type-1]){r->returned=1;goto sampled;}
  r->payload_reachable=payload->graph.actual_reached[r->observed_type-1];
  if(r->source_original->possible){
   if(r->payload_reachable){r->returned=1;goto sampled;}
   /* Owner-type is a distinct conditional classification. Complete native
    * admission still consumes actual closed loaded-owner entry correspondence. */
   if(r->observed_type!=owner->type){
    for(size_t j=0;j<roles->count;j++){
     sl_candidate_role_binding *b=&roles->actual_bindings[j];if(!b->returned||b->result||!b->declaration_original||!b->evidence_original||!b->statement_original.bytes||!b->statement_original.extent||b->type_original!=r->source_original->type_original)continue;
     if(!migration_meaning(b->actor_meaning_original,"service-lasso.maintenance.whole-type.v1",sizeof("service-lasso.maintenance.whole-type.v1")-1)||!migration_meaning(b->episode_meaning_original,"service-lasso.maintenance.enforcement-until-disposition.v1",sizeof("service-lasso.maintenance.enforcement-until-disposition.v1")-1)){r->unsupported_scope_roles++;continue;}
     if(!r->matching_roles)r->first_role_original=b;r->matching_roles++;r->role_original=b;r->evidence_original=b->evidence_original;r->statement_original=b->statement_original;
    }
    if(!r->matching_roles){r->returned=1;goto sampled;}
   }
  }
  r->result=0;r->returned=1;
 }
 o->primary_result=0;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_routes);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&roles->after_roles,&o->after_routes))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
long sl_linux_interpret_startup_migration_routes(sl_linux_public_startup *s){
 if(!s||s->migration_routes_attempted)return -EINVAL;s->migration_routes_attempted=1;s->migration_routes_result=-EINVAL;
 if(!s->actual_policy_capacity||s->actual_policy_capacity>ULONG_MAX/3||s->migration_route_capacity!=3*s->actual_policy_capacity||s->migration_access_capacity!=s->migration_route_capacity||
    !s->migration_routes||!s->migration_access||!s->actual_candidate_roles||s->actual_candidate_role_capacity!=s->actual_policy_capacity||
    !s->actual_contexts||s->actual_context_capacity!=s->actual_policy_capacity||!s->actual_candidate_payloads||s->actual_candidate_payload_capacity!=s->actual_policy_capacity||
    !s->mounts.selinux_count||s->mounts.selinux_count>s->actual_policy_capacity)goto done;
 for(unsigned long i=0;i<s->migration_route_capacity;i++){
  sl_linux_migration_routes *o=&s->migration_routes[i];unsigned long p=i/3;
  if(o->attempted||o->startup_original!=s||o->index!=i||o->access_original!=&s->migration_access[i]||o->roles_original!=&s->actual_candidate_roles[p]||o->owner_original!=&s->actual_contexts[p]||o->payload_original!=&s->actual_candidate_payloads[p].reachable)goto done;
 }
 for(unsigned long i=0;i<3*s->mounts.selinux_count;i++){s->migration_routes_result=sl_linux_interpret_migration_routes(&s->migration_routes[i]);if(s->migration_routes_result)goto done;}
 s->migration_routes_result=0;
 done:s->migration_routes_returned=1;return s->migration_routes_result;
}