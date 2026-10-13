#include "public_startup_linux.h"
#include <errno.h>
long sl_linux_interpret_startup_candidate_file_entry(sl_linux_public_startup *s){
 if(!s||s->candidate_file_entry_attempted)return -EINVAL;s->candidate_file_entry_attempted=1;s->candidate_file_entry_result=-EINVAL;
 s->actual_candidate_file_entry=s->candidate_file_entry;s->actual_candidate_file_entry_capacity=s->candidate_file_entry_capacity;
 if(!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_ptrace_returned||s->candidate_ptrace_result||!s->actual_candidate_file_entry||!s->actual_candidate_policies||!s->actual_candidate_payloads||!s->actual_contexts||!s->actual_exec_contexts||!s->actual_policy_capacity||s->actual_policy_capacity>~0UL/2||s->actual_exec_context_capacity!=2*s->actual_policy_capacity||s->actual_context_capacity!=s->actual_policy_capacity||s->actual_candidate_policy_capacity!=s->actual_policy_capacity||s->actual_candidate_payload_capacity!=s->actual_policy_capacity||s->actual_candidate_file_entry_capacity!=s->actual_policy_capacity||s->mounts.selinux_count>s->actual_policy_capacity)goto done;
 for(unsigned long i=0;i<s->actual_candidate_file_entry_capacity;i++){
  sl_linux_candidate_file_entry *e=&s->actual_candidate_file_entry[i];
  if(e->attempted||e->policy_original!=&s->actual_candidate_policies[i]||e->owner_original!=&s->actual_contexts[i]||e->payload_original!=&s->actual_candidate_payloads[i]||e->public_original!=&s->actual_exec_contexts[i]||e->file_original!=&s->actual_exec_contexts[s->actual_policy_capacity+i])goto done;
 }
 for(unsigned long i=0;i<s->mounts.selinux_count;i++){s->candidate_file_entry_count=i+1;s->candidate_file_entry_result=sl_linux_interpret_candidate_file_entry(&s->actual_candidate_file_entry[i]);if(s->candidate_file_entry_result)goto done;}
 s->candidate_file_entry_result=0;
 done:s->candidate_file_entry_returned=1;return s->candidate_file_entry_result;
}