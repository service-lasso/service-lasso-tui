/* UNRUN actual production owner-entry roster; substituted permission interpreter. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned calls,fail_kind;
long sl_linux_interpret_candidate_owner_transition(sl_linux_candidate_ptrace *p){calls++;p->attempted=p->returned=1;p->permission_kind=1;p->result=fail_kind==1?-91:0;return p->result;}
long sl_linux_interpret_candidate_owner_dyntransition(sl_linux_candidate_ptrace *p){calls++;p->attempted=p->returned=1;p->permission_kind=2;p->result=fail_kind==2?-92:0;return p->result;}
long sl_linux_interpret_candidate_payload_transition(sl_linux_candidate_ptrace *p){calls++;p->attempted=p->returned=1;p->permission_kind=3;p->result=fail_kind==5?-95:0;return p->result;}
long sl_linux_interpret_candidate_payload_dyntransition(sl_linux_candidate_ptrace *p){calls++;p->attempted=p->returned=1;p->permission_kind=4;p->result=fail_kind==6?-96:0;return p->result;}
#include "../candidate_startup_owner_entry_linux.c"
typedef struct{sl_linux_public_startup startup;sl_linux_candidate_ptrace entry[4];sl_linux_candidate_roles roles[1];sl_linux_candidate_payload_context payload[1];sl_linux_selinux_context owner[1];} fixture;
static void prepare(fixture *f){sl_linux_public_startup *s=&f->startup;s->returned=s->candidate_ptrace_returned=1;s->stage=SL_STARTUP_ACQUIRED;s->candidate_owner_entry=f->entry;s->candidate_owner_entry_capacity=4;s->actual_policy_capacity=s->actual_context_capacity=s->actual_candidate_role_capacity=s->actual_candidate_payload_capacity=s->mounts.selinux_count=1;s->actual_candidate_roles=f->roles;s->actual_contexts=f->owner;s->actual_candidate_payloads=f->payload;for(unsigned i=0;i<4;i++){f->entry[i].roles_original=f->roles;f->entry[i].owner_original=f->owner;f->entry[i].payload_original=&f->payload[0].reachable;f->entry[i].target_original=i>=2?&f->payload[0].context:f->owner;}}
int main(void){fixture cases[7]={0};for(unsigned i=0;i<7;i++){fixture *f=&cases[i];prepare(f);fail_kind=i;if(i==3)f->entry[1].owner_original=NULL;if(i==4)f->startup.candidate_ptrace_result=-93;unsigned before=calls;long result=sl_linux_interpret_startup_candidate_owner_entry(&f->startup);assert(f->startup.candidate_owner_entry_returned&&f->startup.actual_candidate_owner_entry==f->entry);
 if(!i)assert(!result&&calls==before+4&&f->startup.candidate_owner_entry_count==4&&f->entry[0].permission_kind==1&&f->entry[1].permission_kind==2&&f->entry[2].permission_kind==3&&f->entry[3].permission_kind==4);
 else if(i==1)assert(result==-91&&calls==before+1&&!f->entry[1].attempted);
 else if(i==2)assert(result==-92&&calls==before+2&&!f->entry[0].result&&f->entry[1].result==-92);
 else if(i==5)assert(result==-95&&calls==before+3&&!f->entry[3].attempted);
 else if(i==6)assert(result==-96&&calls==before+4&&f->entry[3].result==-96);
 else assert(result==-EINVAL&&calls==before&&!f->entry[0].attempted&&!f->entry[1].attempted);
 before=calls;assert(sl_linux_interpret_startup_candidate_owner_entry(&f->startup)==-EINVAL&&calls==before);
 }return 0;}