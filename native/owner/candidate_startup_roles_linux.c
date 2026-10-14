#include "public_startup_linux.h"
#include <errno.h>
long sl_linux_bind_startup_candidate_roles(sl_linux_public_startup *s){
 if(!s||s->candidate_roles_attempted)return -EINVAL;s->candidate_roles_attempted=1;s->candidate_roles_result=-EINVAL;
 s->actual_candidate_roles=s->candidate_roles;s->actual_candidate_role_capacity=s->candidate_role_capacity;
 s->actual_candidate_policies=s->candidate_policies;s->actual_candidate_policy_capacity=s->candidate_policy_capacity;
 if(!s->attempted||!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->actual_candidate||!s->actual_candidate->attempted||!s->actual_candidate->returned||s->actual_candidate->result||!s->actual_candidate_policies||s->actual_candidate_policy_capacity!=s->actual_parse_capacity||!s->actual_parses||!s->actual_candidate_roles||s->actual_candidate_role_capacity!=s->actual_parse_capacity||s->candidate_role_count||s->candidate_policy_count||!s->mounts.returned||s->mounts.result||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_candidate_role_capacity)goto done;
 /* Bind the complete prepared roster, not only successful/observed episodes.
  * Their disjoint output backing was fixed by the source candidate arena. */
 for(unsigned long i=0;i<s->actual_candidate_role_capacity;i++){
  sl_linux_candidate_policy *p=&s->actual_candidate_policies[i];
  if(p->attempted||p->candidate_original!=s->actual_candidate||p->parse_original!=&s->actual_parses[i])goto done;
  sl_linux_candidate_roles *r=&s->actual_candidate_roles[i];
  if(r->attempted||r->candidate_original!=s->actual_candidate||r->parse_original!=&s->actual_parses[i]||r->capacity!=s->actual_candidate->counts[1])goto done;
 }
 for(unsigned long i=0;i<s->mounts.selinux_count;i++){

  s->candidate_policy_count=i+1;
  s->candidate_roles_result=sl_linux_bind_candidate_policy(&s->actual_candidate_policies[i]);if(s->candidate_roles_result)goto done;
  sl_linux_candidate_roles *r=&s->actual_candidate_roles[i];s->candidate_role_count=i+1;
  s->candidate_roles_result=sl_linux_bind_candidate_roles(r);if(s->candidate_roles_result)goto done;
 }
 s->candidate_roles_result=0;
 done:s->candidate_roles_returned=1;return s->candidate_roles_result;
}