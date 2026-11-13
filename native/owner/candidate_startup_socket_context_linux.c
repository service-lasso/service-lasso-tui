#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_startup_socket_contexts(sl_linux_public_startup *s){
 if(!s||s->candidate_socket_context_attempted)return -EINVAL;s->candidate_socket_context_attempted=1;s->candidate_socket_context_result=-EINVAL;s->actual_candidate_socket_contexts=s->candidate_socket_contexts;s->actual_candidate_socket_context_capacity=s->candidate_socket_context_capacity;
 sl_linux_public_controller *c=s->controller_original;
 if(!c||c->startup_original!=s||!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_preexec_returned||s->candidate_preexec_result||!s->actual_candidate_policies||!s->actual_policy_capacity||s->actual_policy_capacity>ULONG_MAX/2||s->actual_candidate_policy_capacity!=s->actual_policy_capacity||!s->actual_candidate_socket_contexts||s->actual_candidate_socket_context_capacity!=2*s->actual_policy_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_policy_capacity)goto done;
 for(unsigned long i=0;i<s->actual_candidate_socket_context_capacity;i++){sl_linux_candidate_socket_context *o=&s->actual_candidate_socket_contexts[i];if(o->attempted||o->policy_original!=&s->actual_candidate_policies[i/2]||o->peer_original!=&c->entry.custody_peers[i%2])goto done;}
 for(unsigned long i=0;i<2*s->mounts.selinux_count;i++){s->candidate_socket_context_count=i+1;s->candidate_socket_context_result=sl_linux_interpret_candidate_socket_context(&s->actual_candidate_socket_contexts[i]);if(s->candidate_socket_context_result)goto done;}
 s->candidate_socket_context_result=0;
 done:s->candidate_socket_context_returned=1;return s->candidate_socket_context_result;
}