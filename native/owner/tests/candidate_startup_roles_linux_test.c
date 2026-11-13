/* UNRUN startup caller cases; controlled policy consumer, no native admission. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned calls,policy_calls;static sl_linux_candidate_roles *failed;
long sl_linux_bind_candidate_roles(sl_linux_candidate_roles *r){calls++;r->attempted=r->returned=1;r->result=r==failed?-88:0;return r->result;}
long sl_linux_bind_candidate_policy(sl_linux_candidate_policy *p){policy_calls++;p->attempted=p->returned=1;return p->result;}
#include "../candidate_startup_roles_linux.c"
typedef struct {sl_linux_public_startup startup;sl_candidate_profile candidate;sl_linux_candidate_roles roles[2];sl_linux_policy_parse parses[2];sl_linux_candidate_policy policies[2];} fixture;
static void prepare(fixture *f){
 f->startup.attempted=f->startup.returned=1;f->startup.stage=SL_STARTUP_ACQUIRED;
 f->startup.actual_candidate=&f->candidate;f->candidate.attempted=f->candidate.returned=1;
 f->startup.candidate_roles=f->roles;f->startup.candidate_role_capacity=2;
 f->startup.actual_parses=f->parses;f->startup.actual_parse_capacity=2;
 f->startup.candidate_policies=f->policies;f->startup.candidate_policy_capacity=2;
 f->startup.mounts.returned=1;f->startup.mounts.selinux_count=2;
 for(unsigned i=0;i<2;i++){f->policies[i].candidate_original=&f->candidate;f->policies[i].parse_original=&f->parses[i];f->roles[i].candidate_original=&f->candidate;f->roles[i].parse_original=&f->parses[i];}
}
int main(void){
 fixture good={0},partial={0},stale={0},policy_failure={0};prepare(&good);prepare(&partial);prepare(&stale);
 assert(sl_linux_bind_startup_candidate_roles(&good.startup)==0&&calls==2&&policy_calls==2&&good.startup.candidate_role_count==2&&good.startup.candidate_roles_returned);
 failed=&partial.roles[1];assert(sl_linux_bind_startup_candidate_roles(&partial.startup)==-88&&calls==4&&policy_calls==4&&partial.roles[0].result==0&&partial.roles[1].result==-88&&partial.startup.candidate_roles_returned);
 stale.roles[1].parse_original=&good.parses[1];assert(sl_linux_bind_startup_candidate_roles(&stale.startup)==-EINVAL&&calls==4&&policy_calls==4&&!stale.roles[0].attempted);
 assert(sl_linux_bind_startup_candidate_roles(&partial.startup)==-EINVAL&&calls==4&&policy_calls==4&&partial.roles[1].result==-88);
 prepare(&policy_failure);policy_failure.policies[0].result=-55;
 assert(sl_linux_bind_startup_candidate_roles(&policy_failure.startup)==-55&&policy_calls==5&&calls==4&&!policy_failure.roles[0].attempted&&policy_failure.startup.candidate_policy_count==1&&policy_failure.startup.candidate_role_count==0);
 return 0;
}