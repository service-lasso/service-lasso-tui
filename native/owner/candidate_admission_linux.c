#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
/* This composition runs under selected175's independently qualified initial
 * loaded-code/OS/evidence premise. No parsed record authenticates that premise.
 * Each controlling source result remains separate and inspectable. */
#define ADMIT_REQUIRE(condition,original) do{if(!(condition)){o->failure_original=(void*)(original);goto done;}}while(0)
long sl_linux_admit_candidate_birth(sl_linux_public_controller *c){
 if(!c||c->birth_admission.attempted)return -EINVAL;
 sl_linux_birth_admission *o=&c->birth_admission;o->attempted=1;o->controller_original=c;o->result=-EINVAL;
 sl_linux_public_startup *s=c->startup_original;o->startup_original=s;o->stage=1;
 ADMIT_REQUIRE(s&&s->controller_original==c&&s->returned&&!s->result&&s->stage==SL_STARTUP_ACQUIRED,s);
 ADMIT_REQUIRE(c->construction_returned&&c->construction_body_returned&&c->construction_body_completed&&!c->construction_result&&c->birth_result>0,c);
 ADMIT_REQUIRE(s->actual_candidate&&s->actual_candidate==s->candidate_original&&s->actual_candidate->returned&&!s->actual_candidate->result,s->actual_candidate);
 unsigned long P=s->actual_policy_capacity,N=s->mounts.selinux_count;
 ADMIT_REQUIRE(P&&P<=ULONG_MAX/4&&N&&N<=P&&s->actual_candidate_policies&&s->actual_candidate_policy_capacity==P&&s->actual_candidate_roles&&s->actual_candidate_role_capacity==P&&s->actual_candidate_payloads&&s->actual_candidate_payload_capacity==P&&s->actual_contexts&&s->actual_context_capacity==P&&s->actual_parses&&s->actual_parse_capacity==P,s);
 ADMIT_REQUIRE(s->control_roster.returned&&!s->control_roster.result&&s->control_roster.startup_original==s,&s->control_roster);
 ADMIT_REQUIRE(s->genfs_correspondences&&s->genfs_correspondence_capacity==P&&s->migration_routes&&s->migration_route_capacity==3*P&&s->migration_routes_returned&&!s->migration_routes_result,s);
 ADMIT_REQUIRE(s->candidate_roles_returned&&!s->candidate_roles_result&&s->candidate_routes_returned&&!s->candidate_routes_result&&s->candidate_ptrace_returned&&!s->candidate_ptrace_result&&s->candidate_owner_entry_returned&&!s->candidate_owner_entry_result&&s->candidate_file_entry_returned&&!s->candidate_file_entry_result,s);
 ADMIT_REQUIRE(s->actual_candidate_ptrace&&s->actual_candidate_ptrace_capacity==2*P&&s->candidate_ptrace_count==2*N&&s->actual_candidate_owner_entry&&s->actual_candidate_owner_entry_capacity==4*P&&s->candidate_owner_entry_count==4*N&&s->actual_candidate_file_entry&&s->actual_candidate_file_entry_capacity==P&&s->candidate_file_entry_count==N,s);
 ADMIT_REQUIRE(s->candidate_socket_context_returned&&!s->candidate_socket_context_result&&s->actual_candidate_socket_contexts&&s->actual_candidate_socket_context_capacity==2*P&&s->candidate_socket_context_count==2*N&&s->candidate_socket_create_returned&&!s->candidate_socket_create_result&&s->actual_candidate_socket_creates&&s->actual_candidate_socket_create_capacity==2*P&&s->candidate_socket_create_count==2*N&&s->candidate_socket_use_returned&&!s->candidate_socket_use_result&&s->candidate_socket_uses&&s->candidate_socket_use_capacity==4*P&&s->candidate_socket_use_count==4*N,s);
 o->stage=2;
 for(unsigned long p=0;p<N;p++){
  sl_linux_candidate_policy *policy=&s->actual_candidate_policies[p];sl_linux_candidate_roles *roles=&s->actual_candidate_roles[p];sl_linux_candidate_payload_context *payload=&s->actual_candidate_payloads[p];sl_linux_genfs_correspondence *cohort=&s->genfs_correspondences[p];sl_linux_selinux_context *owner=&s->actual_contexts[p];o->checked_policies=p+1;
  ADMIT_REQUIRE(policy->returned&&!policy->result&&!policy->status_result&&policy->actual_candidate==s->actual_candidate&&policy->actual_parse==&s->actual_parses[p],policy);
  ADMIT_REQUIRE(roles->returned&&!roles->result&&!roles->status_result&&roles->actual_candidate==s->actual_candidate&&roles->actual_parse==policy->actual_parse,roles);
  ADMIT_REQUIRE(owner->returned&&!owner->result&&owner->actual_parse==policy->actual_parse&&payload->returned&&!payload->result&&payload->actual_policy==policy&&payload->context.returned&&!payload->context.result&&payload->reachable.returned&&!payload->reachable.result,payload);
  ADMIT_REQUIRE(cohort->returned&&!cohort->result&&!cohort->status_result&&cohort->policy_original==policy&&cohort->candidate_original==s->actual_candidate&&cohort->count&&cohort->count<=cohort->actual_capacity&&cohort->actual_declarations,cohort);
  for(size_t j=0;j<cohort->count;j++){sl_linux_genfs_declaration *d=&cohort->actual_declarations[j];ADMIT_REQUIRE(d->returned&&!d->result&&d->declaration_original&&d->evidence_original&&d->statement_original.bytes&&d->statement_original.extent,d);for(unsigned k=0;k<4;k++)ADMIT_REQUIRE(d->identity_checked[k],d);o->checked_edges++;}
  for(unsigned k=0;k<3;k++){sl_linux_migration_routes *r=&s->migration_routes[3*p+k];ADMIT_REQUIRE(r->returned&&!r->result&&!r->status_result&&r->startup_original==s&&r->roles_original==roles&&r->owner_original==owner&&r->payload_original==&payload->reachable&&r->access_original&&r->access_original->returned&&!r->access_original->result,r);o->checked_edges++;}
  for(unsigned k=0;k<2;k++){sl_linux_candidate_ptrace *r=&s->actual_candidate_ptrace[2*p+k];ADMIT_REQUIRE(r->returned&&!r->result&&!r->status_result&&r->actual_roles==roles&&r->actual_owner==owner&&r->actual_payload==&payload->reachable,r);o->checked_edges++;}
  for(unsigned k=0;k<4;k++){sl_linux_candidate_ptrace *r=&s->actual_candidate_owner_entry[4*p+k];ADMIT_REQUIRE(r->returned&&!r->result&&!r->status_result&&r->actual_roles==roles&&r->actual_owner==owner&&r->actual_payload==&payload->reachable&&r->actual_target==(k>=2?&payload->context:owner),r);o->checked_edges++;}
  sl_linux_candidate_file_entry *file=&s->actual_candidate_file_entry[p];ADMIT_REQUIRE(file->returned&&!file->result&&!file->status_result&&file->actual_policy==policy&&file->actual_owner==owner&&file->actual_payload==payload&&!file->exec_context_result&&!file->exec_type_result,file);o->checked_edges++;
  for(unsigned peer=0;peer<2;peer++){sl_linux_candidate_socket_context *context=&s->actual_candidate_socket_contexts[2*p+peer];sl_linux_candidate_socket_create *create=&s->actual_candidate_socket_creates[2*p+peer];ADMIT_REQUIRE(context->returned&&!context->result&&context->actual_policy==policy&&context->actual_peer==&c->entry.custody_peers[peer],context);ADMIT_REQUIRE(create->returned&&!create->result&&!create->status_result&&!create->purpose&&create->actual_socket==context&&create->actual_roles==roles&&create->actual_owner==owner&&create->actual_payload==&payload->reachable,create);o->checked_edges++;
   for(unsigned use=0;use<2;use++){sl_linux_candidate_socket_create *r=&s->candidate_socket_uses[4*p+2*peer+use];ADMIT_REQUIRE(r->returned&&!r->result&&!r->status_result&&r->purpose==use+1&&r->actual_socket==context&&r->actual_roles==roles&&r->actual_owner==owner&&r->actual_payload==&payload->reachable,r);o->checked_edges++;}
  }
 }
 o->stage=3;
 ADMIT_REQUIRE(c->namespace_target.returned&&!c->namespace_target.result&&c->namespace_target.controller_original==c&&c->namespace_target.target_original==&s->candidate_target&&c->namespace_target.namespace_original==&c->namespace_observation,&c->namespace_target);
 ADMIT_REQUIRE(s->candidate_target.returned&&!s->candidate_target.result&&s->candidate_target.bound&&s->initial_family.returned&&!s->initial_family.result&&s->initial_family.actual_target==&s->candidate_target&&s->initial_family.framing_complete&&!s->initial_family.populated&&!s->initial_family.frozen,&s->initial_family);
 ADMIT_REQUIRE(c->authority_observed_by&&c->authority_observed_by->returned&&c->authority_observed_by->controller_original==c&&c->authority_observed_by->authority.returned&&!c->authority_observed_by->authority.result&&c->authority_observed_by->selinux.returned&&!c->authority_observed_by->selinux.result,c->authority_observed_by);
 for(unsigned peer=0;peer<2;peer++){sl_linux_socket_state *r=&c->entry.custody_states[peer];ADMIT_REQUIRE(r->returned&&!r->result&&r->framing_complete,r);o->checked_edges++;}
 o->stage=4;o->result=0;
 done:o->returned=1;return o->result;
}
#undef ADMIT_REQUIRE
#endif