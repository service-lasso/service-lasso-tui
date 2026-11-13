/* Controlled source cases UNRUN; not actual acquisition/native/fit evidence. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include "../public_startup_linux.h"
static sl_linux_selinux_context exec_contexts[2];static sl_linux_mount_location location;static sl_linux_cgroup_children child;static sl_linux_selinux_snapshot policy;static sl_linux_policy_parse parse;static sl_linux_selinux_context context;static sl_linux_bound_domains domains;static sl_linux_context_reachability reach;static sl_linux_control_access access[4];static sl_linux_selinux_context control_contexts[2];static sl_linux_child_view view;static sl_linux_cgroup_control_label controls[2];static unsigned char labels[2][32];static unsigned char body[1];static unsigned calls,fail_at;static sl_linux_public_startup *actual;
long sl_linux_prepare_public_streams(sl_linux_entry *entry){
 assert(++calls==1&&entry==&actual->prepared_controller->entry);
 assert(actual->installation.entry_original==entry&&actual->membership.entry_original==entry);
 assert(actual->immutable.installation_original==&actual->installation);
 assert(actual->actual_parses==&parse&&parse.snapshot_original==&policy&&!parse.attempted);
 assert(actual->actual_contexts==&context&&context.parse_original==&parse&&!context.self_attempted);
 assert(actual->actual_domains==&domains&&domains.context_original==&context&&!domains.attempted);
 if(fail_at==1)return -9;entry->public_streams_prepared=1;return 0;
}
long sl_linux_acquire_local_installation(sl_linux_local_installation *owner){
 assert(++calls==2&&owner==&actual->installation&&owner->entry_original->public_streams_prepared);
 owner->attempted=owner->returned=1;owner->public_fd=19;owner->result=fail_at==2?-13:0;return owner->result;
}
long sl_linux_observe_local_membership(sl_linux_local_membership *owner){
 assert(++calls==3&&owner==&actual->membership);
 owner->attempted=owner->returned=1;owner->fd=20;owner->result=fail_at==3?-5:0;return owner->result;
}
long sl_linux_observe_capability_domain(sl_linux_capability_domain *owner){
 assert(++calls==4&&owner==&actual->capability_domain&&owner->entry_original==&actual->prepared_controller->entry);
 owner->attempted=owner->returned=1;owner->fd=22;owner->result=fail_at==4?-6:0;return owner->result;
}
long sl_linux_prepare_immutable_payload(sl_linux_immutable_payload *owner){
 assert(++calls==5&&owner==&actual->immutable&&owner->installation_original==&actual->installation);
 owner->attempted=owner->returned=1;owner->fd=21;owner->result=fail_at==5?-28:0;return owner->result;
}
long sl_linux_observe_exec_label(sl_linux_exec_label *owner){
 assert(calls==5&&actual->immutable.returned&&!actual->immutable.result);
 assert((owner==&actual->public_label&&owner->installation_original==&actual->installation)||(owner==&actual->payload_label&&owner->payload_original==&actual->immutable));
 owner->attempted=owner->returned=1;return 0;
}
long sl_linux_observe_local_mounts(sl_linux_local_mounts *owner){
 assert(++calls==6&&owner==&actual->mounts&&owner->membership_original==&actual->membership);
 owner->attempted=owner->returned=1;owner->row_count=1;owner->selinux_count=1;owner->result=fail_at==6?-7:0;return owner->result;
}
long sl_linux_acquire_mount_location(sl_linux_mount_location *owner){
 assert(++calls==7&&owner==&location&&owner->mounts_original==&actual->mounts&&owner->row_index==0);
 owner->attempted=owner->returned=1;owner->result=fail_at==7?-8:0;return owner->result;
}
long sl_linux_acquire_cgroup_children(sl_linux_cgroup_children *owner){
 assert(++calls==8&&owner==&child&&owner->location_original==&location);
 owner->attempted=owner->returned=1;owner->eof=1;owner->actual_views=&view;owner->actual_view_capacity=owner->view_count=1;view.attempted=view.returned=1;owner->result=fail_at==8?-10:0;return owner->result;
}
long sl_linux_acquire_cgroup_control_label(sl_linux_cgroup_control_label *owner){
 unsigned phase=++calls;assert((phase==9||phase==10)&&owner==&controls[phase-9]);
 assert(owner->children_original==&child&&owner->view_original==&view&&owner->kind==phase-8);
 owner->attempted=owner->returned=1;owner->framing_complete=1;owner->raw_fd=owner->fd=30+(int)phase;owner->result=fail_at==phase?-17:0;return owner->result;
}long sl_linux_acquire_selinux_snapshot(sl_linux_selinux_snapshot *owner){
 assert(++calls==11&&owner==&policy&&owner->mounts_original==&actual->mounts);
 owner->attempted=owner->returned=owner->eof=1;owner->actual_body=body;owner->length=1;owner->actual_body_capacity=1;
 owner->result=fail_at==11?-11:0;return owner->result;
}
int sl_linux_parse_policy_snapshot(sl_linux_policy_parse *owner){
 assert(++calls==12&&owner==&parse&&owner->snapshot_original==&policy);
 owner->attempted=owner->returned=1;owner->result=fail_at==12?-12:0;return owner->result;
}
long sl_linux_observe_selinux_self(sl_linux_lsm_self *owner){
 assert(++calls==13&&owner==&actual->self_context);owner->attempted=owner->returned=1;owner->raw_query=fail_at==13?-7:1;owner->result=fail_at==13?-14:0;return owner->result;
}
int sl_linux_interpret_selinux_self(sl_linux_selinux_context *owner,sl_linux_lsm_self *self){
 assert(++calls==14&&owner==&context&&owner->parse_original==&parse&&self==&actual->self_context);
 owner->self_original=owner->actual_self=self;owner->self_attempted=owner->self_returned=1;owner->self_result=fail_at==14?-15:0;return owner->self_result;
}
int sl_linux_interpret_exec_context(sl_linux_selinux_context *owner){
 assert(calls==14&&owner->parse_original==&parse);
 assert((owner==&exec_contexts[0]&&owner->exec_original==&actual->public_label)||(owner==&exec_contexts[1]&&owner->exec_original==&actual->payload_label));
 owner->exec_attempted=owner->exec_returned=1;return 0;
}
int sl_linux_interpret_bound_domains(sl_linux_bound_domains *owner){
 assert(++calls==15&&owner==&domains&&owner->context_original==&context);owner->attempted=owner->returned=1;owner->result=fail_at==15?-16:0;return owner->result;
}
int sl_linux_interpret_control_context(sl_linux_selinux_context *owner){
 unsigned phase=++calls;assert((phase==16||phase==17)&&owner==&control_contexts[phase-16]);
 assert(owner->control_original==&controls[phase-16]&&owner->parse_original==&parse);
 owner->control_attempted=owner->control_returned=1;owner->control_result=fail_at==phase?-18:0;return owner->control_result;
}
int sl_linux_interpret_control_access(sl_linux_control_access *owner){
 unsigned phase=++calls;assert((phase>=18&&phase<=21)&&owner==&access[phase-18]);assert(owner->context_original==&control_contexts[(phase-18)/2]&&owner->kind_original==((phase-18)%2?SL_CONTROL_RELABEL_FROM:SL_CONTROL_WRITE_APPEND));
 owner->attempted=owner->returned=1;owner->result=fail_at==phase?-19:0;return owner->result;
}
int sl_linux_interpret_context_reachability(sl_linux_context_reachability *owner){
 assert(++calls==22&&owner==&reach&&owner->context_original==&context);owner->attempted=owner->returned=1;owner->result=fail_at==22?-20:0;return owner->result;
}
#include "../public_startup_linux.c"
int main(void){
 sl_linux_public_startup startup;sl_linux_public_controller controller;
 char arg0[]="service-lasso-tui",arg1[]="--help",setting[]="TOKEN=original";
 char *argv[]={arg0,arg1,0},*environment[]={setting,0};unsigned failure;
 for(failure=0;failure<=22;failure++){
  long result;unsigned prior;
  memset(&startup,0,sizeof(startup));memset(&controller,0,sizeof(controller));
  memset(exec_contexts,0,sizeof exec_contexts);startup.exec_contexts=exec_contexts;startup.exec_context_capacity=2;
  actual=&startup;calls=0;fail_at=failure;memset(&location,0,sizeof location);startup.locations=&location;startup.location_capacity=1;
  memset(&child,0,sizeof child);memset(&policy,0,sizeof policy);memset(&parse,0,sizeof parse);memset(&context,0,sizeof context);startup.contexts=&context;startup.context_capacity=1;memset(&domains,0,sizeof domains);startup.domains=&domains;startup.domain_capacity=1;
  memset(&reach,0,sizeof reach);startup.reachability=&reach;startup.reachability_capacity=1;
  memset(access,0,sizeof access);startup.control_access=access;startup.control_access_capacity=4;memset(control_contexts,0,sizeof control_contexts);startup.control_contexts=control_contexts;startup.control_context_capacity=2;memset(&view,0,sizeof view);memset(controls,0,sizeof controls);child.views=&view;child.view_capacity=1;startup.controls=controls;startup.control_capacity=2;for(unsigned k=0;k<2;k++){controls[k].bytes=labels[k];controls[k].capacity=sizeof labels[k];}startup.children=&child;startup.children_capacity=1;startup.policies=&policy;startup.policy_capacity=1;startup.parses=&parse;startup.parse_capacity=1;
  char **original_environment=failure?environment:0;
  startup.controller_original=&controller;startup.argc=2;startup.argv=argv;startup.environment=original_environment;
  controller.entry.birth.cgroup_fd=controller.entry.birth.payload_fd=-1;
  result=sl_linux_acquire_public_startup(&startup);
  assert(startup.returned&&startup.actual_argv==argv&&startup.actual_environment==original_environment&&startup.actual_argc==2);
  assert(startup.actual_argv[1]==arg1&&(!original_environment||startup.actual_environment[0]==setting));
  assert(controller.entry.birth.environment==startup.actual_environment);
  assert(!controller.attempted&&!controller.entry.birth.attempted);
  assert(controller.entry.birth.cgroup_fd==-1&&controller.entry.birth.payload_fd==-1);
  assert(failure?result<0:result==0);assert(calls==(failure?failure:22));
  if(failure==2)assert(startup.installation.public_fd==19&&startup.installation.result==-13);
  if(failure==3)assert(startup.membership.fd==20&&startup.membership.result==-5);
  if(failure==4)assert(startup.capability_domain.fd==22&&startup.capability_domain.result==-6);
  if(failure==5)assert(startup.immutable.fd==21&&startup.immutable.result==-28);
  if(failure==7)assert(location.result==-8&&startup.location_count==1);
  if(failure==8)assert(child.result==-10&&startup.children_count==1);
  if(failure==9||failure==10)assert(controls[failure-9].result==-17&&startup.control_count==failure-8);
  if(failure==11)assert(policy.result==-11&&startup.policy_count==1&&!parse.attempted);
  if(failure==12)assert(parse.result==-12&&startup.parse_count==1&&parse.snapshot_original==&policy);
  if(failure==13)assert(startup.self_context.raw_query==-7&&startup.self_context_result==-14&&!context.self_attempted);
  if(failure==14)assert(context.self_original==&startup.self_context&&context.self_result==-15&&startup.context_count==1);
  if(failure==15)assert(domains.context_original==&context&&domains.result==-16&&startup.domain_count==1);
  if(failure==16||failure==17)assert(control_contexts[failure-16].control_result==-18&&startup.control_context_count==failure-15);
  if(failure>=18&&failure<=21)assert(access[failure-18].result==-19&&startup.control_access_count==failure-17);
  if(failure==22)assert(reach.result==-20&&startup.reachability_count==1);
  if(failure==6)assert(startup.mounts.result==-7&&startup.mounts.membership_original==&startup.membership);
  prior=calls;assert(sl_linux_acquire_public_startup(&startup)==-1&&calls==prior);
 }
 memset(&startup,0,sizeof(startup));memset(&controller,0,sizeof(controller));
 actual=&startup;calls=0;startup.controller_original=&controller;startup.installation.attempted=1;
 startup.installation.public_fd=31;startup.installation.result=-4;
 assert(sl_linux_acquire_public_startup(&startup)==-1&&!calls);
 assert(startup.installation.public_fd==31&&startup.installation.result==-4);
 return 0;
}
#endif