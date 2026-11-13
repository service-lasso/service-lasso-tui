/* UNRUN controlled original/referent cases; no human or native authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_policy_linux.h"
#include <assert.h>
#include <errno.h>
static int status_calls,status_failure,status_equal=1;static sl_linux_selinux_status *expected_status;
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){assert(s->status_original==expected_status);status_calls++;s->attempted=s->returned=1;s->actual_status=s->status_original;s->result=status_failure;return status_failure;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;return b->attempted?status_equal:1;}
#include "../owned_sha256.c"
#include "../candidate_policy_linux.c"
typedef struct {
 sl_linux_candidate_roles root;sl_candidate_profile candidate;
 sl_candidate_record evidence[1],role[1];sl_candidate_role_binding bindings[1];
 sl_linux_policy_parse parse;sl_linux_selinux_snapshot snapshot;
 user_datum_t user;role_datum_t role_datum;type_datum_t type;
 user_datum_t *users[1];role_datum_t *roles[1];type_datum_t *types[1];
 unsigned char body[3],statement[4],meaning[1];
} fixture;
static void prepare(fixture *f){
 f->root.candidate_original=&f->candidate;f->root.parse_original=&f->parse;f->root.bindings=f->bindings;f->root.capacity=1;
 f->candidate.attempted=f->candidate.returned=1;f->candidate.original.bytes=f->body;f->candidate.original.extent=3;f->candidate.actual_rows[0]=f->evidence;f->candidate.counts[0]=1;f->candidate.actual_rows[1]=f->role;f->candidate.counts[1]=1;
 f->evidence[0].fields[6]=(sl_candidate_span){f->statement,4};f->role[0].field_count=8;f->role[0].evidence_original=&f->evidence[0];f->role[0].statement_offset=1;f->role[0].statement_extent=2;f->role[0].statement_original=(sl_candidate_span){f->statement+1,2};f->role[0].fields[6]=(sl_candidate_span){f->meaning,1};f->role[0].fields[7]=(sl_candidate_span){f->meaning,1};f->role[0].policy_user=f->role[0].policy_role=f->role[0].policy_type=1;
 f->parse.attempted=f->parse.returned=f->parse.parser.returned=1;f->parse.actual_snapshot=&f->snapshot;f->parse.parser.memory.prepared=f->parse.parser.memory.begin_returned=f->parse.parser.memory.finished=1;
 f->snapshot.returned=f->snapshot.eof=1;f->snapshot.actual_body=f->body;f->snapshot.length=3;f->parse.parser.actual_body=f->body;f->parse.parser.actual_extent=3;
 policydb_t *p=&f->parse.parser.policy;p->sl_source_owner=&f->parse.parser.memory;p->policy_type=POLICY_KERN;p->target_platform=SEPOL_TARGET_SELINUX;p->p_users.nprim=p->p_roles.nprim=p->p_types.nprim=1;
 f->users[0]=&f->user;f->roles[0]=&f->role_datum;f->types[0]=&f->type;p->user_val_to_struct=f->users;p->role_val_to_struct=f->roles;p->type_val_to_struct=f->types;
 f->user.s.value=f->role_datum.s.value=f->type.s.value=1;f->role_datum.flavor=ROLE_ROLE;f->type.flavor=TYPE_TYPE;
}
int main(void){
 fixture cases[7]={0};
 for(unsigned i=0;i<7;i++){
  fixture *f=&cases[i];prepare(f);status_failure=(i==4||i==5)?-77:0;status_equal=i!=6;expected_status=&f->snapshot.status;
  if(i==1||i==5)f->role[0].policy_type=2;
  if(i==2)f->role[0].evidence_original=NULL;
  if(i==3)f->root.capacity=0;
  int before=status_calls;long result=sl_linux_bind_candidate_roles(&f->root);
  assert(f->root.returned&&status_calls==before+1&&f->root.actual_parse==&f->parse&&f->root.policy_original==&f->parse.parser.policy);
  if(!i){assert(result==0&&f->bindings[0].returned&&f->bindings[0].type_original==&f->type&&f->bindings[0].evidence_original==&f->evidence[0]&&f->bindings[0].statement_original.bytes==f->statement+1);}
  else assert(result<0);
  if(i==1)assert(f->root.failure==SL_CANDIDATE_ROLE_DATUM&&f->bindings[0].type_value==2);
  if(i==2)assert(f->root.failure==SL_CANDIDATE_ROLE_STATEMENT&&!f->bindings[0].evidence_original);
  if(i==3)assert(f->root.failure==SL_CANDIDATE_ROLE_CAPACITY&&f->root.count==0);
  if(i==4)assert(result==-77&&f->root.status_result==-77&&f->bindings[0].result==0&&f->root.failure==SL_CANDIDATE_ROLE_STATUS);
  if(i==5)assert(result==-EINVAL&&f->root.failure==SL_CANDIDATE_ROLE_DATUM&&f->root.status_result==-77&&f->bindings[0].result==-EINVAL&&f->bindings[0].type_value==2);
  if(i==6)assert(result==-EINVAL&&f->root.failure==SL_CANDIDATE_ROLE_STATUS&&f->root.status_result==-EINVAL&&f->bindings[0].result==0);
  before=status_calls;assert(sl_linux_bind_candidate_roles(&f->root)==-EINVAL&&status_calls==before);
 }
 return 0;
}