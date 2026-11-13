#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <limits.h>
long sl_linux_acquire_public_startup(sl_linux_public_startup *startup){
 sl_linux_entry *entry;unsigned long i,j,control_at=0;
 if(!startup||startup->attempted)return -1;
 startup->attempted=1;startup->result=-1;
 /* These references are the actual C startup inputs, not reconstructed argv
  * or a serialized replacement for their original storage/aliases. */
 startup->actual_argc=startup->argc;
 startup->actual_argv=startup->argv;
 startup->actual_environment=startup->environment;
 startup->actual_candidate=startup->candidate_original;
 startup->prepared_controller=startup->controller_original;
 startup->stage=SL_STARTUP_ROOTS;
 startup->actual_locations=startup->locations;startup->actual_location_capacity=startup->location_capacity;
 startup->actual_children=startup->children;startup->actual_children_capacity=startup->children_capacity;
 startup->actual_controls=startup->controls;startup->actual_control_capacity=startup->control_capacity;
 if(!startup->actual_controls||!startup->actual_control_capacity)goto done;
 startup->actual_policies=startup->policies;startup->actual_policy_capacity=startup->policy_capacity;
 startup->actual_parses=startup->parses;startup->actual_parse_capacity=startup->parse_capacity;
 startup->actual_exec_contexts=startup->exec_contexts;startup->actual_exec_context_capacity=startup->exec_context_capacity;
 if(!startup->actual_exec_contexts||!startup->actual_policy_capacity||startup->actual_policy_capacity>~0UL/2||startup->actual_exec_context_capacity!=2*startup->actual_policy_capacity)goto done;
 for(i=0;i<startup->actual_exec_context_capacity;i++)if(startup->actual_exec_contexts[i].attempted||startup->actual_exec_contexts[i].exec_attempted||startup->actual_exec_contexts[i].parse_original||startup->actual_exec_contexts[i].exec_original)goto done;
 startup->actual_contexts=startup->contexts;startup->actual_context_capacity=startup->context_capacity;
 startup->actual_control_contexts=startup->control_contexts;startup->actual_control_context_capacity=startup->control_context_capacity;
 if(!startup->actual_control_contexts||!startup->actual_policy_capacity||startup->actual_control_capacity>~0UL/startup->actual_policy_capacity||startup->actual_control_context_capacity!=startup->actual_control_capacity*startup->actual_policy_capacity)goto done;
 for(i=0;i<startup->actual_control_context_capacity;i++){
  sl_linux_selinux_context *context=&startup->actual_control_contexts[i];
  if(context->attempted||context->control_attempted||context->parse_original||context->control_original)goto done;
 }
 startup->actual_control_access=startup->control_access;startup->actual_control_access_capacity=startup->control_access_capacity;
 if(!startup->actual_control_access||startup->actual_control_context_capacity>~0UL/2||startup->actual_control_access_capacity!=startup->actual_control_context_capacity*2)goto done;
 for(i=0;i<startup->actual_control_access_capacity;i++)if(startup->actual_control_access[i].attempted||startup->actual_control_access[i].context_original)goto done;
 startup->actual_reachability=startup->reachability;startup->actual_reachability_capacity=startup->reachability_capacity;
 if(!startup->actual_reachability||startup->actual_reachability_capacity!=startup->actual_context_capacity)goto done;
 for(i=0;i<startup->actual_reachability_capacity;i++)if(startup->actual_reachability[i].attempted||startup->actual_reachability[i].context_original)goto done;
 startup->actual_domains=startup->domains;startup->actual_domain_capacity=startup->domain_capacity;
 if(!startup->actual_domains||startup->actual_domain_capacity!=startup->actual_context_capacity)goto done;
 for(i=0;i<startup->actual_domain_capacity;i++)if(startup->actual_domains[i].attempted||startup->actual_domains[i].context_original)goto done;
 if(!startup->actual_contexts||startup->actual_context_capacity!=startup->actual_parse_capacity||startup->self_context.attempted)goto done;
 for(i=0;i<startup->actual_context_capacity;i++)if(startup->actual_contexts[i].attempted||startup->actual_contexts[i].self_attempted||startup->actual_contexts[i].parse_original)goto done;
 if(!startup->actual_parses||startup->actual_parse_capacity!=startup->actual_policy_capacity)goto done;
 for(i=0;i<startup->actual_parse_capacity;i++)if(startup->actual_parses[i].attempted||startup->actual_parses[i].snapshot_original)goto done;
 if(!startup->actual_policies||!startup->actual_policy_capacity)goto done;
 for(i=0;i<startup->actual_policy_capacity;i++)if(startup->actual_policies[i].attempted||startup->actual_policies[i].mounts_original)goto done;
 if(!startup->actual_children||startup->actual_children_capacity!=startup->actual_location_capacity)goto done;
 if(!startup->prepared_controller||!startup->actual_locations||!startup->actual_location_capacity||startup->actual_argc<0||
    (startup->actual_argc&&!startup->actual_argv)||startup->returned||
    startup->installation.attempted||startup->membership.attempted||startup->mounts.attempted||startup->mounts.membership_original||startup->immutable.attempted||startup->capability_domain.attempted||
    startup->installation.entry_original||startup->membership.entry_original||
    startup->immutable.installation_original||startup->capability_domain.entry_original)goto done;
 for(i=0;i<startup->actual_location_capacity;i++){
  sl_linux_mount_location *location=&startup->actual_locations[i];
  if(location->attempted||location->mounts_original||startup->actual_children[i].attempted||startup->actual_children[i].location_original)goto done;
 }
 entry=&startup->prepared_controller->entry;
 startup->prepared_controller->private_arguments.public_argc=startup->actual_argc;startup->prepared_controller->private_arguments.public_argv=startup->actual_argv;
 if(startup->prepared_controller->attempted||entry->public_streams_attempted||
    entry->attempted||entry->birth.attempted||startup->prepared_controller->capability_domain_original)goto done;
 entry->birth.environment=startup->actual_environment;
 /* Bind all actual acquisition owners before the first acquisition. No
  * returned owner is copied into a replacement controller or payload cell. */
 startup->public_label.installation_original=&startup->installation;startup->payload_label.payload_original=&startup->immutable;
 for(i=0;i<startup->actual_exec_context_capacity;i++){
  startup->actual_exec_contexts[i].parse_original=&startup->actual_parses[i%startup->actual_policy_capacity];
  startup->actual_exec_contexts[i].exec_original=i<startup->actual_policy_capacity?&startup->public_label:&startup->payload_label;
 }
 startup->installation.entry_original=entry;
 startup->membership.entry_original=entry;
 startup->mounts.membership_original=&startup->membership;
 for(i=0;i<startup->actual_location_capacity;i++){startup->actual_locations[i].mounts_original=&startup->mounts;startup->actual_locations[i].row_index=i;}
 for(i=0;i<startup->actual_children_capacity;i++)startup->actual_children[i].location_original=&startup->actual_locations[i];
 for(i=0;i<startup->actual_policy_capacity;i++){startup->actual_policies[i].mounts_original=&startup->mounts;startup->actual_policies[i].row_index=i;}
 for(i=0;i<startup->actual_parse_capacity;i++)startup->actual_parses[i].snapshot_original=&startup->actual_policies[i];
 for(i=0;i<startup->actual_context_capacity;i++)startup->actual_contexts[i].parse_original=&startup->actual_parses[i];
 for(i=0;i<startup->actual_domain_capacity;i++)startup->actual_domains[i].context_original=&startup->actual_contexts[i];
 for(i=0;i<startup->actual_reachability_capacity;i++)startup->actual_reachability[i].context_original=&startup->actual_contexts[i];
 for(i=0;i<startup->actual_children_capacity;i++){
  sl_linux_cgroup_children *child=&startup->actual_children[i];
  if(!child->views||!child->view_capacity)goto done;
  for(j=0;j<child->view_capacity;j++)for(unsigned kind=SL_CGROUP_PROCS;kind<=SL_CGROUP_THREADS;kind++){
   if(control_at==startup->actual_control_capacity)goto done;
   sl_linux_cgroup_control_label *control=&startup->actual_controls[control_at++];
   if(control->attempted||control->children_original||control->view_original)goto done;
   control->children_original=child;control->view_original=&child->views[j];control->kind=kind;
  }
 }
 if(control_at!=startup->actual_control_capacity)goto done;
 for(i=0;i<startup->actual_control_capacity;i++)for(j=0;j<startup->actual_policy_capacity;j++){
  sl_linux_selinux_context *context=&startup->actual_control_contexts[i*startup->actual_policy_capacity+j];
  context->control_original=&startup->actual_controls[i];context->parse_original=&startup->actual_parses[j];
 }
 for(i=0;i<startup->actual_control_access_capacity;i++){startup->actual_control_access[i].context_original=&startup->actual_control_contexts[i/2];startup->actual_control_access[i].kind_original=(i%2)?SL_CONTROL_RELABEL_FROM:SL_CONTROL_WRITE_APPEND;}
 startup->capability_domain.entry_original=entry;
 startup->prepared_controller->capability_domain_original=&startup->capability_domain;
 startup->immutable.installation_original=&startup->installation;
 startup->stage=SL_STARTUP_STDIO;
 startup->stdio_result=sl_linux_prepare_public_streams(entry);
 startup->result=startup->stdio_result;if(startup->result)goto done;
 startup->stage=SL_STARTUP_INSTALLATION;
 startup->installation_result=sl_linux_acquire_local_installation(&startup->installation);
 startup->result=startup->installation_result;if(startup->result)goto done;
 startup->stage=SL_STARTUP_MEMBERSHIP;
 startup->membership_result=sl_linux_observe_local_membership(&startup->membership);
 startup->result=startup->membership_result;if(startup->result)goto done;
 startup->stage=SL_STARTUP_CAP_DOMAIN;
 startup->capability_result=sl_linux_observe_capability_domain(&startup->capability_domain);
 startup->result=startup->capability_result;if(startup->result)goto done;
 startup->stage=SL_STARTUP_IMMUTABLE;
 startup->immutable_result=sl_linux_prepare_immutable_payload(&startup->immutable);
 startup->result=startup->immutable_result;if(startup->result)goto done;
 startup->stage=SL_STARTUP_EXEC_LABELS;
 startup->public_label_result=sl_linux_observe_exec_label(&startup->public_label);
 startup->payload_label_result=sl_linux_observe_exec_label(&startup->payload_label);
 if(startup->public_label_result||startup->payload_label_result){startup->result=startup->public_label_result?startup->public_label_result:startup->payload_label_result;goto done;}
 startup->stage=SL_STARTUP_MOUNTS;
 startup->mounts_result=sl_linux_observe_local_mounts(&startup->mounts);
 startup->result=startup->mounts_result;if(startup->result)goto done;
 startup->stage=SL_STARTUP_LOCATIONS;
 if(startup->mounts.row_count>startup->actual_location_capacity){startup->result=-1;goto done;}
 for(i=0;i<startup->mounts.row_count;i++){
  startup->result=sl_linux_acquire_mount_location(&startup->actual_locations[i]);startup->location_count=i+1;
  if(startup->result<0)goto done;
  if(startup->result>0&&!(startup->result==1&&startup->actual_locations[i].outside_mount)){startup->result=-1;goto done;}
 }
 startup->stage=SL_STARTUP_CHILDREN;
 for(i=0;i<startup->mounts.row_count;i++){
  if(startup->actual_locations[i].outside_mount)continue;
  startup->result=sl_linux_acquire_cgroup_children(&startup->actual_children[i]);startup->children_count=i+1;
  if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_CONTROL_LABELS;
 for(i=0;i<startup->actual_control_capacity;i++){
  sl_linux_cgroup_control_label *control=&startup->actual_controls[i];sl_linux_cgroup_children *child=control->children_original;
  if(child->location_original->outside_mount)continue;
  if(!child->returned||child->result||!child->eof||!child->actual_views||child->view_count>child->actual_view_capacity){startup->result=-1;goto done;}
  unsigned found=0;for(j=0;j<child->view_count;j++)if(&child->actual_views[j]==control->view_original){found=1;break;}
  if(!found||control->view_original->dot||control->view_original->not_directory)continue;
  startup->result=sl_linux_acquire_cgroup_control_label(control);startup->control_count=i+1;
  if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_SELINUX_SNAPSHOTS;
 if(!startup->mounts.selinux_count||startup->mounts.selinux_count>startup->actual_policy_capacity){startup->result=-1;goto done;}
 for(i=0;i<startup->mounts.selinux_count;i++){
  startup->result=sl_linux_acquire_selinux_snapshot(&startup->actual_policies[i]);startup->policy_count=i+1;
  if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_POLICY_PARSE;
 for(i=0;i<startup->mounts.selinux_count;i++){
  startup->result=sl_linux_parse_policy_snapshot(&startup->actual_parses[i]);startup->parse_count=i+1;
  if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_SELF_CONTEXT;
 startup->self_context_result=sl_linux_observe_selinux_self(&startup->self_context);startup->result=startup->self_context_result;
 if(startup->result)goto done;
 startup->stage=SL_STARTUP_CONTEXT_PARSE;
 for(i=0;i<startup->mounts.selinux_count;i++){
  startup->result=sl_linux_interpret_selinux_self(&startup->actual_contexts[i],&startup->self_context);startup->context_count=i+1;
  if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_EXEC_CONTEXTS;
 for(i=0;i<2;i++)for(j=0;j<startup->mounts.selinux_count;j++){
  unsigned long at=i*startup->actual_policy_capacity+j;
  startup->result=sl_linux_interpret_exec_context(&startup->actual_exec_contexts[at]);startup->exec_context_count=at+1;if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_BOUND_DOMAINS;
 for(i=0;i<startup->mounts.selinux_count;i++){
  startup->result=sl_linux_interpret_bound_domains(&startup->actual_domains[i]);startup->domain_count=i+1;
  if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_CONTROL_CONTEXTS;
 for(i=0;i<startup->actual_control_capacity;i++){
  sl_linux_cgroup_control_label *control=&startup->actual_controls[i];
  if(!control->attempted)continue;
  if(!control->returned||control->result||!control->framing_complete){startup->result=-1;goto done;}
  for(j=0;j<startup->mounts.selinux_count;j++){
   unsigned long at=i*startup->actual_policy_capacity+j;
   startup->result=sl_linux_interpret_control_context(&startup->actual_control_contexts[at]);startup->control_context_count=at+1;
   if(startup->result)goto done;
  }
 }
 for(i=0;i<startup->mounts.selinux_count;i++){
  if(!startup->migration_labels||startup->migration_label_capacity!=startup->actual_policy_capacity){startup->result=-1;goto done;}
  startup->result=sl_linux_interpret_migration_labels(&startup->migration_labels[i]);if(startup->result)goto done;
 }
 for(i=0;i<startup->mounts.selinux_count;i++)for(j=0;j<3;j++){
  if(startup->actual_policy_capacity>ULONG_MAX/3||!startup->migration_access||startup->migration_access_capacity!=3*startup->actual_policy_capacity){startup->result=-1;goto done;}
  startup->result=sl_linux_interpret_migration_access(&startup->migration_access[3*i+j]);if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_CONTROL_ACCESS;
 for(i=0;i<startup->actual_control_access_capacity;i++){
  sl_linux_control_access *access=&startup->actual_control_access[i];
  if(!access->context_original->control_attempted)continue;
  startup->result=sl_linux_interpret_control_access(access);startup->control_access_count=i+1;if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_REACHABILITY;
 for(i=0;i<startup->mounts.selinux_count;i++){
  startup->result=sl_linux_interpret_context_reachability(&startup->actual_reachability[i]);startup->reachability_count=i+1;if(startup->result)goto done;
 }
 startup->stage=SL_STARTUP_ACQUIRED;
 /* Acquisition is not native eligibility. In particular birth.cgroup_fd and
  * birth.payload_fd, private argv and the gate remain untouched. The exact
  * original environment was bound above; that binding grants no eligibility.
  * Every partial owner remains in this SAME startup object on failure. */
 startup->result=0;
done:
 startup->returned=1;return startup->result;
}
#endif
