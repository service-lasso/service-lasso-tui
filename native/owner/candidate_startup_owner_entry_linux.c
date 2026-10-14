#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_startup_candidate_owner_entry(sl_linux_public_startup *s){
 if(!s||s->candidate_owner_entry_attempted)return -EINVAL;s->candidate_owner_entry_attempted=1;s->candidate_owner_entry_result=-EINVAL;
 s->actual_candidate_owner_entry=s->candidate_owner_entry;s->actual_candidate_owner_entry_capacity=s->candidate_owner_entry_capacity;
 if(!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_ptrace_returned||s->candidate_ptrace_result||!s->actual_candidate_owner_entry||!s->actual_candidate_roles||!s->actual_candidate_payloads||!s->actual_contexts||!s->actual_policy_capacity||s->actual_policy_capacity>ULONG_MAX/4||s->actual_candidate_owner_entry_capacity!=4*s->actual_policy_capacity||s->actual_context_capacity!=s->actual_policy_capacity||s->actual_candidate_role_capacity!=s->actual_policy_capacity||s->actual_candidate_payload_capacity!=s->actual_policy_capacity||s->mounts.selinux_count>s->actual_policy_capacity||s->candidate_owner_entry_count)goto done;
 for(unsigned long i=0;i<s->actual_candidate_owner_entry_capacity;i++){
  unsigned long policy=i/4;sl_linux_candidate_ptrace *p=&s->actual_candidate_owner_entry[i];
  if(p->attempted||p->roles_original!=&s->actual_candidate_roles[policy]||p->owner_original!=&s->actual_contexts[policy]||p->payload_original!=&s->actual_candidate_payloads[policy].reachable||p->target_original!=(i%4>=2?&s->actual_candidate_payloads[policy].context:&s->actual_contexts[policy]))goto done;
 }
 for(unsigned long i=0;i<4*s->mounts.selinux_count;i++){
  s->candidate_owner_entry_count=i+1;sl_linux_candidate_ptrace *p=&s->actual_candidate_owner_entry[i];
  s->candidate_owner_entry_result=i%4==0?sl_linux_interpret_candidate_owner_transition(p):i%4==1?sl_linux_interpret_candidate_owner_dyntransition(p):i%4==2?sl_linux_interpret_candidate_payload_transition(p):sl_linux_interpret_candidate_payload_dyntransition(p);
  if(s->candidate_owner_entry_result)goto done;
 }
 s->candidate_owner_entry_result=0;
 done:s->candidate_owner_entry_returned=1;return s->candidate_owner_entry_result;
 /* Process entry permission sources only; protected file namespace, exact
  * loaded code, endpoint suppliers and initial premise stay separate. */
}