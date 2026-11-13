#ifndef SERVICE_LASSO_CANDIDATE_ARENA_LINUX_H
#define SERVICE_LASSO_CANDIDATE_ARENA_LINUX_H
#include "public_startup_linux.h"
#include "deadline_linux.h"
#include "deadline_poll_linux.h"
#include "creator_file_linux.h"
#include "creator_elf_linux.h"
#include "public_inputs_linux.h"
typedef struct sl_linux_candidate_quantities {
 unsigned long raw_frames[3],public_writes[3],forward_waits[3],forward_iterations[3];
 unsigned long scheduler_interval_ns,services,outer_observations,collector_joins,requests,prefix_attempts,terminal_reply_attempts;
 unsigned long deadline_roots,deadline_observations;
 unsigned long deadline_polls,deadline_poll_fds;
 unsigned long creator_file_bytes,creator_file_reads;
 unsigned long input_frames,input_steps,body_bytes,custody_attempts[5];
 unsigned long scan_frames,scan_bytes,fd_views,fd_actions;
 unsigned long membership_frames,membership_bytes,membership_path;
 unsigned long mountinfo_bytes,mountinfo_attempts,mountinfo_rows,mount_location_bytes;
 unsigned long selinux_rows,selinux_path,policy_frames,policy_frame_bytes,policy_body;
 unsigned long parser_bytes,parser_attempts,diagnostics,diagnostic_arguments,diagnostic_bytes,diagnostic_format;
 unsigned long self_context_bytes,domain_types,domain_steps;
 unsigned long access_visits,access_helpers,access_strings;
 unsigned long reach_visits,reach_helpers,reach_strings,reach_steps;
 unsigned long children_frames,children_bytes,children_views;
 unsigned long executable_path,directory_path,image_frames,image_bytes,image_writes,image_readbacks;
 unsigned long credential_status_bytes,credential_status_attempts,credential_group_capacity;
 unsigned long pidfd_info_bytes,pidfd_info_attempts;
 unsigned long public_argument_count,public_argument_bytes;
 unsigned long public_environment_count,public_environment_bytes;
 unsigned long security_label_bytes,terminal_calls,maintenance_role_rows;
 unsigned long archive_path_bytes,archive_declarations;
 unsigned long final_io_attempts[2];
} sl_linux_candidate_quantities;
typedef struct sl_linux_arena_region {unsigned long offset,bytes;} sl_linux_arena_region;
typedef struct sl_linux_candidate_layout {
 sl_linux_arena_region public_main_owner,outer_birth,outer_continuation,outer_supervisor,outer_turns,outer_observations;
 sl_linux_arena_region failed_parent_identity,failed_archive_header;
 sl_linux_arena_region failed_parent_directory,failed_parent_path,failed_parent_declarations;
 sl_linux_arena_region failed_parent_writer,failed_parent_reader;
 sl_linux_arena_region public_input_source,public_inputs,public_argument_views,public_environment_views;
 sl_linux_arena_region deadline_sources,deadlines,deadline_sequences,deadline_observations;
 sl_linux_arena_region deadline_polls,deadline_poll_descriptors,deadline_poll_outputs;
 sl_linux_arena_region creator_file,creator_file_body,creator_file_reads;
 sl_linux_arena_region creator_elf,creator_elf_programs;
 /* Prebirth containing storage; these are ranges, not nested mmap outputs. */
 sl_linux_arena_region bootstrap_backing,fd_action_backing;
 sl_linux_arena_region credential_status_backing[2],credential_read_backing[2],credential_group_backing[2];
 sl_linux_arena_region preexec_shared_backing;
 unsigned long preexec_shared_stride,preexec_source_extent;
 sl_linux_arena_region startup,controller,readers,forwarders,schedule_wakes,schedule_waits;
 sl_linux_arena_region private_arguments,argument_lengths;
 sl_linux_arena_region raw_frames[3],public_writes[3],waits[3],iterations[3],publications[3];
 sl_linux_arena_region services,raw_joins,releases,forward_joins,collector_joins;
 sl_linux_arena_region requests,prefix_calls,reply_calls,input_frames,input_steps;
 sl_linux_arena_region body,readback,custody_calls[5],scan_frames,scan_bytes,fd_views,fd_actions;
 sl_linux_arena_region final_records,final_readback,final_calls[2];
 sl_linux_arena_region independent_readback,independent_reads,independent_joins;
 sl_linux_arena_region archive_labels;
 sl_linux_arena_region archive_contexts,archive_context_memory,archive_context_attempts,archive_context_diagnostics,archive_context_arguments,archive_context_outputs;
 sl_linux_arena_region archive_accesses,archive_inventory_sources,archive_sources,archive_visits,archive_helpers;
 sl_linux_arena_region archive_creations,archive_creation_types,archive_creation_referents,archive_creation_visits,archive_creation_helpers;
 sl_linux_arena_region membership_frames,membership_bytes,membership_path,executable_path,directory_path;
 sl_linux_arena_region mountinfo_bytes,mountinfo_reads,mountinfo_rows;
 sl_linux_arena_region mount_locations,mount_roots,mount_points,mount_relatives;
 sl_linux_arena_region selinux_rows,policies,policy_paths,policy_frames,policy_frame_bytes,policy_body;
 sl_linux_arena_region archive_path,archive_declarations,archive_statuses,archive_file_statuses,target_statuses,candidate_policies,candidate_digest_workspaces,candidate_roles,candidate_role_bindings;
 sl_linux_arena_region candidate_payloads,payload_memory,payload_attempts,payload_diagnostics,payload_arguments,payload_outputs;
 sl_linux_arena_region payload_edges,payload_matches,payload_visits,payload_helpers,payload_types,payload_steps,candidate_routes,candidate_route_sources;
 sl_linux_arena_region candidate_file_entry,file_entry_edges,file_entry_matches,file_entry_visits,file_entry_helpers,exec_type_visits,exec_type_helpers,exec_context_visits;
 sl_linux_arena_region candidate_preexec,preexec_bounds,preexec_memory,preexec_attempts,preexec_diagnostics,preexec_arguments,preexec_outputs;
 sl_linux_arena_region custody_peer_labels;
 sl_linux_arena_region candidate_socket_uses,socket_use_sources,socket_use_classifications,socket_use_visits,socket_use_helpers;
 sl_linux_arena_region candidate_socket_creates,socket_create_sources,socket_create_classifications,socket_create_visits,socket_create_helpers;
 sl_linux_arena_region candidate_socket_contexts,socket_context_memory,socket_context_attempts,socket_context_diagnostics,socket_context_arguments,socket_context_outputs;
 sl_linux_arena_region candidate_owner_entry,owner_entry_sources,owner_entry_visits,owner_entry_helpers,owner_entry_classifications;
 sl_linux_arena_region candidate_ptrace,ptrace_sources,ptrace_visits,ptrace_helpers,ptrace_classifications;
 sl_linux_arena_region parses,parser_memory,parser_attempts,diagnostics,diagnostic_arguments,diagnostic_outputs;
 sl_linux_arena_region exec_labels,exec_contexts,exec_context_memory,exec_context_attempts,exec_context_diagnostics,exec_context_arguments,exec_context_outputs;
 sl_linux_arena_region self_context,archive_fscreate,contexts,context_memory,context_attempts,context_diagnostics,context_arguments,context_outputs;
 sl_linux_arena_region domains,domain_entries,domain_steps;
 sl_linux_arena_region control_access,access_sources,access_visits,access_helpers;
 sl_linux_arena_region migration_labels,migration_types,migration_referents,migration_helpers,migration_observed;
 sl_linux_arena_region migration_filesystem_types,migration_filesystem_referents;
 sl_linux_arena_region migration_access,migration_access_sources,migration_access_visits,migration_access_helpers;
 sl_linux_arena_region migration_routes,migration_route_sources,genfs_correspondences,genfs_declarations;
 sl_linux_arena_region reachability,reach_edges,reach_matches,reach_visits,reach_helpers,reach_types,reach_steps;
 sl_linux_arena_region control_contexts,control_context_memory,control_context_attempts,control_context_diagnostics,control_context_arguments,control_context_outputs;
 sl_linux_arena_region children,children_frames,children_bytes,children_views,controls,control_labels;
 sl_linux_arena_region image_frames,image_source,image_readback,image_writes,image_readbacks;
 sl_linux_arena_region pidfd_info_bytes,pidfd_info_reads;
 unsigned long bytes;
} sl_linux_candidate_layout;

