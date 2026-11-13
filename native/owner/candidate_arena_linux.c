#if defined(__linux__) && defined(__x86_64__)
#include "candidate_arena_linux.h"
#include "public_main_linux.h"
#include "outer_birth_linux.h"
#include "outer_terminal_linux.h"
#include "outer_supervisor_linux.h"
#include "failed_parent_identity_linux.h"
#include "failed_archive_linux.h"
#include "failed_parent_directory_linux.h"
#include "failed_parent_file_linux.h"
#include <sys/syscall.h>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>
static int arena_product(unsigned long a,unsigned long b,unsigned long *out){
 if(!a||!b||a>ULONG_MAX/b)return -1;*out=a*b;return 0;
}
static int arena_reserve(sl_linux_candidate_layout *layout,sl_linux_arena_region *region,
 unsigned long count,unsigned long unit,unsigned long alignment){
 unsigned long bytes,padding;
 if(arena_product(count,unit,&bytes)||!alignment)return -1;
 padding=(alignment-layout->bytes%alignment)%alignment;
 if(padding>ULONG_MAX-layout->bytes||bytes>ULONG_MAX-layout->bytes-padding)return -1;
 region->offset=layout->bytes+padding;region->bytes=bytes;layout->bytes=region->offset+bytes;return 0;
}
#define RESERVE(region,count,type) do{if(arena_reserve(l,&l->region,(count),sizeof(type),_Alignof(type)))return -1;}while(0)
#define BYTES(region,count) RESERVE(region,count,unsigned char)
#define PRODUCT(a,b,out) do{if(arena_product((a),(b),&(out)))return -1;}while(0)
static long arena_layout(sl_linux_candidate_layout *l,const sl_linux_candidate_quantities *q,unsigned long candidate_bytes,unsigned long protected_rows){
 unsigned long i,n,m,w;
 RESERVE(public_main_owner,1,sl_linux_public_main_owner);
 RESERVE(public_input_source,1,sl_linux_public_input_source);
 RESERVE(public_inputs,1,sl_linux_public_inputs);
 RESERVE(public_argument_views,q->public_argument_count,sl_linux_public_string);
 RESERVE(public_environment_views,q->public_environment_count,sl_linux_public_string);
 if(!q->public_environment_bytes)return -1;
 RESERVE(outer_birth,1,sl_linux_outer_birth);
 RESERVE(outer_continuation,1,sl_linux_outer_continuation);
 RESERVE(outer_supervisor,1,sl_linux_outer_supervisor);
 RESERVE(failed_parent_identity,1,sl_linux_failed_parent_identity);
 RESERVE(failed_archive_header,1,sl_linux_failed_archive_header);
 RESERVE(failed_parent_directory,1,sl_linux_failed_parent_directory);
 RESERVE(failed_parent_writer,1,sl_linux_failed_parent_file);
 RESERVE(failed_parent_reader,1,sl_linux_failed_parent_file);
 RESERVE(failed_parent_path,q->archive_path_bytes,unsigned char);
 RESERVE(failed_parent_declarations,q->archive_declarations,sl_linux_archive_declaration);
 RESERVE(outer_turns,q->outer_observations,sl_linux_outer_turn);
 RESERVE(outer_observations,q->outer_observations,sl_linux_outer_terminal);
 RESERVE(deadline_sources,q->deadline_roots,sl_linux_deadline_source);
 RESERVE(deadlines,q->deadline_roots,sl_linux_deadline);
 RESERVE(deadline_sequences,q->deadline_roots,sl_linux_deadline_sequence);
 PRODUCT(q->deadline_roots,q->deadline_observations,n);
 RESERVE(deadline_observations,n,sl_linux_deadline_observation);
 PRODUCT(q->deadline_roots,q->deadline_polls,n);
 RESERVE(deadline_polls,n,sl_linux_deadline_poll);
 PRODUCT(n,q->deadline_poll_fds,m);
 RESERVE(deadline_poll_descriptors,m,struct pollfd);
 RESERVE(deadline_poll_outputs,m,struct pollfd);
 RESERVE(creator_file,1,sl_linux_creator_file);
 BYTES(creator_file_body,q->creator_file_bytes);
 RESERVE(creator_file_reads,q->creator_file_reads,sl_linux_creator_read);
 RESERVE(creator_elf,1,sl_linux_creator_elf);
 RESERVE(creator_elf_programs,q->creator_file_bytes/56,sl_linux_elf_program);
 if(!q->credential_status_bytes||!q->credential_status_attempts||!q->credential_group_capacity||q->credential_group_capacity>INT_MAX)return -1;
 /* Reserve every nested anonymous output region in the single prebirth map.
  * Later child binders consume these ranges; no credential or LSM query is
  * performed here and no nested native mapping result is manufactured. */
 RESERVE(bootstrap_backing,1,sl_linux_bootstrap);
 RESERVE(fd_action_backing,q->fd_actions,sl_linux_fd_action);
 for(i=0;i<2;i++){
  BYTES(credential_status_backing[i],q->credential_status_bytes);
  RESERVE(credential_read_backing[i],q->credential_status_attempts,sl_linux_call);
  RESERVE(credential_group_backing[i],q->credential_group_capacity,unsigned);
 }
 /* A selected desired context is an original span within candidate source.
  * This source envelope includes the NUL added by the actual child producer. */
 if(!candidate_bytes||
    q->self_context_bytes<sizeof(sl_linux_lsm_header)+2||q->self_context_bytes>UINT32_MAX)return -1;
 /* The existing producer accepts only rounded input <= UINT32_MAX. Larger
  * candidate files do not require a larger input region or become refused. */
 unsigned long preexec_input=UINT32_MAX&~7UL;
 if(candidate_bytes<=preexec_input-sizeof(sl_linux_lsm_header)-8)
  preexec_input=(sizeof(sl_linux_lsm_header)+candidate_bytes+8)&~7UL;
 unsigned long preexec_query=(q->self_context_bytes+7)&~7UL;
 if(preexec_input>UINT32_MAX||preexec_query>(ULONG_MAX-sizeof(sl_linux_candidate_preexec_shared)-preexec_input)/3)return -1;
 l->preexec_shared_stride=sizeof(sl_linux_candidate_preexec_shared)+3*preexec_query+preexec_input;
 /* Round each policy's starting address independently for its typed header. */
 unsigned long preexec_padding=(_Alignof(sl_linux_candidate_preexec_shared)-l->preexec_shared_stride%_Alignof(sl_linux_candidate_preexec_shared))%_Alignof(sl_linux_candidate_preexec_shared);
 if(preexec_padding>ULONG_MAX-l->preexec_shared_stride)return -1;
 l->preexec_shared_stride+=preexec_padding;l->preexec_source_extent=candidate_bytes;
 if(arena_reserve(l,&l->preexec_shared_backing,q->selinux_rows,l->preexec_shared_stride,_Alignof(sl_linux_candidate_preexec_shared)))return -1;
 if(!q->public_argument_count||q->public_argument_count>INT_MAX||q->public_argument_count>ULONG_MAX-8||!q->public_argument_bytes)return -1;
 if(!q->security_label_bytes||q->security_label_bytes>UINT_MAX||!q->terminal_calls||q->terminal_calls>=ULONG_MAX||
    q->terminal_calls>LONG_MAX||q->requests!=q->terminal_calls+1||q->body_bytes>LONG_MAX)return -1;
 RESERVE(private_arguments,q->public_argument_count+8,char*);RESERVE(argument_lengths,q->public_argument_count,unsigned long);
 PRODUCT(2,q->security_label_bytes,n);BYTES(custody_peer_labels,n);
 PRODUCT(3,q->security_label_bytes,n);BYTES(archive_labels,n);
 RESERVE(startup,1,sl_linux_public_startup);RESERVE(controller,1,sl_linux_public_controller);
 if(!q->scheduler_interval_ns||q->services>UINT32_MAX||q->requests>UINT32_MAX)return -1;
 w=8; /* three raw, three forwarding, collector and independent reader terminals */
 for(i=0;i<3;i++){if(q->raw_frames[i]>UINT32_MAX-w)return -1;w+=q->raw_frames[i];}
 if(q->services>UINT32_MAX-w)return -1;w+=q->services;
 if(q->requests>UINT32_MAX-w)return -1;w+=q->requests;
 if(q->services>ULONG_MAX/2)return -1;
 RESERVE(schedule_wakes,w,sl_linux_schedule_wake);RESERVE(schedule_waits,2*q->services,sl_linux_schedule_wait);
 BYTES(archive_path,q->archive_path_bytes);RESERVE(archive_declarations,q->archive_declarations,sl_linux_archive_declaration);RESERVE(archive_statuses,q->selinux_rows,sl_linux_selinux_status_sample);RESERVE(archive_file_statuses,q->selinux_rows,sl_linux_selinux_status_sample);
 RESERVE(readers,3,sl_linux_capture_worker);RESERVE(forwarders,3,sl_linux_forward_worker);
 for(i=0;i<3;i++){
  RESERVE(raw_frames[i],q->raw_frames[i],sl_linux_capture_frame);
  PRODUCT(q->raw_frames[i],q->public_writes[i],n);RESERVE(public_writes[i],n,sl_linux_call);
  RESERVE(waits[i],q->forward_waits[i],sl_linux_forward_wait);
  RESERVE(iterations[i],q->forward_iterations[i],sl_linux_forward_iteration);
  RESERVE(publications[i],q->raw_frames[i],sl_linux_notification);
 }
 RESERVE(services,q->services,sl_linux_service_observation);PRODUCT(q->services,3,n);
 RESERVE(raw_joins,n,sl_linux_reader_join);RESERVE(releases,n,sl_linux_forward_release);RESERVE(forward_joins,n,sl_linux_forward_join);
 RESERVE(collector_joins,q->collector_joins,sl_linux_collector_join);
 RESERVE(requests,q->requests,sl_linux_terminal_request);
 PRODUCT(q->requests,q->prefix_attempts,n);RESERVE(prefix_calls,n,sl_linux_call);
 PRODUCT(q->requests,q->terminal_reply_attempts,n);RESERVE(reply_calls,n,sl_linux_call);
 PRODUCT(q->requests,q->input_frames,n);RESERVE(input_frames,n,sl_linux_input_frame);
 PRODUCT(n,q->input_steps,m);RESERVE(input_steps,m,sl_linux_input_step);
 BYTES(body,q->body_bytes);BYTES(readback,q->body_bytes);
 for(i=0;i<5;i++)RESERVE(custody_calls[i],q->custody_attempts[i],sl_linux_call);
 /* Complete capture-component reservation, before mapping or native effects.
  * Actual finite IO rosters are explicit inputs, never guessed byte budgets. */
 unsigned long capture_frames=0,capture_calls=0,capture_records;
 for(i=0;i<3;i++){
  if(q->raw_frames[i]>ULONG_MAX-capture_frames||q->public_writes[i]>ULONG_MAX-5)return -1;capture_frames+=q->raw_frames[i];
  PRODUCT(q->raw_frames[i],q->public_writes[i]+5,n);if(n>ULONG_MAX-capture_calls)return -1;capture_calls+=n;
 }
 /* Installation: 11 fixed constructor calls + at most10 low-FD calls,
  * 7 typed root/lift states,7 ABI outputs,3 original path spans. */
 if(capture_frames>ULONG_MAX-87||capture_calls>ULONG_MAX-capture_frames-87)return -1;capture_records=87+capture_frames+capture_calls;
 if(q->membership_frames>(ULONG_MAX-8)/2)return -1;n=8+2*q->membership_frames;
 if(n>ULONG_MAX-capture_records)return -1;capture_records+=n;
 /* Nine possible containing ranges: bootstrap, fd actions, six credential
  * ranges and selected preexec. Each retains mapping LE80 + range LE80. */
 if(capture_records>ULONG_MAX-18)return -1;capture_records+=18;
 /* Mount constructor: two states, at most five fixed calls, procfs output,
  * exact raw body and one original call per admitted read occurrence. */
 if(q->mountinfo_attempts>ULONG_MAX-9)return -1;n=9+q->mountinfo_attempts;
 if(n>ULONG_MAX-capture_records)return -1;capture_records+=n;
 /* Target acquisition: four typed states, ten possible calls, six ABI bodies. */
 if(capture_records>ULONG_MAX-20)return -1;capture_records+=20;
 if(q->policy_frames>(ULONG_MAX-35)/2)return -1;n=35+2*q->policy_frames;PRODUCT(q->selinux_rows,n,n);
 if(n>ULONG_MAX-capture_records)return -1;capture_records+=n;
 RESERVE(final_records,capture_records,sl_linux_final_record);
 if(capture_frames>ULONG_MAX/16384)return -1;m=capture_frames*16384;
 if(capture_records>ULONG_MAX/32||capture_records*32>ULONG_MAX-m)return -1;m+=capture_records*32;
 if(capture_calls>ULONG_MAX/80||capture_calls*80>ULONG_MAX-m)return -1;m+=capture_calls*80;
 if(m>ULONG_MAX-18*80)return -1;m+=18*80;
 unsigned long terminal_bytes=80+sizeof(siginfo_t)+sizeof(struct rusage)+22*(80+1);
 if(terminal_bytes>ULONG_MAX-m)return -1;m+=terminal_bytes;
 if(!candidate_bytes||candidate_bytes>ULONG_MAX-m)return -1;m+=candidate_bytes;
 unsigned long installation_bytes=21*80+7*80+sizeof(struct statfs)+4*sizeof(struct statx)+2*sizeof(struct open_how);
 if(q->directory_path>ULONG_MAX/2||q->executable_path>ULONG_MAX-2*q->directory_path)return -1;
 n=q->executable_path+2*q->directory_path;
 if(installation_bytes>ULONG_MAX-n)return -1;installation_bytes+=n;
 if(installation_bytes>ULONG_MAX-m)return -1;m+=installation_bytes;
 unsigned long membership_bytes=6*80+sizeof(struct statfs);
 if(q->membership_path>ULONG_MAX-membership_bytes)return -1;membership_bytes+=q->membership_path;
 if(q->membership_bytes>ULONG_MAX-80)return -1;PRODUCT(q->membership_frames,q->membership_bytes+80,n);
 if(n>ULONG_MAX-membership_bytes)return -1;membership_bytes+=n;
 if(membership_bytes>ULONG_MAX-m)return -1;m+=membership_bytes;
 unsigned long mount_bytes=7*80+sizeof(struct statfs);
 if(q->mountinfo_attempts>ULONG_MAX/80)return -1;n=q->mountinfo_attempts*80;
 if(n>ULONG_MAX-mount_bytes)return -1;mount_bytes+=n;
 if(q->mountinfo_bytes>ULONG_MAX-mount_bytes)return -1;mount_bytes+=q->mountinfo_bytes;
 if(mount_bytes>ULONG_MAX-m)return -1;m+=mount_bytes;
 unsigned long target_bytes=14*80+2*sizeof(struct statfs)+2*sizeof(struct statx)+2*sizeof(struct open_how);
 if(target_bytes>ULONG_MAX-m)return -1;m+=target_bytes;
 unsigned long policy_bytes=25*80+3*sizeof(struct statfs)+3*sizeof(struct statx)+3*sizeof(struct open_how);
 if(q->selinux_path>ULONG_MAX-policy_bytes)return -1;policy_bytes+=q->selinux_path;
 if(q->policy_frame_bytes>ULONG_MAX-80)return -1;PRODUCT(q->policy_frames,q->policy_frame_bytes+80,n);
 if(n>ULONG_MAX-policy_bytes)return -1;policy_bytes+=n;PRODUCT(q->selinux_rows,policy_bytes,n);
 if(n>ULONG_MAX-m)return -1;m+=n;
 if(q->body_bytes>ULONG_MAX-m)return -1;m+=q->body_bytes;if(m>LONG_MAX)return -1;
 BYTES(final_readback,m);for(i=0;i<2;i++)RESERVE(final_calls[i],q->final_io_attempts[i],sl_linux_call);
 BYTES(independent_readback,m);RESERVE(independent_reads,q->final_io_attempts[1],sl_linux_call);RESERVE(independent_joins,q->services,sl_linux_archive_reader_join);
 RESERVE(scan_frames,q->scan_frames,sl_linux_fd_scan_frame);PRODUCT(q->scan_frames,q->scan_bytes,n);BYTES(scan_bytes,n);
 RESERVE(fd_views,q->fd_views,sl_linux_fd_view);RESERVE(fd_actions,q->fd_actions,sl_linux_fd_action);
 RESERVE(membership_frames,q->membership_frames,sl_linux_membership_frame);
 PRODUCT(q->membership_frames,q->membership_bytes,n);BYTES(membership_bytes,n);BYTES(membership_path,q->membership_path);
 BYTES(mountinfo_bytes,q->mountinfo_bytes);RESERVE(mountinfo_reads,q->mountinfo_attempts,sl_linux_call);RESERVE(mountinfo_rows,q->mountinfo_rows,sl_linux_mount_row);
 RESERVE(mount_locations,q->mountinfo_rows,sl_linux_mount_location);PRODUCT(q->mountinfo_rows,q->mount_location_bytes,n);
 BYTES(mount_roots,n);BYTES(mount_points,n);BYTES(mount_relatives,n);
 RESERVE(selinux_rows,q->selinux_rows,sl_linux_mount_row);RESERVE(policies,q->selinux_rows,sl_linux_selinux_snapshot);
 PRODUCT(q->selinux_rows,q->selinux_path,n);BYTES(policy_paths,n);
 PRODUCT(q->selinux_rows,q->policy_frames,n);RESERVE(policy_frames,n,sl_linux_policy_frame);
 PRODUCT(n,q->policy_frame_bytes,m);BYTES(policy_frame_bytes,m);
 PRODUCT(q->selinux_rows,q->policy_body,n);BYTES(policy_body,n);
 if(!q->diagnostic_format||q->parser_bytes%_Alignof(max_align_t))return -1;
 if(q->self_context_bytes<sizeof(sl_linux_lsm_header)+2||q->self_context_bytes>UINT32_MAX)return -1;
 if(arena_reserve(l,&l->self_context,q->self_context_bytes,1,_Alignof(sl_linux_lsm_header)))return -1;
 if(arena_reserve(l,&l->archive_fscreate,q->self_context_bytes,1,_Alignof(sl_linux_lsm_header)))return -1;
 if(q->domain_types>UINT16_MAX)return -1;
 if(!q->reach_strings)return -1;
 RESERVE(reachability,q->selinux_rows,sl_linux_context_reachability);
 PRODUCT(q->domain_types,q->domain_types,n);PRODUCT(n,q->selinux_rows,m);RESERVE(reach_edges,m,sl_selinux_transition_edge);
 PRODUCT(q->domain_types,2,n);PRODUCT(n,q->selinux_rows,m);RESERVE(reach_matches,m,unsigned);
 PRODUCT(q->selinux_rows,q->reach_visits,m);RESERVE(reach_visits,m,sl_selinux_rule_visit);
 PRODUCT(q->selinux_rows,q->reach_helpers,m);RESERVE(reach_helpers,m,sl_selinux_helper_visit);
 PRODUCT(q->selinux_rows,q->domain_types,n);RESERVE(reach_types,n,unsigned);
 PRODUCT(q->selinux_rows,q->reach_steps,n);RESERVE(reach_steps,n,sl_selinux_reach_step);
 RESERVE(candidate_payloads,q->selinux_rows,sl_linux_candidate_payload_context);
 PRODUCT(q->domain_types,q->domain_types,n);PRODUCT(n,q->selinux_rows,m);RESERVE(payload_edges,m,sl_selinux_transition_edge);
 PRODUCT(q->domain_types,2,n);PRODUCT(n,q->selinux_rows,m);RESERVE(payload_matches,m,unsigned);
 PRODUCT(q->selinux_rows,q->reach_visits,m);RESERVE(payload_visits,m,sl_selinux_rule_visit);
 PRODUCT(q->selinux_rows,q->reach_helpers,m);RESERVE(payload_helpers,m,sl_selinux_helper_visit);
 PRODUCT(q->selinux_rows,q->domain_types,n);RESERVE(payload_types,n,unsigned);
 PRODUCT(q->selinux_rows,q->reach_steps,n);RESERVE(payload_steps,n,sl_selinux_reach_step);
 RESERVE(domains,q->selinux_rows,sl_linux_bound_domains);
 PRODUCT(q->selinux_rows,q->domain_types,n);RESERVE(domain_entries,n,sl_linux_domain_entry);
 PRODUCT(q->selinux_rows,q->domain_steps,n);RESERVE(domain_steps,n,sl_linux_domain_step);
 PRODUCT(2,q->self_context_bytes,n);BYTES(exec_labels,n);
 PRODUCT(2,q->selinux_rows,n);RESERVE(candidate_socket_contexts,n,sl_linux_candidate_socket_context);
 PRODUCT(n,q->parser_bytes,m);if(arena_reserve(l,&l->socket_context_memory,m,1,_Alignof(max_align_t)))return -1;
 PRODUCT(n,q->parser_attempts,m);RESERVE(socket_context_attempts,m,sl_sepol_memory_attempt);
 PRODUCT(n,q->diagnostics,m);RESERVE(socket_context_diagnostics,m,sl_sepol_diagnostic);
 PRODUCT(m,q->diagnostic_arguments,n);RESERVE(socket_context_arguments,n,sl_sepol_argument);
 PRODUCT(m,q->diagnostic_bytes,n);BYTES(socket_context_outputs,n);
 PRODUCT(2,q->selinux_rows,n);RESERVE(exec_contexts,n,sl_linux_selinux_context);
 PRODUCT(n,q->parser_bytes,m);if(arena_reserve(l,&l->exec_context_memory,m,1,_Alignof(max_align_t)))return -1;
 PRODUCT(n,q->parser_attempts,m);RESERVE(exec_context_attempts,m,sl_sepol_memory_attempt);
 PRODUCT(n,q->diagnostics,m);RESERVE(exec_context_diagnostics,m,sl_sepol_diagnostic);
 PRODUCT(m,q->diagnostic_arguments,n);RESERVE(exec_context_arguments,n,sl_sepol_argument);
 PRODUCT(m,q->diagnostic_bytes,n);BYTES(exec_context_outputs,n);
 RESERVE(contexts,q->selinux_rows,sl_linux_selinux_context);
 PRODUCT(q->selinux_rows,q->parser_bytes,n);if(arena_reserve(l,&l->context_memory,n,1,_Alignof(max_align_t)))return -1;
 PRODUCT(q->selinux_rows,q->parser_attempts,n);RESERVE(context_attempts,n,sl_sepol_memory_attempt);
 PRODUCT(q->selinux_rows,q->diagnostics,n);RESERVE(context_diagnostics,n,sl_sepol_diagnostic);
 PRODUCT(n,q->diagnostic_arguments,m);RESERVE(context_arguments,m,sl_sepol_argument);
 PRODUCT(n,q->diagnostic_bytes,m);BYTES(context_outputs,m);
 PRODUCT(q->selinux_rows,q->parser_bytes,n);if(arena_reserve(l,&l->payload_memory,n,1,_Alignof(max_align_t)))return -1;
 PRODUCT(q->selinux_rows,q->parser_attempts,n);RESERVE(payload_attempts,n,sl_sepol_memory_attempt);
 PRODUCT(q->selinux_rows,q->diagnostics,n);RESERVE(payload_diagnostics,n,sl_sepol_diagnostic);
 PRODUCT(n,q->diagnostic_arguments,m);RESERVE(payload_arguments,m,sl_sepol_argument);
 PRODUCT(n,q->diagnostic_bytes,m);BYTES(payload_outputs,m);
 if(q->policy_body>ULONG_MAX-72||q->policy_body>UINT64_MAX/8)return -1;
 RESERVE(candidate_policies,q->selinux_rows,sl_linux_candidate_policy);
 RESERVE(genfs_correspondences,q->selinux_rows,sl_linux_genfs_correspondence);
 PRODUCT(q->selinux_rows,protected_rows,m);RESERVE(genfs_declarations,m,sl_linux_genfs_declaration);
 RESERVE(candidate_digest_workspaces,q->selinux_rows,sl_owned_sha256_workspace);
 RESERVE(candidate_roles,q->selinux_rows,sl_linux_candidate_roles);
 PRODUCT(q->selinux_rows,q->maintenance_role_rows?q->maintenance_role_rows:1,n);RESERVE(candidate_role_bindings,n,sl_candidate_role_binding);
 RESERVE(candidate_file_entry,q->selinux_rows,sl_linux_candidate_file_entry);
 PRODUCT(q->domain_types,q->domain_types,n);PRODUCT(q->selinux_rows,n,m);RESERVE(file_entry_edges,m,sl_selinux_transition_edge);
 PRODUCT(q->domain_types,2,n);PRODUCT(q->selinux_rows,n,m);RESERVE(file_entry_matches,m,unsigned);
 PRODUCT(q->selinux_rows,q->reach_visits,n);RESERVE(file_entry_visits,n,sl_selinux_rule_visit);RESERVE(exec_type_visits,n,sl_selinux_rule_visit);
 PRODUCT(q->selinux_rows,q->reach_helpers,n);RESERVE(file_entry_helpers,n,sl_selinux_helper_visit);RESERVE(exec_type_helpers,n,sl_selinux_helper_visit);RESERVE(exec_context_visits,n,sl_exec_context_visit);
 RESERVE(target_statuses,q->selinux_rows,sl_linux_selinux_status_sample);
 RESERVE(candidate_preexec,q->selinux_rows,sl_linux_candidate_preexec);
 PRODUCT(q->selinux_rows,q->domain_types,n);RESERVE(preexec_bounds,n,type_datum_t *);
 PRODUCT(q->selinux_rows,q->parser_bytes,n);if(arena_reserve(l,&l->preexec_memory,n,1,_Alignof(max_align_t)))return -1;
 PRODUCT(q->selinux_rows,q->parser_attempts,n);RESERVE(preexec_attempts,n,sl_sepol_memory_attempt);
 PRODUCT(q->selinux_rows,q->diagnostics,n);RESERVE(preexec_diagnostics,n,sl_sepol_diagnostic);
 PRODUCT(n,q->diagnostic_arguments,m);RESERVE(preexec_arguments,m,sl_sepol_argument);
 PRODUCT(n,q->diagnostic_bytes,m);BYTES(preexec_outputs,m);
 PRODUCT(q->selinux_rows,4,m);RESERVE(candidate_owner_entry,m,sl_linux_candidate_ptrace);
 PRODUCT(m,q->domain_types,n);RESERVE(owner_entry_sources,n,sl_selinux_source_access);RESERVE(owner_entry_classifications,n,sl_candidate_route_source);
 PRODUCT(m,q->access_visits,n);RESERVE(owner_entry_visits,n,sl_selinux_rule_visit);
 PRODUCT(m,q->access_helpers,n);RESERVE(owner_entry_helpers,n,sl_selinux_helper_visit);
 PRODUCT(q->selinux_rows,4,m);RESERVE(candidate_socket_uses,m,sl_linux_candidate_socket_create);
 PRODUCT(m,q->domain_types,n);RESERVE(socket_use_sources,n,sl_selinux_source_access);RESERVE(socket_use_classifications,n,sl_candidate_route_source);
 PRODUCT(m,q->access_visits,n);RESERVE(socket_use_visits,n,sl_selinux_rule_visit);
 PRODUCT(m,q->access_helpers,n);RESERVE(socket_use_helpers,n,sl_selinux_helper_visit);
 PRODUCT(q->selinux_rows,2,m);RESERVE(candidate_socket_creates,m,sl_linux_candidate_socket_create);
 PRODUCT(m,q->domain_types,n);RESERVE(socket_create_sources,n,sl_selinux_source_access);RESERVE(socket_create_classifications,n,sl_candidate_route_source);
 PRODUCT(m,q->access_visits,n);RESERVE(socket_create_visits,n,sl_selinux_rule_visit);
 PRODUCT(m,q->access_helpers,n);RESERVE(socket_create_helpers,n,sl_selinux_helper_visit);
 PRODUCT(q->selinux_rows,2,m);RESERVE(candidate_ptrace,m,sl_linux_candidate_ptrace);
 PRODUCT(m,q->domain_types,n);RESERVE(ptrace_sources,n,sl_selinux_source_access);RESERVE(ptrace_classifications,n,sl_candidate_route_source);
 PRODUCT(m,q->access_visits,n);RESERVE(ptrace_visits,n,sl_selinux_rule_visit);
 PRODUCT(m,q->access_helpers,n);RESERVE(ptrace_helpers,n,sl_selinux_helper_visit);
 RESERVE(parses,q->selinux_rows,sl_linux_policy_parse);
 PRODUCT(q->selinux_rows,q->parser_bytes,n);
 if(arena_reserve(l,&l->parser_memory,n,1,_Alignof(max_align_t)))return -1;
 PRODUCT(q->selinux_rows,q->parser_attempts,n);RESERVE(parser_attempts,n,sl_sepol_memory_attempt);
 PRODUCT(q->selinux_rows,q->diagnostics,n);RESERVE(diagnostics,n,sl_sepol_diagnostic);
 PRODUCT(n,q->diagnostic_arguments,m);RESERVE(diagnostic_arguments,m,sl_sepol_argument);
 PRODUCT(n,q->diagnostic_bytes,m);BYTES(diagnostic_outputs,m);
 RESERVE(children,q->mountinfo_rows,sl_linux_cgroup_children);
 PRODUCT(q->mountinfo_rows,q->children_frames,n);RESERVE(children_frames,n,sl_linux_child_frame);
 PRODUCT(n,q->children_bytes,m);BYTES(children_bytes,m);
 PRODUCT(q->mountinfo_rows,q->children_views,n);RESERVE(children_views,n,sl_linux_child_view);
 PRODUCT(n,2,m);RESERVE(controls,m,sl_linux_cgroup_control_label);
 PRODUCT(m,q->security_label_bytes,n);BYTES(control_labels,n);
 PRODUCT(m,q->selinux_rows,n);RESERVE(control_contexts,n,sl_linux_selinux_context);
 RESERVE(migration_labels,q->selinux_rows,sl_linux_migration_labels);
 PRODUCT(q->selinux_rows,q->domain_types,m);RESERVE(migration_types,m,unsigned);RESERVE(migration_referents,m,type_datum_t*);RESERVE(migration_filesystem_types,m,unsigned);RESERVE(migration_filesystem_referents,m,type_datum_t*);
 PRODUCT(q->selinux_rows,q->access_helpers,m);RESERVE(migration_helpers,m,sl_selinux_helper_visit);
 RESERVE(migration_observed,n,context_struct_t*);
 PRODUCT(q->selinux_rows,3,m);RESERVE(migration_access,m,sl_linux_migration_access);RESERVE(migration_routes,m,sl_linux_migration_routes);
 PRODUCT(m,q->domain_types,m);RESERVE(migration_access_sources,m,sl_selinux_source_access);RESERVE(migration_route_sources,m,sl_candidate_route_source);
 PRODUCT(q->selinux_rows,3,m);PRODUCT(m,q->access_visits,m);RESERVE(migration_access_visits,m,sl_selinux_rule_visit);
 PRODUCT(q->selinux_rows,3,m);PRODUCT(m,q->access_helpers,m);RESERVE(migration_access_helpers,m,sl_selinux_helper_visit);
 if(!q->access_strings)return -1;
 PRODUCT(n,2,m);RESERVE(control_access,m,sl_linux_control_access);
 RESERVE(candidate_routes,m,sl_linux_candidate_routes);
 PRODUCT(n,2,m);PRODUCT(m,q->domain_types,m);RESERVE(candidate_route_sources,m,sl_candidate_route_source);
 PRODUCT(n,2,m);PRODUCT(m,q->domain_types,m);RESERVE(access_sources,m,sl_selinux_source_access);
 PRODUCT(n,2,m);PRODUCT(m,q->access_visits,m);RESERVE(access_visits,m,sl_selinux_rule_visit);
 PRODUCT(n,2,m);PRODUCT(m,q->access_helpers,m);RESERVE(access_helpers,m,sl_selinux_helper_visit);
 PRODUCT(n,q->parser_bytes,m);if(arena_reserve(l,&l->control_context_memory,m,1,_Alignof(max_align_t)))return -1;
 PRODUCT(n,q->parser_attempts,m);RESERVE(control_context_attempts,m,sl_sepol_memory_attempt);
 PRODUCT(n,q->diagnostics,m);RESERVE(control_context_diagnostics,m,sl_sepol_diagnostic);
 PRODUCT(m,q->diagnostic_arguments,n);RESERVE(control_context_arguments,n,sl_sepol_argument);
 PRODUCT(m,q->diagnostic_bytes,n);BYTES(control_context_outputs,n);
 PRODUCT(4,q->selinux_rows,n);RESERVE(archive_contexts,n,sl_linux_selinux_context);
 PRODUCT(n,q->parser_bytes,m);if(arena_reserve(l,&l->archive_context_memory,m,1,_Alignof(max_align_t)))return -1;
 PRODUCT(n,q->parser_attempts,m);RESERVE(archive_context_attempts,m,sl_sepol_memory_attempt);
 PRODUCT(n,q->diagnostics,m);RESERVE(archive_context_diagnostics,m,sl_sepol_diagnostic);
 PRODUCT(m,q->diagnostic_arguments,n);RESERVE(archive_context_arguments,n,sl_sepol_argument);
 PRODUCT(m,q->diagnostic_bytes,n);BYTES(archive_context_outputs,n);
 PRODUCT(SL_ARCHIVE_PURPOSE_COUNT,q->selinux_rows,n);RESERVE(archive_accesses,n,sl_linux_archive_access);
 PRODUCT(n,q->domain_types,m);RESERVE(archive_inventory_sources,m,sl_selinux_source_access);RESERVE(archive_sources,m,sl_linux_archive_source);
 PRODUCT(n,q->access_visits,m);RESERVE(archive_visits,m,sl_selinux_rule_visit);
 PRODUCT(n,q->access_helpers,m);RESERVE(archive_helpers,m,sl_selinux_helper_visit);
 RESERVE(archive_creations,q->selinux_rows,sl_linux_archive_creation);
 PRODUCT(q->selinux_rows,q->domain_types,n);RESERVE(archive_creation_types,n,unsigned);RESERVE(archive_creation_referents,n,type_datum_t*);
 PRODUCT(q->selinux_rows,q->access_visits,n);RESERVE(archive_creation_visits,n,sl_selinux_rule_visit);
 PRODUCT(q->selinux_rows,q->access_helpers,n);RESERVE(archive_creation_helpers,n,sl_selinux_helper_visit);
 BYTES(executable_path,q->executable_path);BYTES(directory_path,q->directory_path);
 RESERVE(image_frames,q->image_frames,sl_linux_payload_frame);PRODUCT(q->image_frames,q->image_bytes,n);
 BYTES(image_source,n);BYTES(image_readback,n);
 PRODUCT(q->image_frames,q->image_writes,n);RESERVE(image_writes,n,sl_linux_payload_attempt);
 PRODUCT(q->image_frames,q->image_readbacks,n);RESERVE(image_readbacks,n,sl_linux_payload_attempt);
 BYTES(pidfd_info_bytes,q->pidfd_info_bytes);RESERVE(pidfd_info_reads,q->pidfd_info_attempts,sl_linux_call);
 if(l->bytes>LONG_MAX)return -1;return 0;
}
#undef PRODUCT
#undef BYTES
#undef RESERVE
#define REGION(name,type) ((type*)((unsigned char*)a->base+l->name.offset))
static void arena_bind(sl_linux_candidate_arena *a){
 const sl_linux_candidate_quantities *q=&a->prepared_quantities;sl_linux_candidate_layout *l=&a->layout;
 unsigned long i,j;sl_linux_public_startup *s=REGION(startup,sl_linux_public_startup);
 sl_linux_public_controller *c=REGION(controller,sl_linux_public_controller);
 a->public_main_owner=REGION(public_main_owner,sl_linux_public_main_owner);
 a->outer_birth=REGION(outer_birth,sl_linux_outer_birth);
 a->outer_continuation=REGION(outer_continuation,sl_linux_outer_continuation);a->outer_continuation->birth_original=a->outer_birth;
 a->outer_observations=REGION(outer_observations,sl_linux_outer_terminal);a->outer_observation_capacity=q->outer_observations;
 for(i=0;i<q->outer_observations;i++)a->outer_observations[i].birth_original=a->outer_birth;
 a->outer_supervisor=REGION(outer_supervisor,sl_linux_outer_supervisor);
 a->failed_parent_identity=REGION(failed_parent_identity,sl_linux_failed_parent_identity);
 a->failed_archive_header=REGION(failed_archive_header,sl_linux_failed_archive_header);
 a->failed_parent_directory=REGION(failed_parent_directory,sl_linux_failed_parent_directory);
 a->failed_parent_writer=REGION(failed_parent_writer,sl_linux_failed_parent_file);
 a->failed_parent_reader=REGION(failed_parent_reader,sl_linux_failed_parent_file);
 a->failed_parent_path=REGION(failed_parent_path,unsigned char);a->failed_parent_path_capacity=q->archive_path_bytes;
 a->failed_parent_declarations=REGION(failed_parent_declarations,sl_linux_archive_declaration);a->failed_parent_declaration_capacity=q->archive_declarations;
 a->outer_supervisor->continuation_original=a->outer_continuation;
 a->outer_supervisor->turns_original=REGION(outer_turns,sl_linux_outer_turn);
 a->outer_supervisor->observations_original=a->outer_observations;a->outer_supervisor->capacity=q->outer_observations;
 a->deadline_sources=REGION(deadline_sources,sl_linux_deadline_source);
 a->deadlines=REGION(deadlines,sl_linux_deadline);
 a->deadline_sequences=REGION(deadline_sequences,sl_linux_deadline_sequence);a->deadline_capacity=q->deadline_roots;
 a->deadline_polls=REGION(deadline_polls,sl_linux_deadline_poll);
 a->deadline_poll_capacity=q->deadline_polls;a->deadline_poll_fd_capacity=q->deadline_poll_fds;
 a->public_inputs=REGION(public_inputs,sl_linux_public_inputs);
 a->public_inputs->source_original=REGION(public_input_source,sl_linux_public_input_source);
 sl_linux_public_input_source *public_source=REGION(public_input_source,sl_linux_public_input_source);
 public_source->argument_capacity=q->public_argument_count;public_source->argument_bytes=q->public_argument_bytes;
 public_source->environment_capacity=q->public_environment_count;public_source->environment_bytes=q->public_environment_bytes;
 a->public_inputs->arguments=REGION(public_argument_views,sl_linux_public_string);
 a->public_inputs->environment=REGION(public_environment_views,sl_linux_public_string);
 /* Actual native argc/argv/environment are supplied at the genuine prebirth entry, not by this binder. */
 a->creator_file=REGION(creator_file,sl_linux_creator_file);
 a->creator_file->installation_original=&s->installation;
 a->creator_file->body=REGION(creator_file_body,unsigned char);a->creator_file->body_capacity=q->creator_file_bytes;
 a->creator_file->reads=REGION(creator_file_reads,sl_linux_creator_read);a->creator_file->read_capacity=q->creator_file_reads;
 a->creator_elf=REGION(creator_elf,sl_linux_creator_elf);a->creator_elf->file_original=a->creator_file;
 a->creator_elf->programs=REGION(creator_elf_programs,sl_linux_elf_program);a->creator_elf->capacity=q->creator_file_bytes/56;
 for(i=0;i<q->deadline_roots;i++){
  sl_linux_deadline_sequence *d=&a->deadline_sequences[i];
  d->root_original=&a->deadlines[i];
  d->observations_original=REGION(deadline_observations,sl_linux_deadline_observation)+i*q->deadline_observations;
  d->capacity=q->deadline_observations;
  for(j=0;j<q->deadline_polls;j++){
   unsigned long slot=i*q->deadline_polls+j;
   sl_linux_deadline_poll *p=&a->deadline_polls[slot];
   p->sequence_original=d;
   p->descriptors_original=REGION(deadline_poll_descriptors,struct pollfd)+slot*q->deadline_poll_fds;
   p->outputs_original=REGION(deadline_poll_outputs,struct pollfd)+slot*q->deadline_poll_fds;
   /* Actual count/descriptors are acquired at the owning operation boundary;
    * no FD identity, source root or native wait is invented by this binding. */
  }
  /* Native clock/source tuple and actual sequence enrollment belong to the
   * genuine operation boundary; this is prebirth storage binding only. */
 }
 if(a->mapping.arguments[3]==(MAP_SHARED|MAP_ANONYMOUS)){
  c->entry.bootstrap_range=(sl_linux_shared_range){&a->mapping,l->bootstrap_backing.offset,l->bootstrap_backing.bytes};
  c->entry.fd_action_range=(sl_linux_shared_range){&a->mapping,l->fd_action_backing.offset,l->fd_action_backing.bytes};
  for(i=0;i<2;i++){
   c->entry.credential_ranges[i][0]=(sl_linux_shared_range){&a->mapping,l->credential_status_backing[i].offset,l->credential_status_backing[i].bytes};
   c->entry.credential_ranges[i][1]=(sl_linux_shared_range){&a->mapping,l->credential_read_backing[i].offset,l->credential_read_backing[i].bytes};
   c->entry.credential_ranges[i][2]=(sl_linux_shared_range){&a->mapping,l->credential_group_backing[i].offset,l->credential_group_backing[i].bytes};
  }
 }
 for(i=0;i<2;i++){c->entry.custody_peers[i].bytes=REGION(custody_peer_labels,unsigned char)+i*q->security_label_bytes;c->entry.custody_peers[i].capacity=q->security_label_bytes;}
 c->private_arguments.security_quantity_original=&q->security_label_bytes;c->private_arguments.terminal_quantity_original=&q->terminal_calls;c->private_arguments.representation_quantity_original=&q->body_bytes;
 c->private_arguments.private_argv=REGION(private_arguments,char*);c->private_arguments.pointer_capacity=q->public_argument_count+8;
 c->private_arguments.lengths=REGION(argument_lengths,unsigned long);c->private_arguments.length_capacity=q->public_argument_count;c->private_arguments.string_byte_capacity=q->public_argument_bytes;
 a->startup=s;s->controller_original=c;c->startup_original=s;s->candidate_target.startup_original=s;s->candidate_target.statuses=REGION(target_statuses,sl_linux_selinux_status_sample);s->candidate_target.status_capacity=q->selinux_rows;
 s->final_reader.startup_original=s;s->archive_files.startup_original=s;s->archive_files.statuses=REGION(archive_file_statuses,sl_linux_selinux_status_sample);s->archive_files.status_capacity=q->selinux_rows;s->archive_directory.startup_original=s;s->archive_directory.path=REGION(archive_path,unsigned char);s->archive_directory.path_capacity=q->archive_path_bytes;
 s->archive_directory.declarations=REGION(archive_declarations,sl_linux_archive_declaration);s->archive_directory.declaration_capacity=q->archive_declarations;
 s->archive_directory.statuses=REGION(archive_statuses,sl_linux_selinux_status_sample);s->archive_directory.status_capacity=q->selinux_rows;
 s->public_label.bytes=REGION(exec_labels,unsigned char);s->public_label.capacity=q->self_context_bytes;
 s->payload_label.bytes=s->public_label.bytes+q->self_context_bytes;s->payload_label.capacity=q->self_context_bytes;
 c->namespace_observation.pidfd_info.bytes=REGION(pidfd_info_bytes,unsigned char);
 c->namespace_observation.pidfd_info.capacity=q->pidfd_info_bytes;
 c->namespace_observation.pidfd_info.reads=REGION(pidfd_info_reads,sl_linux_call);
 c->namespace_observation.pidfd_info.read_capacity=q->pidfd_info_attempts;
 c->namespace_observation.pidfd_info.birth_original=&c->entry.birth;
 c->namespace_observation.pidfd_info.procfs_original=&c->namespace_observation.views[0];
 c->credential_status_bytes=q->credential_status_bytes;c->credential_status_attempts=q->credential_status_attempts;c->credential_group_capacity=q->credential_group_capacity;
 c->entry.birth.cgroup_fd=-1;c->entry.birth.payload_fd=-1;c->entry.birth.entry_gate_fd=-1;
 c->readers=REGION(readers,sl_linux_capture_worker);c->forwarders=REGION(forwarders,sl_linux_forward_worker);
 /* Actual route count is derived by the public caller from observed stdio;
  * allocate three disjoint candidate slots without inventing route presence. */
 for(i=0;i<3;i++){
  sl_linux_capture_worker *r=&c->readers[i];sl_linux_forward_worker *f=&c->forwarders[i];
  sl_linux_call *writes=REGION(public_writes[i],sl_linux_call);
  r->frames=REGION(raw_frames[i],sl_linux_capture_frame);r->capacity=q->raw_frames[i];
  for(j=0;j<r->capacity;j++){r->frames[j].public_writes=writes+j*q->public_writes[i];r->frames[j].public_write_capacity=q->public_writes[i];}
  f->waits=REGION(waits[i],sl_linux_forward_wait);f->wait_capacity=q->forward_waits[i];
  f->iterations=REGION(iterations[i],sl_linux_forward_iteration);f->iteration_capacity=q->forward_iterations[i];
  f->publications=REGION(publications[i],sl_linux_notification);f->publication_capacity=q->raw_frames[i];
 }
 a->services=REGION(services,sl_linux_service_observation);a->service_capacity=q->services;
 a->collector_joins=REGION(collector_joins,sl_linux_collector_join);a->collector_join_capacity=q->collector_joins;
 c->collector_join_roster=a->collector_joins;c->collector_join_capacity=a->collector_join_capacity;
 for(i=0;i<q->collector_joins;i++)a->collector_joins[i].controller_original=c;
 c->initial_service=a->services;c->service_roster=a->services;c->service_capacity=q->services;
 for(i=0;i<q->services;i++){
  sl_linux_service_observation *o=&a->services[i];
  o->controller_original=c;o->root.birth_original=&c->entry.birth;o->authority.birth_original=&c->entry.birth;
  o->raw_joins=REGION(raw_joins,sl_linux_reader_join)+i*3;
  o->forward_releases=REGION(releases,sl_linux_forward_release)+i*3;
  o->forward_joins=REGION(forward_joins,sl_linux_forward_join)+i*3;
  for(j=0;j<3;j++){
   o->raw_joins[j].worker_original=&c->readers[j];
   o->forward_releases[j].worker_original=&c->forwarders[j];
   o->forward_joins[j].worker_original=&c->forwarders[j];
  }
  /* capacity is set to actual reader_count by the source-selected caller. */
 }
 c->channel.requests=REGION(requests,sl_linux_terminal_request);c->channel.capacity=q->requests;
 for(i=0;i<q->requests;i++){
  sl_linux_terminal_request *r=&c->channel.requests[i];
  r->prefix.receive.attempts=REGION(prefix_calls,sl_linux_call)+i*q->prefix_attempts;r->prefix.receive.capacity=q->prefix_attempts;
  r->reply.send.attempts=REGION(reply_calls,sl_linux_call)+i*q->terminal_reply_attempts;r->reply.send.capacity=q->terminal_reply_attempts;
  r->input.frames=REGION(input_frames,sl_linux_input_frame)+i*q->input_frames;r->input.capacity=q->input_frames;
  for(j=0;j<q->input_frames;j++){
   r->input.frames[j].steps=REGION(input_steps,sl_linux_input_step)+(i*q->input_frames+j)*q->input_steps;
   r->input.frames[j].capacity=q->input_steps;
  }
 }
 c->collection.body=REGION(body,unsigned char);c->collection.readback=REGION(readback,unsigned char);c->collection.capacity=q->body_bytes;
 c->final_capture.controller_original=c;
 c->final_capture.records=REGION(final_records,sl_linux_final_record);c->final_capture.capacity=l->final_records.bytes/sizeof(sl_linux_final_record);
 c->final_capture.readback=REGION(final_readback,unsigned char);c->final_capture.bytes=l->final_readback.bytes;
 c->final_capture.writes.calls=REGION(final_calls[0],sl_linux_call);c->final_capture.writes.capacity=q->final_io_attempts[0];
 c->final_capture.reads.calls=REGION(final_calls[1],sl_linux_call);c->final_capture.reads.capacity=q->final_io_attempts[1];
 c->final_reader_task.controller_original=c;c->final_reader_task.view_original=&s->final_reader;c->final_reader_task.capture_original=&c->final_capture;
 for(i=0;i<3;i++){s->archive_labels[i].startup_original=s;s->archive_labels[i].kind=(enum sl_linux_archive_label_kind)i;s->archive_labels[i].bytes=REGION(archive_labels,unsigned char)+i*q->security_label_bytes;s->archive_labels[i].capacity=q->security_label_bytes;}
 c->final_reader_task.bytes=REGION(independent_readback,unsigned char);c->final_reader_task.capacity=l->independent_readback.bytes;
 c->final_reader_task.reads=REGION(independent_reads,sl_linux_call);c->final_reader_task.read_capacity=q->final_io_attempts[1];
 for(i=0;i<q->final_io_attempts[1];i++){atomic_init(&c->final_reader_task.reads[i].entered,0);atomic_init(&c->final_reader_task.reads[i].returned,0);}
 c->final_reader_joins=REGION(independent_joins,sl_linux_archive_reader_join);c->final_reader_join_capacity=q->services;
 c->collection.endpoint_fd=-1;c->collection.storage_fd=-1;
 {sl_linux_custody_io *io[5]={&c->collection.receive_header,&c->collection.receive_body,&c->collection.write_body,&c->collection.read_body,&c->collection.send_reply};
  for(i=0;i<5;i++){io[i]->attempts=REGION(custody_calls[i],sl_linux_call);io[i]->capacity=q->custody_attempts[i];}}
 c->scan.frames=REGION(scan_frames,sl_linux_fd_scan_frame);c->scan.frame_capacity=q->scan_frames;
 for(i=0;i<q->scan_frames;i++){c->scan.frames[i].bytes=REGION(scan_bytes,unsigned char)+i*q->scan_bytes;c->scan.frames[i].capacity=q->scan_bytes;}
 c->scan.views=REGION(fd_views,sl_linux_fd_view);c->scan.view_capacity=q->fd_views;
 c->child_map.actions=REGION(fd_actions,sl_linux_fd_action);c->child_map.action_capacity=q->fd_actions;
 s->membership.frames=REGION(membership_frames,sl_linux_membership_frame);s->membership.capacity=q->membership_frames;
 for(i=0;i<q->membership_frames;i++){s->membership.frames[i].bytes=REGION(membership_bytes,unsigned char)+i*q->membership_bytes;s->membership.frames[i].capacity=q->membership_bytes;}
 s->locations=REGION(mount_locations,sl_linux_mount_location);s->location_capacity=q->mountinfo_rows;
 for(i=0;i<q->mountinfo_rows;i++){sl_linux_mount_location *location=&s->locations[i];
  location->root=REGION(mount_roots,char)+i*q->mount_location_bytes;location->mountpoint=REGION(mount_points,char)+i*q->mount_location_bytes;
  location->relative=REGION(mount_relatives,char)+i*q->mount_location_bytes;location->capacity=q->mount_location_bytes;
 }
 s->children=REGION(children,sl_linux_cgroup_children);s->children_capacity=q->mountinfo_rows;
 for(i=0;i<q->mountinfo_rows;i++){
  sl_linux_cgroup_children *child=&s->children[i];child->frames=REGION(children_frames,sl_linux_child_frame)+i*q->children_frames;child->frame_capacity=q->children_frames;
  child->views=REGION(children_views,sl_linux_child_view)+i*q->children_views;child->view_capacity=q->children_views;
  for(j=0;j<q->children_frames;j++){child->frames[j].bytes=REGION(children_bytes,unsigned char)+(i*q->children_frames+j)*q->children_bytes;child->frames[j].capacity=q->children_bytes;}
 }
 s->mounts.selinux_rows=REGION(selinux_rows,sl_linux_mount_row);s->mounts.selinux_capacity=q->selinux_rows;
 s->policies=REGION(policies,sl_linux_selinux_snapshot);s->policy_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_selinux_snapshot *policy=&s->policies[i];policy->path=REGION(policy_paths,char)+i*q->selinux_path;policy->path_capacity=q->selinux_path;
  policy->body=REGION(policy_body,unsigned char)+i*q->policy_body;policy->body_capacity=q->policy_body;
  policy->frames=REGION(policy_frames,sl_linux_policy_frame)+i*q->policy_frames;policy->frame_capacity=q->policy_frames;
  for(j=0;j<q->policy_frames;j++){policy->frames[j].bytes=REGION(policy_frame_bytes,unsigned char)+(i*q->policy_frames+j)*q->policy_frame_bytes;policy->frames[j].capacity=q->policy_frame_bytes;}
 }
 s->candidate_original=a->candidate_original;
 s->candidate_policies=REGION(candidate_policies,sl_linux_candidate_policy);s->candidate_policy_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_candidate_policy *binding=&s->candidate_policies[i];
  binding->candidate_original=s->candidate_original;binding->parse_original=REGION(parses,sl_linux_policy_parse)+i;
  binding->digest.workspace=REGION(candidate_digest_workspaces,sl_owned_sha256_workspace)+i;
 }
 s->control_roster.startup_original=s;
 s->genfs_correspondences=REGION(genfs_correspondences,sl_linux_genfs_correspondence);s->genfs_correspondence_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_genfs_correspondence *g=&s->genfs_correspondences[i];g->policy_original=&s->candidate_policies[i];g->capacity=a->candidate_original->actual_capacities[3];g->declarations=REGION(genfs_declarations,sl_linux_genfs_declaration)+i*g->capacity;
 }
 s->candidate_roles=REGION(candidate_roles,sl_linux_candidate_roles);s->candidate_role_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_candidate_roles *roles=&s->candidate_roles[i];roles->candidate_original=s->candidate_original;
  roles->parse_original=REGION(parses,sl_linux_policy_parse)+i;
  roles->bindings=REGION(candidate_role_bindings,sl_candidate_role_binding)+i*(q->maintenance_role_rows?q->maintenance_role_rows:1);roles->capacity=q->maintenance_role_rows;
 }
 s->parses=REGION(parses,sl_linux_policy_parse);s->parse_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_sepol_parse_owner *p=&s->parses[i].parser;
  p->memory.bytes=REGION(parser_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(parser_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;
  atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;
  atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];
   d->arguments=REGION(diagnostic_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(diagnostic_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 s->controls=REGION(controls,sl_linux_cgroup_control_label);s->control_capacity=q->mountinfo_rows*q->children_views*2;
 for(i=0;i<s->control_capacity;i++){s->controls[i].bytes=REGION(control_labels,unsigned char)+i*q->security_label_bytes;s->controls[i].capacity=q->security_label_bytes;}
 s->control_contexts=REGION(control_contexts,sl_linux_selinux_context);s->control_context_capacity=s->control_capacity*q->selinux_rows;
 for(i=0;i<s->control_context_capacity;i++){
  sl_linux_selinux_context *p=&s->control_contexts[i];
  p->memory.bytes=REGION(control_context_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(control_context_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(control_context_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];
   d->arguments=REGION(control_context_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(control_context_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 s->migration_labels=REGION(migration_labels,sl_linux_migration_labels);s->migration_label_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_migration_labels *o=&s->migration_labels[i];o->startup_original=s;o->parse_original=&s->parses[i];o->policy_index=i;o->observed_capacity=s->control_capacity;
  o->labels.observed=REGION(migration_observed,context_struct_t*)+i*s->control_capacity;
  o->labels.targets.types=REGION(migration_types,unsigned)+i*q->domain_types;o->labels.targets.referents=REGION(migration_referents,type_datum_t*)+i*q->domain_types;o->labels.targets.capacity=q->domain_types;
  o->labels.filesystem.types=REGION(migration_filesystem_types,unsigned)+i*q->domain_types;o->labels.filesystem.referents=REGION(migration_filesystem_referents,type_datum_t*)+i*q->domain_types;o->labels.filesystem.capacity=q->domain_types;
  o->labels.targets.custody.helpers=REGION(migration_helpers,sl_selinux_helper_visit)+i*q->access_helpers;o->labels.targets.custody.helper_capacity=q->access_helpers;o->labels.targets.custody.string_capacity=q->access_strings;
 }
 s->migration_access=REGION(migration_access,sl_linux_migration_access);s->migration_access_capacity=3*q->selinux_rows;
 for(i=0;i<s->migration_access_capacity;i++){
  sl_linux_migration_access *o=&s->migration_access[i];o->startup_original=s;o->policy_index=i/3;o->kind=i%3;o->labels_original=&s->migration_labels[i/3];
  o->inventory.sources=REGION(migration_access_sources,sl_selinux_source_access)+i*q->domain_types;o->inventory.source_capacity=q->domain_types;
  o->inventory.custody.visits=REGION(migration_access_visits,sl_selinux_rule_visit)+i*q->access_visits;o->inventory.custody.capacity=q->access_visits;
  o->inventory.custody.helpers=REGION(migration_access_helpers,sl_selinux_helper_visit)+i*q->access_helpers;o->inventory.custody.helper_capacity=q->access_helpers;o->inventory.custody.string_capacity=q->access_strings;
 }
 s->migration_routes=REGION(migration_routes,sl_linux_migration_routes);s->migration_route_capacity=3*q->selinux_rows;
 for(i=0;i<s->migration_route_capacity;i++){
  sl_linux_migration_routes *o=&s->migration_routes[i];o->startup_original=s;o->index=i;o->access_original=&s->migration_access[i];o->roles_original=REGION(candidate_roles,sl_linux_candidate_roles)+i/3;o->owner_original=REGION(contexts,sl_linux_selinux_context)+i/3;o->payload_original=&(REGION(candidate_payloads,sl_linux_candidate_payload_context)+i/3)->reachable;
  o->sources=REGION(migration_route_sources,sl_candidate_route_source)+i*q->domain_types;o->capacity=q->domain_types;
 }
 s->control_access=REGION(control_access,sl_linux_control_access);s->control_access_capacity=s->control_context_capacity*2;
 s->archive_contexts=REGION(archive_contexts,sl_linux_selinux_context);s->archive_context_capacity=3*q->selinux_rows;
 s->archive_fscreate_contexts=s->archive_contexts+3*q->selinux_rows;s->archive_fscreate_context_capacity=q->selinux_rows;
 for(i=0;i<4*q->selinux_rows;i++){
  sl_linux_selinux_context *p=&s->archive_contexts[i];if(i<3*q->selinux_rows)p->archive_original=&s->archive_labels[i/q->selinux_rows];p->parse_original=&s->parses[i%q->selinux_rows];
  p->memory.bytes=REGION(archive_context_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(archive_context_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(archive_context_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];d->arguments=REGION(archive_context_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(archive_context_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 for(i=0;i<s->control_access_capacity;i++){
  s->control_access[i].kind_original=(i%2)?SL_CONTROL_RELABEL_FROM:SL_CONTROL_WRITE_APPEND;sl_selinux_access_sources *inventory=&s->control_access[i].inventory;
  inventory->sources=REGION(access_sources,sl_selinux_source_access)+i*q->domain_types;inventory->source_capacity=q->domain_types;
  inventory->custody.visits=REGION(access_visits,sl_selinux_rule_visit)+i*q->access_visits;inventory->custody.capacity=q->access_visits;
  inventory->custody.helpers=REGION(access_helpers,sl_selinux_helper_visit)+i*q->access_helpers;inventory->custody.helper_capacity=q->access_helpers;inventory->custody.string_capacity=q->access_strings;
 }
 s->archive_fscreate.bytes=REGION(archive_fscreate,unsigned char);s->archive_fscreate.capacity=q->self_context_bytes;
 {sl_linux_call *v[3]={&s->archive_fscreate.before,&s->archive_fscreate.query,&s->archive_fscreate.after};for(i=0;i<3;i++){atomic_init(&v[i]->entered,0);atomic_init(&v[i]->returned,0);s->archive_fscreate.initialized_calls[i]=v[i];}}
 s->self_context.bytes=REGION(self_context,unsigned char);s->self_context.capacity=q->self_context_bytes;
 /* Fresh anonymous arena, no borrowers: initialize before atomic queries. */
 {sl_linux_call *v[3]={&s->self_context.before,&s->self_context.query,&s->self_context.after};
  for(i=0;i<3;i++){atomic_init(&v[i]->entered,0);atomic_init(&v[i]->returned,0);s->self_context.initialized_calls[i]=v[i];}}
 s->reachability=REGION(reachability,sl_linux_context_reachability);s->reachability_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_selinux_reachability *graph=&s->reachability[i].graph;unsigned long edges=q->domain_types*q->domain_types;
  graph->matrix.edges=REGION(reach_edges,sl_selinux_transition_edge)+i*edges;graph->matrix.edge_capacity=edges;
  graph->matrix.matches=REGION(reach_matches,unsigned)+i*q->domain_types*2;graph->matrix.match_capacity=q->domain_types*2;
  graph->matrix.custody.visits=REGION(reach_visits,sl_selinux_rule_visit)+i*q->reach_visits;graph->matrix.custody.capacity=q->reach_visits;
  graph->matrix.custody.helpers=REGION(reach_helpers,sl_selinux_helper_visit)+i*q->reach_helpers;graph->matrix.custody.helper_capacity=q->reach_helpers;graph->matrix.custody.string_capacity=q->reach_strings;
  graph->reached=REGION(reach_types,unsigned)+i*q->domain_types;graph->type_capacity=q->domain_types;
  graph->steps=REGION(reach_steps,sl_selinux_reach_step)+i*q->reach_steps;graph->step_capacity=q->reach_steps;
 }
 s->domains=REGION(domains,sl_linux_bound_domains);s->domain_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  s->domains[i].entries=REGION(domain_entries,sl_linux_domain_entry)+i*q->domain_types;s->domains[i].capacity=q->domain_types;
  s->domains[i].steps=REGION(domain_steps,sl_linux_domain_step)+i*q->domain_steps;s->domains[i].step_capacity=q->domain_steps;
 }
 s->contexts=REGION(contexts,sl_linux_selinux_context);s->context_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_selinux_context *p=&s->contexts[i];
  p->memory.bytes=REGION(context_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(context_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(context_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];
   d->arguments=REGION(context_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(context_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 s->candidate_socket_contexts=REGION(candidate_socket_contexts,sl_linux_candidate_socket_context);s->candidate_socket_context_capacity=2*q->selinux_rows;
 for(i=0;i<s->candidate_socket_context_capacity;i++){
  sl_linux_candidate_socket_context *o=&s->candidate_socket_contexts[i];o->policy_original=&s->candidate_policies[i/2];o->peer_original=&c->entry.custody_peers[i%2];
  sl_linux_selinux_context *p=&o->context;
  p->memory.bytes=REGION(socket_context_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(socket_context_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(socket_context_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];d->arguments=REGION(socket_context_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(socket_context_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 s->exec_contexts=REGION(exec_contexts,sl_linux_selinux_context);s->exec_context_capacity=2*q->selinux_rows;
 for(i=0;i<s->exec_context_capacity;i++){
  sl_linux_selinux_context *p=&s->exec_contexts[i];
  p->memory.bytes=REGION(exec_context_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(exec_context_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(exec_context_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];d->arguments=REGION(exec_context_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(exec_context_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 s->candidate_payloads=REGION(candidate_payloads,sl_linux_candidate_payload_context);s->candidate_payload_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  s->candidate_payloads[i].policy_original=&s->candidate_policies[i];
  sl_linux_selinux_context *p=&s->candidate_payloads[i].context;
  p->memory.bytes=REGION(payload_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(payload_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(payload_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];
   d->arguments=REGION(payload_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(payload_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }

 for(i=0;i<q->selinux_rows;i++){
  sl_selinux_reachability *graph=&s->candidate_payloads[i].reachable.graph;unsigned long edges=q->domain_types*q->domain_types;
  graph->matrix.edges=REGION(payload_edges,sl_selinux_transition_edge)+i*edges;graph->matrix.edge_capacity=edges;
  graph->matrix.matches=REGION(payload_matches,unsigned)+i*q->domain_types*2;graph->matrix.match_capacity=q->domain_types*2;
  graph->matrix.custody.visits=REGION(payload_visits,sl_selinux_rule_visit)+i*q->reach_visits;graph->matrix.custody.capacity=q->reach_visits;
  graph->matrix.custody.helpers=REGION(payload_helpers,sl_selinux_helper_visit)+i*q->reach_helpers;graph->matrix.custody.helper_capacity=q->reach_helpers;graph->matrix.custody.string_capacity=q->reach_strings;
  graph->reached=REGION(payload_types,unsigned)+i*q->domain_types;graph->type_capacity=q->domain_types;
  graph->steps=REGION(payload_steps,sl_selinux_reach_step)+i*q->reach_steps;graph->step_capacity=q->reach_steps;
 }
 s->candidate_routes=REGION(candidate_routes,sl_linux_candidate_routes);s->candidate_route_capacity=s->control_access_capacity;
 for(i=0;i<s->candidate_route_capacity;i++){
  sl_linux_candidate_routes *r=&s->candidate_routes[i];unsigned long policy=(i/2)%q->selinux_rows;
  r->roles_original=&s->candidate_roles[policy];r->control_original=&s->control_access[i];r->payload_original=&s->candidate_payloads[policy].reachable;
  r->sources=REGION(candidate_route_sources,sl_candidate_route_source)+i*q->domain_types;r->capacity=q->domain_types;
 }
 s->candidate_file_entry=REGION(candidate_file_entry,sl_linux_candidate_file_entry);s->candidate_file_entry_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_candidate_file_entry *e=&s->candidate_file_entry[i];e->policy_original=&s->candidate_policies[i];e->owner_original=&s->contexts[i];e->public_original=&s->exec_contexts[i];e->file_original=&s->exec_contexts[q->selinux_rows+i];e->payload_original=&s->candidate_payloads[i];
  e->exec_context.source_original=&s->candidate_payloads[i].context;e->exec_context.file_original=&s->exec_contexts[q->selinux_rows+i];e->exec_context.type_original=&e->exec_type;
  e->exec_context.visits=REGION(exec_context_visits,sl_exec_context_visit)+i*q->reach_helpers;e->exec_context.capacity=q->reach_helpers;
  e->exec_type.custody.visits=REGION(exec_type_visits,sl_selinux_rule_visit)+i*q->reach_visits;e->exec_type.custody.capacity=q->reach_visits;
  e->exec_type.custody.helpers=REGION(exec_type_helpers,sl_selinux_helper_visit)+i*q->reach_helpers;e->exec_type.custody.helper_capacity=q->reach_helpers;e->exec_type.custody.string_capacity=q->reach_strings;
  sl_selinux_transition_matrix *m=&e->matrix;unsigned long edges=q->domain_types*q->domain_types;
  m->edges=REGION(file_entry_edges,sl_selinux_transition_edge)+i*edges;m->edge_capacity=edges;
  m->matches=REGION(file_entry_matches,unsigned)+i*2*q->domain_types;m->match_capacity=2*q->domain_types;
  m->custody.visits=REGION(file_entry_visits,sl_selinux_rule_visit)+i*q->reach_visits;m->custody.capacity=q->reach_visits;
  m->custody.helpers=REGION(file_entry_helpers,sl_selinux_helper_visit)+i*q->reach_helpers;m->custody.helper_capacity=q->reach_helpers;m->custody.string_capacity=q->reach_strings;
 }
 s->candidate_preexec=REGION(candidate_preexec,sl_linux_candidate_preexec);s->candidate_preexec_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_candidate_preexec *e=&s->candidate_preexec[i];e->payload_original=&s->candidate_payloads[i];e->owner_original=&s->contexts[i];e->query_capacity=q->self_context_bytes;
  if(a->mapping.arguments[3]==(MAP_SHARED|MAP_ANONYMOUS)){
   e->containing_range.mapping_original=&a->mapping;
   e->containing_range.offset=l->preexec_shared_backing.offset+i*l->preexec_shared_stride;
   e->containing_range.bytes=l->preexec_shared_stride;
  }
  e->bounds.visited=REGION(preexec_bounds,type_datum_t *)+i*q->domain_types;e->bounds.capacity=q->domain_types;
  sl_linux_selinux_context *p=&e->child_context;p->parse_original=&s->parses[i];
  p->memory.bytes=REGION(preexec_memory,unsigned char)+i*q->parser_bytes;p->memory.capacity=q->parser_bytes;
  p->memory.attempts=REGION(preexec_attempts,sl_sepol_memory_attempt)+i*q->parser_attempts;p->memory.attempt_capacity=q->parser_attempts;atomic_init(&p->memory.claimed,0);
  p->diagnostics.frames=REGION(preexec_diagnostics,sl_sepol_diagnostic)+i*q->diagnostics;p->diagnostics.capacity=q->diagnostics;p->diagnostics.format_capacity=q->diagnostic_format;atomic_init(&p->diagnostics.claimed,0);
  for(j=0;j<q->diagnostics;j++){
   sl_sepol_diagnostic *d=&p->diagnostics.frames[j];d->arguments=REGION(preexec_arguments,sl_sepol_argument)+(i*q->diagnostics+j)*q->diagnostic_arguments;d->argument_capacity=q->diagnostic_arguments;
   d->output=REGION(preexec_outputs,char)+(i*q->diagnostics+j)*q->diagnostic_bytes;d->output_capacity=q->diagnostic_bytes;
  }
 }
 s->candidate_socket_uses=REGION(candidate_socket_uses,sl_linux_candidate_socket_create);s->candidate_socket_use_capacity=4*q->selinux_rows;
 for(i=0;i<4*q->selinux_rows;i++){
  unsigned long policy=i/4,peer=(i/2)%2;sl_linux_candidate_socket_create *p=&s->candidate_socket_uses[i];p->purpose=1+i%2;
  p->roles_original=&s->candidate_roles[policy];p->owner_original=&s->contexts[policy];p->payload_original=&s->candidate_payloads[policy].reachable;p->socket_original=&s->candidate_socket_contexts[2*policy+peer];
  p->sources=REGION(socket_use_classifications,sl_candidate_route_source)+i*q->domain_types;p->capacity=q->domain_types;
  p->inventory.sources=REGION(socket_use_sources,sl_selinux_source_access)+i*q->domain_types;p->inventory.source_capacity=q->domain_types;
  p->inventory.custody.visits=REGION(socket_use_visits,sl_selinux_rule_visit)+i*q->access_visits;p->inventory.custody.capacity=q->access_visits;
  p->inventory.custody.helpers=REGION(socket_use_helpers,sl_selinux_helper_visit)+i*q->access_helpers;p->inventory.custody.helper_capacity=q->access_helpers;p->inventory.custody.string_capacity=q->access_strings;
 }
 s->candidate_socket_creates=REGION(candidate_socket_creates,sl_linux_candidate_socket_create);s->candidate_socket_create_capacity=2*q->selinux_rows;
 for(i=0;i<2*q->selinux_rows;i++){
  unsigned long policy=i/2;sl_linux_candidate_socket_create *p=&s->candidate_socket_creates[i];p->roles_original=&s->candidate_roles[policy];p->owner_original=&s->contexts[policy];p->payload_original=&s->candidate_payloads[policy].reachable;p->socket_original=&s->candidate_socket_contexts[i];
  p->sources=REGION(socket_create_classifications,sl_candidate_route_source)+i*q->domain_types;p->capacity=q->domain_types;
  p->inventory.sources=REGION(socket_create_sources,sl_selinux_source_access)+i*q->domain_types;p->inventory.source_capacity=q->domain_types;
  p->inventory.custody.visits=REGION(socket_create_visits,sl_selinux_rule_visit)+i*q->access_visits;p->inventory.custody.capacity=q->access_visits;
  p->inventory.custody.helpers=REGION(socket_create_helpers,sl_selinux_helper_visit)+i*q->access_helpers;p->inventory.custody.helper_capacity=q->access_helpers;p->inventory.custody.string_capacity=q->access_strings;
 }
 s->candidate_ptrace=REGION(candidate_ptrace,sl_linux_candidate_ptrace);s->candidate_ptrace_capacity=2*q->selinux_rows;
 for(i=0;i<2*q->selinux_rows;i++){
  unsigned long policy=i/2;sl_linux_candidate_ptrace *p=&s->candidate_ptrace[i];p->roles_original=&s->candidate_roles[policy];p->owner_original=&s->contexts[policy];p->payload_original=&s->candidate_payloads[policy].reachable;
  p->sources=REGION(ptrace_classifications,sl_candidate_route_source)+i*q->domain_types;p->capacity=q->domain_types;
  p->inventory.sources=REGION(ptrace_sources,sl_selinux_source_access)+i*q->domain_types;p->inventory.source_capacity=q->domain_types;
  p->inventory.custody.visits=REGION(ptrace_visits,sl_selinux_rule_visit)+i*q->access_visits;p->inventory.custody.capacity=q->access_visits;
  p->inventory.custody.helpers=REGION(ptrace_helpers,sl_selinux_helper_visit)+i*q->access_helpers;p->inventory.custody.helper_capacity=q->access_helpers;p->inventory.custody.string_capacity=q->access_strings;
 }
 s->candidate_owner_entry=REGION(candidate_owner_entry,sl_linux_candidate_ptrace);s->candidate_owner_entry_capacity=4*q->selinux_rows;
 for(i=0;i<4*q->selinux_rows;i++){
  unsigned long policy=i/4;sl_linux_candidate_ptrace *p=&s->candidate_owner_entry[i];p->roles_original=&s->candidate_roles[policy];p->owner_original=&s->contexts[policy];p->payload_original=&s->candidate_payloads[policy].reachable;
  p->target_original=i%4>=2?&s->candidate_payloads[policy].context:&s->contexts[policy];
  p->sources=REGION(owner_entry_classifications,sl_candidate_route_source)+i*q->domain_types;p->capacity=q->domain_types;
  p->inventory.sources=REGION(owner_entry_sources,sl_selinux_source_access)+i*q->domain_types;p->inventory.source_capacity=q->domain_types;
  p->inventory.custody.visits=REGION(owner_entry_visits,sl_selinux_rule_visit)+i*q->access_visits;p->inventory.custody.capacity=q->access_visits;
  p->inventory.custody.helpers=REGION(owner_entry_helpers,sl_selinux_helper_visit)+i*q->access_helpers;p->inventory.custody.helper_capacity=q->access_helpers;p->inventory.custody.string_capacity=q->access_strings;
 }
 s->mounts.bytes=REGION(mountinfo_bytes,unsigned char);s->mounts.capacity=q->mountinfo_bytes;
 s->mounts.reads=REGION(mountinfo_reads,sl_linux_call);s->mounts.read_capacity=q->mountinfo_attempts;
 s->mounts.rows=REGION(mountinfo_rows,sl_linux_mount_row);s->mounts.row_capacity=q->mountinfo_rows;
 s->membership.path=REGION(membership_path,char);s->membership.path_capacity=q->membership_path;
 s->installation.executable_path=REGION(executable_path,char);s->installation.executable_capacity=q->executable_path;
 s->installation.directory_path=REGION(directory_path,char);s->installation.directory_capacity=q->directory_path;
 s->immutable.frames=REGION(image_frames,sl_linux_payload_frame);s->immutable.capacity=q->image_frames;
 for(i=0;i<q->image_frames;i++){
  sl_linux_payload_frame *f=&s->immutable.frames[i];
  f->source_bytes=REGION(image_source,unsigned char)+i*q->image_bytes;f->readback_bytes=REGION(image_readback,unsigned char)+i*q->image_bytes;f->capacity=q->image_bytes;
  f->writes=REGION(image_writes,sl_linux_payload_attempt)+i*q->image_writes;f->write_capacity=q->image_writes;
  f->readbacks=REGION(image_readbacks,sl_linux_payload_attempt)+i*q->image_readbacks;f->readback_capacity=q->image_readbacks;
 }
 s->archive_accesses=REGION(archive_accesses,sl_linux_archive_access);s->archive_access_capacity=SL_ARCHIVE_PURPOSE_COUNT*q->selinux_rows;
 for(i=0;i<s->archive_access_capacity;i++){
  sl_linux_archive_access *o=&s->archive_accesses[i];unsigned long purpose=i/q->selinux_rows,policy=i%q->selinux_rows,kind=purpose<6?0:purpose<12?1:2;
  o->startup_original=s;o->purpose=purpose;o->policy_index=policy;o->context_original=&s->archive_contexts[kind*q->selinux_rows+policy];o->owner_original=&s->contexts[policy];o->roles_original=&s->candidate_roles[policy];o->payload_original=&s->candidate_payloads[policy].reachable;
  if(purpose>=SL_ARCHIVE_CREATION_PURPOSE_FIRST)o->creation_original=REGION(archive_creations,sl_linux_archive_creation)+policy;
  o->sources=REGION(archive_sources,sl_linux_archive_source)+i*q->domain_types;o->capacity=q->domain_types;
  o->inventory.sources=REGION(archive_inventory_sources,sl_selinux_source_access)+i*q->domain_types;o->inventory.source_capacity=q->domain_types;
  o->inventory.custody.visits=REGION(archive_visits,sl_selinux_rule_visit)+i*q->access_visits;o->inventory.custody.capacity=q->access_visits;
  o->inventory.custody.helpers=REGION(archive_helpers,sl_selinux_helper_visit)+i*q->access_helpers;o->inventory.custody.helper_capacity=q->access_helpers;o->inventory.custody.string_capacity=q->access_strings;
 }
 s->archive_creations=REGION(archive_creations,sl_linux_archive_creation);s->archive_creation_capacity=q->selinux_rows;
 for(i=0;i<q->selinux_rows;i++){
  sl_linux_archive_creation *o=&s->archive_creations[i];o->startup_original=s;o->policy_index=i;o->directory_original=&s->archive_contexts[2*q->selinux_rows+i];o->owner_original=&s->contexts[i];o->fscreate_original=&s->archive_fscreate_contexts[i];
  o->types.types=REGION(archive_creation_types,unsigned)+i*q->domain_types;o->types.referents=REGION(archive_creation_referents,type_datum_t*)+i*q->domain_types;o->types.capacity=q->domain_types;
  o->types.custody.visits=REGION(archive_creation_visits,sl_selinux_rule_visit)+i*q->access_visits;o->types.custody.capacity=q->access_visits;
  o->types.custody.helpers=REGION(archive_creation_helpers,sl_selinux_helper_visit)+i*q->access_helpers;o->types.custody.helper_capacity=q->access_helpers;o->types.custody.string_capacity=q->access_strings;
 }
 c->schedule.controller_original=c;c->schedule.wakes=REGION(schedule_wakes,sl_linux_schedule_wake);
 c->schedule.wake_capacity=l->schedule_wakes.bytes/sizeof(sl_linux_schedule_wake);
 c->schedule.waits=REGION(schedule_waits,sl_linux_schedule_wait);c->schedule.wait_capacity=2*q->services;c->schedule.interval_ns=q->scheduler_interval_ns;
 unsigned long wake_index=0;
 for(i=0;i<3;i++){
  for(j=0;j<q->raw_frames[i];j++){
   sl_linux_capture_frame *f=&c->readers[i].frames[j];f->schedule_wake=&c->schedule.wakes[wake_index++];f->schedule_wake->source_original=f;
  }
  c->readers[i].terminal_wake=&c->schedule.wakes[wake_index++];c->readers[i].terminal_wake->source_original=&c->readers[i];
  c->forwarders[i].terminal_wake=&c->schedule.wakes[wake_index++];c->forwarders[i].terminal_wake->source_original=&c->forwarders[i];
 }
 c->collector_wake=&c->schedule.wakes[wake_index++];c->collector_wake->source_original=&c->channel;
 c->final_reader_task.terminal_wake=&c->schedule.wakes[wake_index++];c->final_reader_task.terminal_wake->source_original=&c->final_reader_task;
 for(i=0;i<q->services;i++){c->final_reader_joins[i].reader_original=&c->final_reader_task;c->final_reader_joins[i].wait_original=&c->schedule.waits[q->services+i];c->schedule.waits[q->services+i].schedule_original=&c->schedule;}
 for(i=0;i<q->services;i++){sl_linux_family_worker *f=&c->service_roster[i].family;f->terminal_wake=&c->schedule.wakes[wake_index++];f->terminal_wake->source_original=f;c->schedule.waits[i].schedule_original=&c->schedule;c->service_roster[i].schedule_original=&c->schedule.waits[i];}
 for(i=0;i<q->requests;i++){sl_linux_input_worker *f=&c->channel.requests[i].input;f->terminal_wake=&c->schedule.wakes[wake_index++];f->terminal_wake->source_original=f;}
 for(i=0;i<wake_index;i++)c->schedule.wakes[i].schedule_original=&c->schedule;
 a->bound=1;
}
#undef REGION
static int arena_fresh(const void *p,unsigned long bytes){
 const unsigned char *b=p;for(unsigned long i=0;i<bytes;i++)if(b[i])return 0;return 1;
}
static int arena_disjoint(const void *p,unsigned long bytes,const void *q,unsigned long extent){
 uintptr_t a=(uintptr_t)p,b=(uintptr_t)q;
 return p&&q&&bytes<=UINTPTR_MAX-a&&extent<=UINTPTR_MAX-b&&!(a<b+extent&&b<a+bytes);
}
long sl_linux_plan_candidate_arena(sl_linux_candidate_layout_plan *p){
 if(!p||!p->quantities_original||!p->candidate_original||sizeof(*p)>UINTPTR_MAX-(uintptr_t)p||
    !arena_fresh(&p->actual_quantities,sizeof(*p)-offsetof(sl_linux_candidate_layout_plan,actual_quantities))||
    !arena_disjoint(p,sizeof(*p),p->quantities_original,sizeof(*p->quantities_original))||
    !arena_disjoint(p,sizeof(*p),p->candidate_original,sizeof(*p->candidate_original)))return -1;
 sl_candidate_profile *c=p->candidate_original;
 if(!c->returned||c->result||!c->source||!c->source_extent||c->source_extent>LONG_MAX||
    c->original.bytes!=c->source||c->original.extent!=c->source_extent||
    !arena_disjoint(p,sizeof(*p),c->source,c->source_extent))return -1;
 for(unsigned i=0;i<5;i++){
  unsigned long count=c->actual_capacities[i];
  if(count>ULONG_MAX/sizeof(sl_candidate_record)||
     (count&&!arena_disjoint(p,sizeof(*p),c->actual_rows[i],count*sizeof(sl_candidate_record))))return -1;
 }
 p->attempted=1;p->result=-1;p->actual_quantities=p->quantities_original;p->actual_candidate=c;
 p->source_original=c->source;p->source_extent=c->source_extent;p->protected_rows=c->actual_capacities[3];
 p->prepared_quantities=*p->actual_quantities;
 if(!arena_layout(&p->layout,&p->prepared_quantities,p->source_extent,p->protected_rows))p->result=0;
 p->returned=1;return p->result;
}
static long arena_construct(sl_linux_candidate_arena *a,int sharing){
 long result;
 if(!a||a->attempted||(sharing!=MAP_PRIVATE&&sharing!=MAP_SHARED))return -1;
 /* Quantities, candidate pointer and optional preceding plan are inputs.
  * Reject all dirty output bytes and transitive source overlap BEFORE writes;
  * an alias refusal must not first mutate an original by marking attempted. */
 if(sizeof(*a)>UINTPTR_MAX-(uintptr_t)a||
    !arena_fresh(&a->prepared_quantities,sizeof(a->prepared_quantities))||
    !arena_fresh(&a->layout,sizeof(*a)-offsetof(sl_linux_candidate_arena,layout)))return -1;
 sl_candidate_profile *candidate=a->candidate_original;
 if(!candidate||!arena_disjoint(a,sizeof(*a),candidate,sizeof(*candidate)))return -1;
 if(!candidate->returned||candidate->result||!candidate->source||!candidate->source_extent||candidate->source_extent>LONG_MAX||candidate->original.bytes!=candidate->source||candidate->original.extent!=candidate->source_extent||
    !arena_disjoint(a,sizeof(*a),candidate->source,candidate->source_extent))return -1;
 for(unsigned i=0;i<5;i++){
  unsigned long count=candidate->actual_capacities[i];
  if(count>ULONG_MAX/sizeof(sl_candidate_record)||
     (count&&!arena_disjoint(a,sizeof(*a),candidate->actual_rows[i],count*sizeof(sl_candidate_record))))return -1;
 }
 const sl_linux_candidate_layout_plan *plan=a->plan_original;
 if(plan&&(!arena_disjoint(a,sizeof(*a),plan,sizeof(*plan))||
    !plan->attempted||!plan->returned||plan->result||!plan->quantities_original||
    plan->actual_quantities!=plan->quantities_original||plan->actual_candidate!=candidate||plan->candidate_original!=candidate||
    plan->source_original!=candidate->source||plan->source_extent!=candidate->source_extent||plan->protected_rows!=candidate->actual_capacities[3]||
    !arena_disjoint(a,sizeof(*a),plan->actual_quantities,sizeof(*plan->actual_quantities))||
    memcmp(&plan->prepared_quantities,plan->actual_quantities,sizeof(plan->prepared_quantities))||
    memcmp(&a->quantities,&plan->prepared_quantities,sizeof(a->quantities))))return -1;
 a->attempted=1;a->result=-1;a->prepared_quantities=a->quantities;
 if(arena_layout(&a->layout,&a->prepared_quantities,candidate->source_extent,candidate->actual_capacities[3]))goto done;
 if(plan&&memcmp(&a->layout,&plan->layout,sizeof(a->layout)))goto done;
 a->layout_complete=1;
 atomic_init(&a->mapping.entered,0);atomic_init(&a->mapping.returned,0);
 result=sl_linux_call6(&a->mapping,sharing==MAP_SHARED?"outer.candidate-arena.shared.mmap":"public.candidate-arena.mmap",SYS_mmap,0,a->layout.bytes,PROT_READ|PROT_WRITE,sharing|MAP_ANONYMOUS,-1,0);
 a->raw_mapping_result=result;a->mapping_returned=1;a->result=result;
 if(result<0)goto done;
 a->base=(void*)result;
 if(!result){a->result=-1;goto done;}
 /* Fresh anonymous mapping supplies zero-initialized backing. All disjoint
  * region extents and pointer products were checked before the syscall. */
 arena_bind(a);a->result=sl_linux_prepare_schedule(&a->startup->controller_original->schedule);
done:
 a->returned=1;return a->result;
}
long sl_linux_construct_candidate_arena(sl_linux_candidate_arena *a){
 return arena_construct(a,MAP_PRIVATE);
}
/* Parameterized source construction only. The actual root itself and ALL
 * required external output targets must be observer-held before creator
 * birth. This new wrapper is not yet an activated observer/factory caller. */
long sl_linux_construct_shared_candidate_arena(sl_linux_candidate_arena *a){
 return arena_construct(a,MAP_SHARED);
}
#endif
