/* UNRUN actual composition with separately retained controlled source owners.
 * Fixture values are not an authenticated candidate or native admission. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include "../candidate_admission_linux.c"
typedef struct {
 sl_linux_public_controller c;sl_linux_public_startup s;sl_candidate_profile candidate;
 sl_linux_candidate_policy policy;sl_linux_policy_parse parse;sl_linux_candidate_roles roles;sl_linux_selinux_context owner;
 sl_linux_candidate_payload_context payload;sl_linux_genfs_correspondence cohort;sl_linux_genfs_declaration declarations[2];
 sl_candidate_record statement,evidence;unsigned char statement_byte;
 sl_linux_migration_routes routes[3];sl_linux_migration_access access[3];
 sl_linux_candidate_ptrace ptrace[2],entry[4];sl_linux_candidate_file_entry file;
 sl_linux_candidate_socket_context sockets[2];sl_linux_candidate_socket_create creates[2],uses[4];sl_linux_service_observation service;
} fixture;
static void prepare(fixture *f){
 sl_linux_public_startup *s=&f->s;sl_linux_public_controller *c=&f->c;c->startup_original=s;s->controller_original=c;s->returned=1;s->stage=SL_STARTUP_ACQUIRED;c->construction_returned=c->construction_body_returned=c->construction_body_completed=1;c->birth_result=42;
 s->actual_candidate=s->candidate_original=&f->candidate;f->candidate.returned=1;s->actual_policy_capacity=s->mounts.selinux_count=1;
 s->actual_candidate_policies=&f->policy;s->actual_candidate_policy_capacity=1;s->actual_parses=&f->parse;s->actual_parse_capacity=1;s->actual_candidate_roles=&f->roles;s->actual_candidate_role_capacity=1;s->actual_candidate_payloads=&f->payload;s->actual_candidate_payload_capacity=1;s->actual_contexts=&f->owner;s->actual_context_capacity=1;
 s->control_roster.returned=1;s->control_roster.startup_original=s;s->genfs_correspondences=&f->cohort;s->genfs_correspondence_capacity=1;s->migration_routes=f->routes;s->migration_route_capacity=3;s->migration_routes_returned=1;
 s->candidate_roles_returned=s->candidate_routes_returned=s->candidate_ptrace_returned=s->candidate_owner_entry_returned=s->candidate_file_entry_returned=1;
 s->actual_candidate_ptrace=f->ptrace;s->actual_candidate_ptrace_capacity=s->candidate_ptrace_count=2;s->actual_candidate_owner_entry=f->entry;s->actual_candidate_owner_entry_capacity=s->candidate_owner_entry_count=4;s->actual_candidate_file_entry=&f->file;s->actual_candidate_file_entry_capacity=s->candidate_file_entry_count=1;
 s->candidate_socket_context_returned=s->candidate_socket_create_returned=s->candidate_socket_use_returned=1;s->actual_candidate_socket_contexts=f->sockets;s->actual_candidate_socket_context_capacity=s->candidate_socket_context_count=2;s->actual_candidate_socket_creates=f->creates;s->actual_candidate_socket_create_capacity=s->candidate_socket_create_count=2;s->candidate_socket_uses=f->uses;s->candidate_socket_use_capacity=s->candidate_socket_use_count=4;
 f->policy.returned=1;f->policy.actual_candidate=&f->candidate;f->policy.actual_parse=&f->parse;f->roles.returned=1;f->roles.actual_candidate=&f->candidate;f->roles.actual_parse=&f->parse;f->owner.returned=1;f->owner.actual_parse=&f->parse;
 f->payload.returned=f->payload.context.returned=f->payload.reachable.returned=1;f->payload.actual_policy=&f->policy;
 f->cohort.returned=1;f->cohort.policy_original=&f->policy;f->cohort.candidate_original=&f->candidate;f->cohort.actual_declarations=f->declarations;f->cohort.count=f->cohort.actual_capacity=2;
 for(unsigned j=0;j<2;j++){sl_linux_genfs_declaration *d=&f->declarations[j];d->returned=1;d->declaration_original=&f->statement;d->evidence_original=&f->evidence;d->statement_original=(sl_candidate_span){&f->statement_byte,1};for(unsigned k=0;k<4;k++)d->identity_checked[k]=1;}
 for(unsigned k=0;k<3;k++){sl_linux_migration_routes *r=&f->routes[k];r->returned=1;r->startup_original=s;r->roles_original=&f->roles;r->owner_original=&f->owner;r->payload_original=&f->payload.reachable;r->access_original=&f->access[k];f->access[k].returned=1;}
 for(unsigned k=0;k<2;k++){sl_linux_candidate_ptrace *r=&f->ptrace[k];r->returned=1;r->actual_roles=&f->roles;r->actual_owner=&f->owner;r->actual_payload=&f->payload.reachable;}
 for(unsigned k=0;k<4;k++){sl_linux_candidate_ptrace *r=&f->entry[k];r->returned=1;r->actual_roles=&f->roles;r->actual_owner=&f->owner;r->actual_payload=&f->payload.reachable;r->actual_target=k>=2?&f->payload.context:&f->owner;}
 f->file.returned=1;f->file.actual_policy=&f->policy;f->file.actual_owner=&f->owner;f->file.actual_payload=&f->payload;
 for(unsigned peer=0;peer<2;peer++){f->sockets[peer].returned=1;f->sockets[peer].actual_policy=&f->policy;f->sockets[peer].actual_peer=&c->entry.custody_peers[peer];sl_linux_candidate_socket_create *r=&f->creates[peer];r->returned=1;r->actual_socket=&f->sockets[peer];r->actual_roles=&f->roles;r->actual_owner=&f->owner;r->actual_payload=&f->payload.reachable;
  for(unsigned use=0;use<2;use++){r=&f->uses[2*peer+use];r->purpose=use+1;r->returned=1;r->actual_socket=&f->sockets[peer];r->actual_roles=&f->roles;r->actual_owner=&f->owner;r->actual_payload=&f->payload.reachable;}
  c->entry.custody_states[peer].returned=c->entry.custody_states[peer].framing_complete=1;
 }
 c->namespace_target.returned=1;c->namespace_target.controller_original=c;c->namespace_target.target_original=&s->candidate_target;c->namespace_target.namespace_original=&c->namespace_observation;
 s->candidate_target.returned=s->candidate_target.bound=1;s->initial_family.returned=s->initial_family.framing_complete=1;s->initial_family.actual_target=&s->candidate_target;c->authority_observed_by=&f->service;f->service.returned=1;f->service.controller_original=c;f->service.authority.returned=f->service.selinux.returned=1;
}
int main(void){static fixture originals[7];for(unsigned mode=0;mode<7;mode++){fixture *f=&originals[mode];prepare(f);void *expected=0;
 if(mode==1){f->declarations[1].result=-EPERM;expected=&f->declarations[1];}if(mode==2){f->file.status_result=-EIO;expected=&f->file;}if(mode==3){f->uses[3].actual_payload=0;expected=&f->uses[3];}if(mode==4){f->c.namespace_target.result=-EIO;expected=&f->c.namespace_target;}if(mode==5){f->s.initial_family.frozen=1;expected=&f->s.initial_family;}if(mode==6){f->c.entry.custody_states[1].result=-EIO;expected=&f->c.entry.custody_states[1];}
 long r=sl_linux_admit_candidate_birth(&f->c);sl_linux_birth_admission *o=&f->c.birth_admission;assert(o->returned&&r==(mode?-EINVAL:0)&&o->failure_original==expected);if(!mode)assert(o->stage==4&&o->checked_policies==1&&o->checked_edges==20);assert(sl_linux_admit_candidate_birth(&f->c)==-EINVAL&&o->failure_original==expected&&o->result==r);if(mode==1)assert(f->declarations[1].result==-EPERM);if(mode==2)assert(f->file.status_result==-EIO);
 }return 0;}