typedef struct sl_linux_candidate_layout_plan {
 const sl_linux_candidate_quantities *quantities_original;
 sl_candidate_profile *candidate_original;
 const sl_linux_candidate_quantities *actual_quantities;
 sl_candidate_profile *actual_candidate;
 const unsigned char *source_original;
 unsigned long source_extent,protected_rows;
 sl_linux_candidate_quantities prepared_quantities;
 sl_linux_candidate_layout layout;
 long result;unsigned attempted,returned;
} sl_linux_candidate_layout_plan;
/* Pure typed source geometry only: no mmap, containing credit or source
 * qualification. Its static original itself is part of the full charge. */
long sl_linux_plan_candidate_arena(sl_linux_candidate_layout_plan *plan);
typedef struct sl_linux_candidate_arena {
 sl_linux_candidate_quantities quantities,prepared_quantities;
 sl_candidate_profile *candidate_original;
 const sl_linux_candidate_layout_plan *plan_original;
 sl_linux_candidate_layout layout;
 sl_linux_call mapping;long raw_mapping_result,result;
 unsigned attempted,layout_complete,mapping_returned,bound,returned;
 void *base;sl_linux_public_startup *startup;
 struct sl_linux_public_main_owner *public_main_owner;
 struct sl_linux_outer_birth *outer_birth;
 struct sl_linux_outer_continuation *outer_continuation;
 struct sl_linux_outer_supervisor *outer_supervisor;
 struct sl_linux_failed_parent_identity *failed_parent_identity;
 struct sl_linux_failed_archive_header *failed_archive_header;
 struct sl_linux_failed_parent_directory *failed_parent_directory;
 struct sl_linux_failed_parent_file *failed_parent_writer,*failed_parent_reader;
 unsigned char *failed_parent_path;unsigned long failed_parent_path_capacity;
 struct sl_linux_archive_declaration *failed_parent_declarations;unsigned long failed_parent_declaration_capacity;
 struct sl_linux_outer_terminal *outer_observations;unsigned long outer_observation_capacity;
 sl_linux_deadline_source *deadline_sources;
 sl_linux_deadline *deadlines;sl_linux_deadline_sequence *deadline_sequences;
 unsigned long deadline_capacity;
 sl_linux_deadline_poll *deadline_polls;
 unsigned long deadline_poll_capacity,deadline_poll_fd_capacity;
 sl_linux_creator_file *creator_file;
 sl_linux_public_inputs *public_inputs;
 sl_linux_creator_elf *creator_elf;
 sl_linux_service_observation *services;unsigned long service_capacity;
 sl_linux_collector_join *collector_joins;unsigned long collector_join_capacity;
} sl_linux_candidate_arena;
/* Candidate quantities are explicit source inputs, never defaults or measured
 * fit. This maps current Linux native data only; worker stacks/Go/Windows/FINAL
 * and genuine profile qualification remain independently unfinished. */
long sl_linux_construct_candidate_arena(sl_linux_candidate_arena *arena);
/* Same checked data layout, shared anonymous backing. Does not enroll the
 * caller root/private outputs or qualify creator fork/terminal custody. */
long sl_linux_construct_shared_candidate_arena(sl_linux_candidate_arena *arena);
#endif
