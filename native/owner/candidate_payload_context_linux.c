#include "candidate_payload_context_linux.h"
#include <errno.h>
static uint64_t payload_le64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static int payload_meaning(sl_candidate_span s){static const unsigned char word[]="service-lasso.payload-context.v1";if(!s.bytes||s.extent!=sizeof(word)-1)return 0;for(size_t i=0;i<s.extent;i++)if(s.bytes[i]!=word[i])return 0;return 1;}
long sl_linux_interpret_candidate_payload_context(sl_linux_candidate_payload_context *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->actual_policy=o->policy_original;
 sl_linux_candidate_policy *p=o->actual_policy;
 if(!p||!p->returned||p->result||p->status_result||!p->actual_candidate||!p->actual_parse||o->context.attempted||o->context.parse_original||o->context.body||o->reachable.attempted||o->reachable.context_original)goto done;
 sl_candidate_profile *c=p->actual_candidate;
 if(!c->returned||c->result||(c->counts[2]&&!c->actual_rows[2]))goto done;
 for(size_t i=0;i<c->counts[2];i++){
  const sl_candidate_record *row=&c->actual_rows[2][i];o->declarations_examined=i+1;
  if(row->field_count!=3||!payload_meaning(row->fields[1]))continue;
  sl_candidate_span proposed=row->fields[2];
  if(o->declaration_original){
   if(!proposed.bytes||proposed.extent!=o->desired_original.extent)goto done;
   for(size_t j=0;j<proposed.extent;j++)if(proposed.bytes[j]!=o->desired_original.bytes[j])goto done;
  }else{o->declaration_original=row;o->desired_original=proposed;}
  sl_candidate_span link=row->fields[0];
  if(!link.bytes||link.extent!=24||!o->desired_original.bytes||!o->desired_original.extent)goto done;
  o->evidence_index=payload_le64(link.bytes);o->statement_offset=payload_le64(link.bytes+8);o->statement_extent=payload_le64(link.bytes+16);
  if(o->evidence_index>=c->counts[0]||!c->actual_rows[0]||!o->statement_extent)goto done;
  o->evidence_original=&c->actual_rows[0][(size_t)o->evidence_index];sl_candidate_span body=o->evidence_original->fields[6];
  if(!body.bytes||o->statement_offset>body.extent||o->statement_extent>body.extent-o->statement_offset)goto done;
  o->statement_original=(sl_candidate_span){body.bytes+(size_t)o->statement_offset,(size_t)o->statement_extent};
 }
 if(!o->declaration_original)goto done;
 o->context.parse_original=p->actual_parse;o->context.body=o->desired_original.bytes;o->context.extent=o->desired_original.extent;
 o->context_result=sl_linux_interpret_selinux_context(&o->context);if(o->context_result){o->result=o->context_result;goto done;}
 /* Explicit desired-context graph: do not invoke the actual-self wrapper or
  * set its self_attempted/actual_self fields to manufacture an observation. */
 sl_linux_context_reachability *g=&o->reachable;g->attempted=1;g->result=-EINVAL;g->context_original=&o->context;g->actual_context=&o->context;
 g->graph.policy_original=&p->actual_parse->parser.policy;g->graph.seed=o->context.type;g->after_graph.status_original=p->after_binding.status_original;
 o->graph_result=g->graph_result=sl_selinux_interpret_reachability(&g->graph);
 o->status_result=g->status_result=sl_linux_sample_selinux_status(&g->after_graph);
 if(!g->status_result&&!sl_linux_equal_selinux_status(&o->context.after_context,&g->after_graph))o->status_result=g->status_result=-EINVAL;
 g->result=g->graph_result?g->graph_result:g->status_result;g->returned=1;o->result=g->result;
 done:o->returned=1;return o->result;
}