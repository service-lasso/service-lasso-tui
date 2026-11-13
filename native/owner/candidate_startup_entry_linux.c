#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_startup_candidate_ptrace(sl_linux_public_startup *s){
 if(!s||s->candidate_ptrace_attempted)return -EINVAL;s->candidate_ptrace_attempted=1;s->candidate_ptrace_result=-EINVAL;
 s->actual_candidate_ptrace=s->candidate_ptrace;s->actual_candidate_ptrace_capacity=s->candidate_ptrace_capacity;
 if(!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_roles_returned||s->candidate_roles_result||!s->candidate_routes_returned||s->candidate_routes_result||!s->actual_candidate_ptrace||!s->actual_candidate_roles||!s->actual_candidate_payloads||!s->actual_contexts||!s->actual_policy_capacity||s->actual_policy_capacity>ULONG_MAX/2||s->actual_candidate_ptrace_capacity!=2*s->actual_policy_capacity||s->actual_context_capacity!=s->actual_policy_capacity||s->actual_candidate_role_capacity!=s->actual_policy_capacity||s->actual_candidate_payload_capacity!=s->actual_policy_capacity||s->mounts.selinux_count>s->actual_policy_capacity||s->candidate_ptrace_count)goto done;
 for(unsigned long i=0;i<s->actual_candidate_ptrace_capacity;i++){
  unsigned long policy=i/2;sl_linux_candidate_ptrace *p=&s->actual_candidate_ptrace[i];
  if(p->attempted||p->roles_original!=&s->actual_candidate_roles[policy]||p->owner_original!=&s->actual_contexts[policy]||p->payload_original!=&s->actual_candidate_payloads[policy].reachable)goto done;
 }
 for(unsigned long i=0;i<2*s->mounts.selinux_count;i++){
  s->candidate_ptrace_count=i+1;s->candidate_ptrace_result=i%2?sl_linux_interpret_candidate_owner_proc_read(&s->actual_candidate_ptrace[i]):sl_linux_interpret_candidate_ptrace(&s->actual_candidate_ptrace[i]);if(s->candidate_ptrace_result)goto done;
 }
 s->candidate_ptrace_result=0;
 done:s->candidate_ptrace_returned=1;return s->candidate_ptrace_result;
}