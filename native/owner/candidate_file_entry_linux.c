#include "candidate_file_entry_linux.h"
#include <errno.h>
static uint64_t little(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(i*8);return n;}
static int meaning(sl_candidate_span s,const char *text){if(!s.bytes||!text)return 0;size_t i=0;while(text[i]){if(i==s.extent||s.bytes[i]!=(unsigned char)text[i])return 0;i++;}return i==s.extent;}
static int scope(sl_linux_candidate_file_entry *o,unsigned which,const char *name,unsigned expected){
 sl_candidate_profile *c=o->actual_policy->actual_candidate;
 for(size_t i=0;i<c->counts[2];i++){
  const sl_candidate_record *r=&c->actual_rows[2][i];if(r->field_count!=3||!meaning(r->fields[1],name))continue;
  sl_candidate_span link=r->fields[0],interpretation=r->fields[2];if(!link.bytes||link.extent!=24||!interpretation.bytes||interpretation.extent!=8)return -EINVAL;
  uint64_t index=little(link.bytes),offset=little(link.bytes+8),extent=little(link.bytes+16),type=little(interpretation.bytes);
  if(index>=c->counts[0]||!extent||type!=expected)return -EINVAL;
  const sl_candidate_record *e=&c->actual_rows[0][index];if(e->field_count!=7||!e->fields[6].bytes||offset>e->fields[6].extent||extent>e->fields[6].extent-offset)return -EINVAL;
  o->declarations[which]=r;o->evidence[which]=e;o->evidence_index[which]=index;o->statement_offset[which]=offset;o->statement_extent[which]=extent;o->declared_type[which]=type;
  o->statements[which]=(sl_candidate_span){e->fields[6].bytes+offset,(size_t)extent};o->matches[which]++;
 }
 return o->matches[which]?0:-EINVAL;
}
long sl_linux_interpret_candidate_file_entry(sl_linux_candidate_file_entry *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=o->primary_result=-EINVAL;
 o->actual_policy=o->policy_original;o->actual_owner=o->owner_original;o->actual_public=o->public_original;o->actual_file=o->file_original;o->actual_payload=o->payload_original;
 sl_linux_candidate_policy *policy=o->actual_policy;sl_linux_selinux_context *owner=o->actual_owner,*public=o->actual_public,*file=o->actual_file;sl_linux_candidate_payload_context *payload=o->actual_payload;
 if(!policy||!policy->returned||policy->result||!policy->actual_candidate||!policy->actual_parse||!owner||!public||!file||!payload||!payload->returned||payload->result||!owner->self_returned||owner->self_result||!public->exec_returned||public->exec_result||!file->exec_returned||file->exec_result||!owner->returned||owner->result||!public->returned||public->result||!file->returned||file->result||owner->actual_parse!=policy->actual_parse||public->actual_parse!=policy->actual_parse||file->actual_parse!=policy->actual_parse||payload->context.actual_parse!=policy->actual_parse)goto done;
 o->after_entry.status_original=&policy->actual_parse->actual_snapshot->status;
 if(scope(o,0,"service-lasso.owner-entrypoint-file-type.v1",public->type)||scope(o,1,"service-lasso.payload-exec-file-type.v1",file->type))goto sample;
 o->matrix.custody.policy_original=&policy->actual_parse->parser.policy;
 o->matrix_result=sl_selinux_interpret_permission_matrix(&o->matrix,sl_selinux_name_file,sl_selinux_name_entrypoint,sl_selinux_name_execute_no_trans);if(o->matrix_result){o->primary_result=o->matrix_result;goto sample;}
 unsigned long n=o->matrix.types;if(!owner->type||owner->type>n||!payload->context.type||payload->context.type>n||!public->type||public->type>n||!file->type||file->type>n)goto sample;
 for(unsigned long target=0;target<n;target++){
  sl_selinux_transition_edge *edge=&o->matrix.actual_edges[(owner->type-1)*n+target];o->examined=target+1;
  if(edge->permissive||(edge->transition&&target+1!=public->type))goto sample;
 }
 sl_selinux_transition_edge *exec=&o->matrix.actual_edges[(payload->context.type-1)*n+file->type-1];
 if(exec->permissive||!exec->dyntransition)goto sample;
 o->exec_type.custody.policy_original=&policy->actual_parse->parser.policy;o->exec_type.custody.source_type=payload->context.type;o->exec_type.custody.target_type=file->type;
 o->exec_type_result=sl_selinux_interpret_exec_type_stability(&o->exec_type);if(o->exec_type_result){o->primary_result=o->exec_type_result;goto sample;}
 if(o->exec_context.source_original!=&payload->context||o->exec_context.file_original!=file||o->exec_context.type_original!=&o->exec_type)goto sample;
 o->exec_context_result=sl_linux_interpret_exec_context_stability(&o->exec_context);if(o->exec_context_result){o->primary_result=o->exec_context_result;goto sample;}
 o->primary_result=0;
 sample:o->status_result=sl_linux_sample_selinux_status(&o->after_entry);if(!o->status_result&&!sl_linux_equal_selinux_status(&policy->actual_parse->after_parse,&o->after_entry))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
 /* Qualified whole-label scope is an initial evidence meaning, never inferred
  * from type equality. No process transition/postexec or gate grant here. */
}