#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_startup_socket_uses(sl_linux_public_startup *s){
 if(!s||s->candidate_socket_use_attempted)return -EINVAL;s->candidate_socket_use_attempted=1;s->candidate_socket_use_result=-EINVAL;
 if(!s->candidate_socket_create_returned||s->candidate_socket_create_result||!s->actual_policy_capacity||s->actual_policy_capacity>ULONG_MAX/4||!s->actual_candidate_roles||!s->actual_contexts||!s->actual_candidate_payloads||!s->actual_candidate_socket_contexts||!s->candidate_socket_uses||s->candidate_socket_use_capacity!=4*s->actual_policy_capacity||s->actual_candidate_role_capacity!=s->actual_policy_capacity||s->actual_context_capacity!=s->actual_policy_capacity||s->actual_candidate_payload_capacity!=s->actual_policy_capacity||s->actual_candidate_socket_context_capacity!=2*s->actual_policy_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_policy_capacity)goto done;
 for(unsigned long i=0;i<s->candidate_socket_use_capacity;i++){
  sl_linux_candidate_socket_create *o=&s->candidate_socket_uses[i];unsigned long p=i/4,peer=(i/2)%2;
  if(o->attempted||o->purpose!=1+i%2||o->roles_original!=&s->actual_candidate_roles[p]||o->owner_original!=&s->actual_contexts[p]||o->payload_original!=&s->actual_candidate_payloads[p].reachable||o->socket_original!=&s->actual_candidate_socket_contexts[2*p+peer])goto done;
 }
 for(unsigned long i=0;i<4*s->mounts.selinux_count;i++){
  s->candidate_socket_use_count=i+1;s->candidate_socket_use_result=sl_linux_interpret_candidate_socket_use(&s->candidate_socket_uses[i]);if(s->candidate_socket_use_result)goto done;
 }
 s->candidate_socket_use_result=0;
 done:s->candidate_socket_use_returned=1;return s->candidate_socket_use_result;
}
