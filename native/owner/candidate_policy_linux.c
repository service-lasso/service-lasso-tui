#include "candidate_policy_linux.h"
#include <errno.h>
#include <limits.h>
static long roles_finish(sl_linux_candidate_roles *o,long result){o->result=result;o->returned=1;return result;}
long sl_linux_bind_candidate_roles(sl_linux_candidate_roles *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->failure=SL_CANDIDATE_ROLE_ROOT;
 o->actual_candidate=o->candidate_original;o->actual_parse=o->parse_original;o->actual_bindings=o->bindings;o->actual_capacity=o->capacity;
 sl_candidate_profile *candidate=o->actual_candidate;sl_linux_policy_parse *parse=o->actual_parse;
 if(!candidate||!candidate->attempted||!candidate->returned||candidate->result||!candidate->original.bytes||!parse)return roles_finish(o,-EINVAL);
 o->failure=SL_CANDIDATE_ROLE_POLICY;
 if(!parse->attempted||!parse->returned||parse->result||parse->status_result||!parse->actual_snapshot||!parse->parser.returned||parse->parser.result||!parse->parser.memory.prepared||!parse->parser.memory.begin_returned||parse->parser.memory.begin_result||parse->parser.memory.failed||parse->parser.memory.first_failure||!parse->parser.memory.finished||parse->parser.memory.active)return roles_finish(o,-EINVAL);
 policydb_t *p=&parse->parser.policy;o->policy_original=p;
 if(p->sl_source_owner!=&parse->parser.memory||p->policy_type!=POLICY_KERN||p->target_platform!=SEPOL_TARGET_SELINUX||!parse->actual_snapshot->returned||parse->actual_snapshot->result||!parse->actual_snapshot->eof||parse->parser.actual_body!=parse->actual_snapshot->actual_body||parse->parser.actual_extent!=parse->actual_snapshot->length)return roles_finish(o,-EINVAL);
 if(!sl_linux_equal_selinux_status(&parse->actual_snapshot->after_policy,&parse->after_parse))return roles_finish(o,-EINVAL);
 if(o->after_roles.attempted||o->after_roles.status_original)return roles_finish(o,-EINVAL);
 o->after_roles.status_original=&parse->actual_snapshot->status;
 long result=-EINVAL;o->failure=SL_CANDIDATE_ROLE_CAPACITY;
 if(candidate->counts[1]>o->actual_capacity||(o->actual_capacity&&!o->actual_bindings)||o->count)goto sampled;
 for(size_t i=0;i<o->actual_capacity;i++)if(o->actual_bindings[i].attempted||o->actual_bindings[i].declaration_original)goto sampled;
 if(candidate->counts[1]&&!candidate->actual_rows[1])goto sampled;
 for(size_t i=0;i<candidate->counts[1];i++){
  sl_candidate_role_binding *b=&o->actual_bindings[o->count++];b->attempted=1;
  const sl_candidate_record *row=&candidate->actual_rows[1][i];b->declaration_original=row;o->failed_declaration=row;o->failure=SL_CANDIDATE_ROLE_STATEMENT;
  b->evidence_original=row->evidence_original;b->statement_original=row->statement_original;
  b->actor_meaning_original=row->fields[6];b->episode_meaning_original=row->fields[7];
  b->user_value=row->policy_user;b->role_value=row->policy_role;b->type_value=row->policy_type;
  if(row->field_count!=8||row->evidence_index>=candidate->counts[0]||!candidate->actual_rows[0]||row->evidence_original!=&candidate->actual_rows[0][(size_t)row->evidence_index]||!b->statement_original.extent||!b->actor_meaning_original.extent||!b->episode_meaning_original.extent){b->result=result;b->returned=1;goto sampled;}
  sl_candidate_span body=b->evidence_original->fields[6];
  if(row->statement_offset>body.extent||row->statement_extent>body.extent-row->statement_offset||b->statement_original.bytes!=body.bytes+(size_t)row->statement_offset||b->statement_original.extent!=row->statement_extent){b->result=result;b->returned=1;goto sampled;}
  o->failure=SL_CANDIDATE_ROLE_DATUM;
  if(!b->user_value||b->user_value>p->p_users.nprim||b->user_value>UINT_MAX||!b->role_value||b->role_value>p->p_roles.nprim||b->role_value>UINT_MAX||!b->type_value||b->type_value>p->p_types.nprim||b->type_value>UINT_MAX||!p->user_val_to_struct||!p->role_val_to_struct||!p->type_val_to_struct){b->result=result;b->returned=1;goto sampled;}
  b->user_original=p->user_val_to_struct[(unsigned)b->user_value-1];b->role_original=p->role_val_to_struct[(unsigned)b->role_value-1];b->type_original=p->type_val_to_struct[(unsigned)b->type_value-1];
  if(!b->user_original||!b->role_original||!b->type_original){b->result=result;b->returned=1;goto sampled;}
  b->observed_user=b->user_original->s.value;b->observed_role=b->role_original->s.value;b->observed_type=b->type_original->s.value;b->observed_role_flavor=b->role_original->flavor;b->observed_type_flavor=b->type_original->flavor;
  if(b->observed_user!=b->user_value||b->observed_role!=b->role_value||b->observed_type!=b->type_value||b->observed_role_flavor!=ROLE_ROLE||b->observed_type_flavor!=TYPE_TYPE){b->result=result;b->returned=1;goto sampled;}
  b->result=0;b->returned=1;
 }
 result=0;o->failure=SL_CANDIDATE_ROLE_NONE;o->failed_declaration=NULL;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_roles);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&parse->after_parse,&o->after_roles))o->status_result=-EINVAL;
 if(!result&&o->status_result){result=o->status_result;o->failure=SL_CANDIDATE_ROLE_STATUS;}
 return roles_finish(o,result);
}
long sl_linux_bind_candidate_policy(sl_linux_candidate_policy *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->failure=SL_CANDIDATE_POLICY_ROOT;
 o->actual_candidate=o->candidate_original;o->actual_parse=o->parse_original;
 sl_candidate_profile *c=o->actual_candidate;sl_linux_policy_parse *p=o->actual_parse;
 if(!c||!c->attempted||!c->returned||c->result||!p||o->digest.attempted||o->digest.body||o->after_binding.attempted||o->after_binding.status_original)goto done;
 o->declared_identity=c->policy_identity;o->declared_extent=c->policy_extent;o->kernel_interpretation_original=c->kernel_interpretation;
 if(o->declared_identity.extent!=32||!o->declared_identity.bytes||o->kernel_interpretation_original.extent!=32||!o->kernel_interpretation_original.bytes)goto done;
 o->failure=SL_CANDIDATE_POLICY_PARSE;
 if(!p->attempted||!p->returned||p->result||p->status_result||!p->parser.returned||p->parser.result||!p->actual_snapshot||!p->actual_snapshot->returned||p->actual_snapshot->result||!p->actual_snapshot->eof||!p->parser.memory.finished||p->parser.memory.active||p->parser.memory.failed||p->parser.memory.first_failure||p->parser.policy.sl_source_owner!=&p->parser.memory)goto done;
 if(p->parser.actual_body!=p->actual_snapshot->actual_body||p->parser.actual_extent!=p->actual_snapshot->length||!sl_linux_equal_selinux_status(&p->actual_snapshot->after_policy,&p->after_parse))goto done;
 o->observed_extent=p->parser.actual_extent;o->digest.body=p->parser.actual_body;o->digest.extent=p->parser.actual_extent;
 o->after_binding.status_original=&p->actual_snapshot->status;o->failure=SL_CANDIDATE_POLICY_EXTENT;
 if(o->declared_extent!=o->observed_extent)goto sampled;
 o->failure=SL_CANDIDATE_POLICY_DIGEST;o->digest_result=sl_owned_sha256_compute(&o->digest);
 if(o->digest_result){o->result=o->digest_result;goto sampled;}
 o->failure=SL_CANDIDATE_POLICY_MISMATCH;
 for(size_t i=0;i<32;i++)if(o->digest.digest[i]!=o->declared_identity.bytes[i]){o->mismatch_offset=i;goto sampled;}
 o->failure=SL_CANDIDATE_POLICY_NONE;o->result=0;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_binding);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&p->after_parse,&o->after_binding))o->status_result=-EINVAL;
 if(!o->result&&o->status_result){o->result=o->status_result;o->failure=SL_CANDIDATE_POLICY_STATUS;}
 done:o->returned=1;return o->result;
}