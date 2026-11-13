#include "public_startup_linux.h"
#include <errno.h>
long sl_linux_interpret_startup_candidate_routes(sl_linux_public_startup *s){
 if(!s||s->candidate_routes_attempted)return -EINVAL;s->candidate_routes_attempted=1;s->candidate_routes_result=-EINVAL;
 s->actual_candidate_payloads=s->candidate_payloads;s->actual_candidate_payload_capacity=s->candidate_payload_capacity;
 s->actual_candidate_routes=s->candidate_routes;s->actual_candidate_route_capacity=s->candidate_route_capacity;
 if(!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_roles_returned||s->candidate_roles_result||!s->actual_candidate_payloads||s->actual_candidate_payload_capacity!=s->actual_parse_capacity||!s->actual_candidate_routes||s->actual_candidate_route_capacity!=s->actual_control_access_capacity||s->candidate_payload_count||s->candidate_route_count||!s->actual_candidate_policies||!s->actual_candidate_roles||!s->actual_control_access||!s->actual_policy_capacity||s->actual_policy_capacity!=s->actual_candidate_payload_capacity||s->actual_candidate_role_capacity!=s->actual_policy_capacity||s->actual_candidate_policy_capacity!=s->actual_policy_capacity||s->mounts.selinux_count>s->actual_policy_capacity)goto done;
 for(unsigned long i=0;i<s->actual_candidate_payload_capacity;i++){
  sl_linux_candidate_payload_context *p=&s->actual_candidate_payloads[i];
  if(p->attempted||p->policy_original!=&s->actual_candidate_policies[i])goto done;
 }
 for(unsigned long i=0;i<s->actual_candidate_route_capacity;i++){
  sl_linux_candidate_routes *r=&s->actual_candidate_routes[i];unsigned long policy=(i/2)%s->actual_policy_capacity;
  if(r->attempted||r->roles_original!=&s->actual_candidate_roles[policy]||r->control_original!=&s->actual_control_access[i]||r->payload_original!=&s->actual_candidate_payloads[policy].reachable)goto done;
 }
 for(unsigned long i=0;i<s->mounts.selinux_count;i++){
  s->candidate_payload_count=i+1;s->candidate_routes_result=sl_linux_interpret_candidate_payload_context(&s->actual_candidate_payloads[i]);if(s->candidate_routes_result)goto done;
 }
 for(unsigned long i=0;i<s->actual_candidate_route_capacity;i++){
  sl_linux_control_access *access=&s->actual_control_access[i];
  if(!access->attempted)continue;
  s->candidate_route_count=i+1;s->candidate_routes_result=sl_linux_interpret_candidate_routes(&s->actual_candidate_routes[i]);if(s->candidate_routes_result)goto done;
 }
 s->candidate_routes_result=0;
 done:s->candidate_routes_returned=1;return s->candidate_routes_result;
}