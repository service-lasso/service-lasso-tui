#include "candidate_socket_context_linux.h"
#include <errno.h>
static uint64_t socket_le64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static int socket_meaning(sl_candidate_span s){static const unsigned char word[]="service-lasso.owner-endpoint-context.v1";if(!s.bytes||s.extent!=sizeof word-1)return 0;for(size_t i=0;i<s.extent;i++)if(s.bytes[i]!=word[i])return 0;return 1;}
long sl_linux_interpret_candidate_socket_context(sl_linux_candidate_socket_context *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->actual_policy=o->policy_original;o->actual_peer=o->peer_original;
 sl_linux_candidate_policy *p=o->actual_policy;sl_linux_socket_peer *peer=o->actual_peer;
 if(!p||!p->returned||p->result||p->status_result||!p->actual_candidate||!p->actual_parse||!peer||!peer->returned||peer->result||!peer->framing_complete||!peer->actual_bytes||!peer->extent||peer->extent>peer->security_length||peer->security_length>peer->actual_capacity||o->context.attempted||o->context.parse_original||o->context.body)goto done;
 sl_candidate_profile *c=p->actual_candidate;if(!c->returned||c->result||(c->counts[2]&&!c->actual_rows[2]))goto done;
 for(size_t i=0;i<c->counts[2];i++){
  const sl_candidate_record *row=&c->actual_rows[2][i];o->declarations_examined=i+1;if(row->field_count!=3||!socket_meaning(row->fields[1]))continue;
  sl_candidate_span proposed=row->fields[2],link=row->fields[0];o->matches++;
  if(!proposed.bytes||!proposed.extent||!link.bytes||link.extent!=24)goto done;
  if(o->declaration_original){if(proposed.extent!=o->desired_original.extent)goto done;for(size_t j=0;j<proposed.extent;j++)if(proposed.bytes[j]!=o->desired_original.bytes[j])goto done;}else{o->declaration_original=row;o->desired_original=proposed;}
  o->evidence_index=socket_le64(link.bytes);o->statement_offset=socket_le64(link.bytes+8);o->statement_extent=socket_le64(link.bytes+16);
  if(o->evidence_index>=c->counts[0]||!c->actual_rows[0]||!o->statement_extent)goto done;o->evidence_original=&c->actual_rows[0][(size_t)o->evidence_index];if(o->evidence_original->field_count!=7)goto done;
  sl_candidate_span body=o->evidence_original->fields[6];if(!body.bytes||o->statement_offset>body.extent||o->statement_extent>body.extent-o->statement_offset)goto done;o->statement_original=(sl_candidate_span){body.bytes+(size_t)o->statement_offset,(size_t)o->statement_extent};
  if(proposed.extent!=peer->extent)goto done;for(size_t j=0;j<proposed.extent;j++){o->bytes_compared++;if(proposed.bytes[j]!=peer->actual_bytes[j])goto done;}
 }
 if(!o->declaration_original)goto done;
 o->context.parse_original=p->actual_parse;o->context.body=peer->actual_bytes;o->context.extent=peer->extent;
 o->context_result=sl_linux_interpret_selinux_context(&o->context);o->result=o->context_result;
 done:o->returned=1;return o->result;
 /* Actual socket-context datum only. Creation/relabel/supplier/actor and
  * initial loaded-code/OS premises remain separate required consumers. */
}