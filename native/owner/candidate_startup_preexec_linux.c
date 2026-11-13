#include "public_startup_linux.h"
#include <errno.h>
static int same_span(sl_candidate_span a,sl_candidate_span b){if(!a.bytes||!b.bytes||a.extent!=b.extent)return 0;for(size_t i=0;i<a.extent;i++)if(a.bytes[i]!=b.bytes[i])return 0;return 1;}
long sl_linux_bind_controller_preexec(sl_linux_public_startup *s){
 if(!s||s->candidate_preexec_attempted)return -EINVAL;s->candidate_preexec_attempted=1;s->candidate_preexec_result=-EINVAL;
 s->actual_candidate_preexec=s->candidate_preexec;s->actual_candidate_preexec_capacity=s->candidate_preexec_capacity;
 sl_linux_public_controller *c=s->controller_original;
 if(!c||c->startup_original!=s||c->candidate_preexec_original||!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->actual_candidate_preexec||!s->actual_policy_capacity||s->actual_candidate_preexec_capacity!=s->actual_policy_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_policy_capacity)goto done;
 s->candidate_preexec_result=sl_linux_interpret_control_roster(&s->control_roster);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_bind_startup_candidate_roles(s);if(s->candidate_preexec_result)goto done;
 if(!s->genfs_correspondences||s->genfs_correspondence_capacity!=s->actual_policy_capacity||!s->actual_candidate_policies||s->actual_candidate_policy_capacity!=s->actual_policy_capacity){s->candidate_preexec_result=-EINVAL;goto done;}
 for(unsigned long i=0;i<s->genfs_correspondence_capacity;i++)if(s->genfs_correspondences[i].attempted||s->genfs_correspondences[i].policy_original!=&s->actual_candidate_policies[i]){s->candidate_preexec_result=-EINVAL;goto done;}
 for(unsigned long i=0;i<s->mounts.selinux_count;i++){s->candidate_preexec_result=sl_linux_interpret_genfs_correspondence(&s->genfs_correspondences[i]);if(s->candidate_preexec_result)goto done;}
 s->candidate_preexec_result=sl_linux_interpret_startup_migration_routes(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_startup_candidate_routes(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_startup_candidate_ptrace(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_startup_candidate_owner_entry(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_startup_candidate_file_entry(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_acquire_archive_directory(&s->archive_directory);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_archive_precreation(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_create_archive_files(&s->archive_files);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_acquire_archive_read_view(&s->final_reader);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_observe_archive_labels(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_archive_contexts(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_interpret_archive_accesses(s);if(s->candidate_preexec_result)goto done;
 s->candidate_preexec_result=sl_linux_bind_candidate_target(&s->candidate_target);if(s->candidate_preexec_result)goto done;
 s->initial_family.target_original=&s->candidate_target;s->initial_family_result=sl_linux_observe_family_events(&s->initial_family);
 if(s->initial_family_result){s->candidate_preexec_result=s->initial_family_result;goto done;}
 if(!s->initial_family.framing_complete||s->initial_family.populated||s->initial_family.frozen){s->candidate_preexec_result=-EINVAL;goto done;}
 s->candidate_preexec_result=-EINVAL;
 if(!s->actual_contexts||!s->actual_candidate_payloads||!s->actual_parses||s->actual_parse_capacity!=s->actual_policy_capacity||s->actual_context_capacity!=s->actual_policy_capacity||s->actual_candidate_payload_capacity!=s->actual_policy_capacity)goto done;
 for(unsigned long i=0;i<s->actual_candidate_preexec_capacity;i++){
  sl_linux_candidate_preexec *p=&s->actual_candidate_preexec[i];
  if(p->attempted||p->shared||p->bootstrap_original||p->payload_original!=&s->actual_candidate_payloads[i]||p->owner_original!=&s->actual_contexts[i]||p->child_context.parse_original!=&s->actual_parses[i])goto done;
 }
 /* All observed policy paths were individually digested/interpreted above.
  * Selecting backing from the first exact-equivalent meaning does not grant
  * path authority or discard another observation. No external index selects. */
 for(unsigned long i=1;i<s->mounts.selinux_count;i++){
  sl_linux_candidate_payload_context *a=&s->actual_candidate_payloads[0],*b=&s->actual_candidate_payloads[i];
  if(!same_span(a->desired_original,b->desired_original)||a->context.user!=b->context.user||a->context.role!=b->context.role||a->context.type!=b->context.type)goto done;
 }
 s->candidate_preexec_selected=0;c->candidate_preexec_original=&s->actual_candidate_preexec[0];s->candidate_preexec_result=0;
 done:s->candidate_preexec_returned=1;return s->candidate_preexec_result;
 /* This binds a preexec source owner, not full profile or gate admission. */
}
