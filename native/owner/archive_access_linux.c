#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
static int archive_meaning(sl_candidate_span a,const char *b,size_t n){if(!a.bytes||a.extent!=n)return 0;for(size_t i=0;i<n;i++)if(a.bytes[i]!=(unsigned char)b[i])return 0;return 1;}
static const char *archive_permissions[SL_ARCHIVE_PURPOSE_COUNT]={0,sl_selinux_name_setattr,sl_selinux_name_ioctl,sl_selinux_name_relabelfrom,sl_selinux_name_unlink,sl_selinux_name_link,0,sl_selinux_name_setattr,sl_selinux_name_ioctl,sl_selinux_name_relabelfrom,sl_selinux_name_unlink,sl_selinux_name_link,0,sl_selinux_name_add_name,sl_selinux_name_remove_name,sl_selinux_name_rename,sl_selinux_name_reparent,sl_selinux_name_setattr,sl_selinux_name_relabelfrom,0,sl_selinux_name_setattr,sl_selinux_name_ioctl,sl_selinux_name_relabelfrom,sl_selinux_name_unlink,sl_selinux_name_link};
long sl_linux_interpret_archive_access(sl_linux_archive_access *o){
 if(!o||o->attempted)return -EINVAL;
 sl_linux_public_startup *s=o->startup_original;
 if(!s||!s->actual_parse_capacity||s->actual_parse_capacity>ULONG_MAX/SL_ARCHIVE_PURPOSE_COUNT||!s->archive_accesses||s->archive_access_capacity!=SL_ARCHIVE_PURPOSE_COUNT*s->actual_parse_capacity||o->purpose>=SL_ARCHIVE_PURPOSE_COUNT||o->policy_index>=s->actual_parse_capacity||o!=&s->archive_accesses[o->purpose*s->actual_parse_capacity+o->policy_index])return -EINVAL;
 o->attempted=1;o->primary_result=o->result=-EINVAL;o->actual_sources=o->sources;o->actual_capacity=o->capacity;
 sl_linux_selinux_context *context=o->context_original,*owner=o->owner_original;sl_linux_candidate_roles *roles=o->roles_original;sl_linux_context_reachability *payload=o->payload_original;
 if(!context||!context->archive_returned||context->archive_result||!context->returned||context->result||context->status_result||!context->actual_parse||!context->type||!owner||!owner->returned||owner->result||owner->status_result||owner->actual_parse!=context->actual_parse||!owner->type||!roles||!roles->returned||roles->result||roles->status_result||!roles->policy_original||!payload||!payload->returned||payload->result||payload->status_result||!payload->graph.returned||payload->graph.result||payload->graph.actual_policy!=roles->policy_original||!payload->graph.actual_reached||payload->graph.actual_type_capacity<roles->policy_original->p_types.nprim||&context->actual_parse->parser.policy!=roles->policy_original||!roles->policy_original->type_val_to_struct||(roles->count&&!roles->actual_bindings))goto done;
 if(!s->candidate_owner_entry_returned||s->candidate_owner_entry_result||!s->candidate_ptrace_returned||s->candidate_ptrace_result||!s->candidate_file_entry_returned||s->candidate_file_entry_result)goto done;
 if(!sl_linux_equal_selinux_status(&roles->after_roles,&context->after_context)||!sl_linux_equal_selinux_status(&roles->after_roles,&owner->after_context)||!sl_linux_equal_selinux_status(&roles->after_roles,&payload->after_graph)||o->after_access.attempted||o->after_access.status_original||o->inventory.custody.policy_original||o->inventory.custody.target_type)goto done;
 o->after_access.status_original=roles->after_roles.status_original;
 o->inventory.custody.policy_original=roles->policy_original;if(o->purpose<SL_ARCHIVE_CREATION_PURPOSE_FIRST)o->inventory.custody.target_type=context->type;
 o->class_original=o->purpose<12||o->purpose>=SL_ARCHIVE_CREATION_PURPOSE_FIRST?sl_selinux_name_file:sl_selinux_name_dir;o->permission_original=archive_permissions[o->purpose];
 if(o->purpose>=SL_ARCHIVE_CREATION_PURPOSE_FIRST){
  sl_linux_archive_creation *creation=o->creation_original;
  if(!creation||!s->archive_creations||creation!=&s->archive_creations[o->policy_index]||!creation->returned||creation->result||creation->status_result||creation->directory_original!=context||creation->owner_original!=owner||!sl_linux_equal_selinux_status(&creation->after_creation,&context->after_context))goto sampled;
  o->inventory_result=sl_selinux_interpret_creation_permission_sources(&o->inventory,&creation->types,o->permission_original?o->permission_original:sl_selinux_name_write,o->permission_original?NULL:sl_selinux_name_append);
 }
 else if(o->permission_original)o->inventory_result=sl_selinux_interpret_permission_sources(&o->inventory,o->class_original,o->permission_original);
 else o->inventory_result=sl_selinux_interpret_permission_pair_sources(&o->inventory,o->class_original,sl_selinux_name_write,sl_selinux_name_append);
 if(o->inventory_result){o->primary_result=o->inventory_result;goto sampled;}
 if(!o->inventory.returned||o->inventory.result||o->inventory.source_count>o->actual_capacity||(o->actual_capacity&&!o->actual_sources)||(o->inventory.source_count&&!o->inventory.actual_sources))goto sampled;
 for(unsigned long i=0;i<o->actual_capacity;i++)if(o->actual_sources[i].relation.attempted)goto sampled;
 for(unsigned long i=0;i<o->inventory.source_count;i++){
  sl_linux_archive_source *record=&o->actual_sources[o->count++];sl_candidate_route_source *r=&record->relation;r->attempted=1;r->result=-EINVAL;r->source_original=&o->inventory.actual_sources[i];r->observed_type=r->source_original->type;
  if(!r->observed_type||r->observed_type>roles->policy_original->p_types.nprim||r->source_original->type_original!=roles->policy_original->type_val_to_struct[r->observed_type-1]){r->returned=1;goto sampled;}
  r->payload_reachable=payload->graph.actual_reached[r->observed_type-1];
  if(r->source_original->possible){
   if(r->payload_reachable){r->returned=1;goto sampled;}
   if(r->observed_type==owner->type){record->conditional_closed_owner=1;record->owner_original=owner;}
   else{
    for(size_t j=0;j<roles->count;j++){sl_candidate_role_binding *b=&roles->actual_bindings[j];if(!b->returned||b->result||b->type_original!=r->source_original->type_original)continue;
     if(!archive_meaning(b->actor_meaning_original,"service-lasso.maintenance.whole-type.v1",sizeof("service-lasso.maintenance.whole-type.v1")-1)||!archive_meaning(b->episode_meaning_original,"service-lasso.maintenance.enforcement-until-disposition.v1",sizeof("service-lasso.maintenance.enforcement-until-disposition.v1")-1)){r->unsupported_scope_roles++;continue;}
     if(!r->matching_roles)r->first_role_original=b;r->matching_roles++;r->role_original=b;r->evidence_original=b->evidence_original;r->statement_original=b->statement_original;
    }
    if(!r->matching_roles){r->returned=1;goto sampled;}
   }
  }
  r->result=0;r->returned=1;
 }
 o->primary_result=0;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_access);if(!o->status_result&&!sl_linux_equal_selinux_status(&roles->after_roles,&o->after_access))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
 /* Conditional owner classification is not full loaded-source/alias/last-use
  * correspondence and cannot authorize writer retirement, seal or reader. */
}
long sl_linux_interpret_archive_accesses(sl_linux_public_startup *s){
 if(!s||s->archive_accesses_attempted)return -EINVAL;s->archive_accesses_attempted=1;s->archive_accesses_result=-EINVAL;
 if(!s->archive_contexts_returned||s->archive_contexts_result||!s->actual_parse_capacity||s->actual_parse_capacity>ULONG_MAX/SL_ARCHIVE_PURPOSE_COUNT||!s->archive_accesses||s->archive_access_capacity!=SL_ARCHIVE_PURPOSE_COUNT*s->actual_parse_capacity||!s->actual_contexts||s->actual_context_capacity!=s->actual_parse_capacity||!s->actual_candidate_roles||s->actual_candidate_role_capacity!=s->actual_parse_capacity||!s->actual_candidate_payloads||s->actual_candidate_payload_capacity!=s->actual_parse_capacity||!s->actual_archive_contexts||s->actual_archive_context_capacity!=3*s->actual_parse_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_parse_capacity)goto done;
 for(unsigned long i=0;i<s->archive_access_capacity;i++){
  sl_linux_archive_access *o=&s->archive_accesses[i];unsigned long purpose=i/s->actual_parse_capacity,policy=i%s->actual_parse_capacity,kind=purpose<6?0:purpose<12?1:2;
  if((o->attempted&&!(purpose>=12&&policy<s->mounts.selinux_count&&s->archive_precreation_returned&&!s->archive_precreation_result&&o->returned&&!o->result))||o->startup_original!=s||o->purpose!=purpose||o->policy_index!=policy||o->context_original!=&s->actual_archive_contexts[kind*s->actual_parse_capacity+policy]||o->owner_original!=&s->actual_contexts[policy]||o->roles_original!=&s->actual_candidate_roles[policy]||o->payload_original!=&s->actual_candidate_payloads[policy].reachable)goto done;
 }
 for(unsigned long purpose=0;purpose<SL_ARCHIVE_PURPOSE_COUNT;purpose++)for(unsigned long policy=0;policy<s->mounts.selinux_count;policy++){
  unsigned long index=purpose*s->actual_parse_capacity+policy;s->archive_access_count=index+1;s->archive_accesses_result=s->archive_accesses[index].attempted?s->archive_accesses[index].result:sl_linux_interpret_archive_access(&s->archive_accesses[index]);if(s->archive_accesses_result)goto done;
 }
 s->archive_accesses_result=0;
 done:s->archive_accesses_returned=1;return s->archive_accesses_result;
}

