#include "candidate_routes_linux.h"
#include <errno.h>
static int meaning(sl_candidate_span span,const char *word,size_t extent){
 if(!span.bytes||span.extent!=extent)return 0;
 for(size_t i=0;i<extent;i++)if(span.bytes[i]!=(unsigned char)word[i])return 0;return 1;
}
long sl_linux_interpret_candidate_routes(sl_linux_candidate_routes *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->primary_result=-EINVAL;
 o->actual_roles=o->roles_original;o->actual_control=o->control_original;o->actual_payload=o->payload_original;
 o->actual_sources=o->sources;o->actual_capacity=o->capacity;
 sl_linux_candidate_roles *r=o->actual_roles;sl_linux_control_access *c=o->actual_control;sl_linux_context_reachability *g=o->actual_payload;
 if(!r||!r->returned||r->result||r->status_result||!r->policy_original||!c||!c->returned||c->result||c->status_result||!g||!g->returned||g->result||g->status_result)goto done;
 if((r->count&&!r->actual_bindings)||!r->policy_original->type_val_to_struct)goto done;
 if(!c->inventory.returned||c->inventory.result||!g->graph.returned||g->graph.result||c->inventory.custody.actual_policy!=r->policy_original||g->graph.actual_policy!=r->policy_original||!g->graph.actual_reached||g->graph.actual_type_capacity<r->policy_original->p_types.nprim)goto done;
 if(!sl_linux_equal_selinux_status(&r->after_roles,&c->after_access)||!sl_linux_equal_selinux_status(&r->after_roles,&g->after_graph)||o->after_routes.attempted||o->after_routes.status_original)goto done;
 o->after_routes.status_original=r->after_roles.status_original;
 if(c->inventory.source_count>o->actual_capacity||(o->actual_capacity&&!o->actual_sources)||(c->inventory.source_count&&!c->inventory.actual_sources))goto sampled;
 for(size_t i=0;i<o->actual_capacity;i++)if(o->actual_sources[i].attempted||o->actual_sources[i].source_original)goto sampled;
 for(size_t i=0;i<c->inventory.source_count;i++){
  sl_candidate_route_source *s=&o->actual_sources[o->count++];s->attempted=1;s->result=-EINVAL;s->source_original=&c->inventory.actual_sources[i];s->observed_type=s->source_original->type;
  if(!s->observed_type||s->observed_type>r->policy_original->p_types.nprim||!s->source_original->type_original||s->source_original->type_original!=r->policy_original->type_val_to_struct[s->observed_type-1]){s->returned=1;goto sampled;}
  s->payload_reachable=g->graph.actual_reached[s->observed_type-1];
  if(s->source_original->possible){
   if(s->payload_reachable){s->returned=1;goto sampled;}
   for(size_t j=0;j<r->count;j++){
    sl_candidate_role_binding *b=&r->actual_bindings[j];
    if(!b->returned||b->result||b->type_original!=s->source_original->type_original)continue;
    if(!meaning(b->actor_meaning_original,"service-lasso.maintenance.whole-type.v1",sizeof("service-lasso.maintenance.whole-type.v1")-1)||!meaning(b->episode_meaning_original,"service-lasso.maintenance.enforcement-until-disposition.v1",sizeof("service-lasso.maintenance.enforcement-until-disposition.v1")-1)){s->unsupported_scope_roles++;continue;}
    if(!s->matching_roles)s->first_role_original=b;s->matching_roles++;s->role_original=b;s->evidence_original=b->evidence_original;s->statement_original=b->statement_original;
   }
   if(!s->matching_roles){s->returned=1;goto sampled;}
  }
  s->result=0;s->returned=1;
 }
 o->primary_result=0;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_routes);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&r->after_roles,&o->after_routes))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}