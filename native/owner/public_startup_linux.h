#ifndef SERVICE_LASSO_PUBLIC_STARTUP_LINUX_H
#define SERVICE_LASSO_PUBLIC_STARTUP_LINUX_H
#include "public_controller_linux.h"
#include "local_installation_linux.h"
#include "local_membership_linux.h"
#include "local_mounts_linux.h"
#include "mount_location_linux.h"
#include "cgroup_children_linux.h"
#include "cgroup_control_label_linux.h"
#include "selinux_snapshot_linux.h"
#include "policy_parse_linux.h"
#include "selinux_context_linux.h"
#include "bound_domains_linux.h"
#include "control_access_linux.h"
#include "context_reachability_linux.h"
#include "immutable_payload_linux.h"
#include "capability_domain_linux.h"
#include "candidate_policy_linux.h"
#include "candidate_payload_context_linux.h"
#include "candidate_routes_linux.h"
#include "candidate_entry_linux.h"
#include "exec_label_linux.h"
#include "candidate_file_entry_linux.h"
#include "candidate_socket_context_linux.h"
#include "candidate_socket_create_linux.h"
#include "candidate_target_linux.h"
#include "archive_directory_linux.h"
#include "archive_files_linux.h"
#include "archive_reader_linux.h"
#include "archive_label_linux.h"
#include "archive_access_linux.h"
#include "migration_labels_linux.h"
#include "genfs_correspondence_linux.h"
/* Stage identifies the actual attempted source body, never admission. */
enum sl_linux_startup_stage {
 SL_STARTUP_UNENTERED=0,SL_STARTUP_ROOTS,SL_STARTUP_STDIO,
 SL_STARTUP_INSTALLATION,SL_STARTUP_MEMBERSHIP,SL_STARTUP_CAP_DOMAIN,SL_STARTUP_IMMUTABLE,SL_STARTUP_EXEC_LABELS,
 SL_STARTUP_MOUNTS,SL_STARTUP_LOCATIONS,SL_STARTUP_CHILDREN,SL_STARTUP_CONTROL_LABELS,SL_STARTUP_SELINUX_SNAPSHOTS,SL_STARTUP_POLICY_PARSE,SL_STARTUP_SELF_CONTEXT,SL_STARTUP_CONTEXT_PARSE,SL_STARTUP_EXEC_CONTEXTS,SL_STARTUP_BOUND_DOMAINS,SL_STARTUP_CONTROL_CONTEXTS,SL_STARTUP_CONTROL_ACCESS,SL_STARTUP_REACHABILITY,SL_STARTUP_ACQUIRED
};
typedef struct sl_linux_public_startup {
 sl_linux_archive_directory archive_directory;
 sl_linux_archive_files archive_files;sl_linux_archive_read_view final_reader;
 sl_linux_archive_label archive_labels[3],*archive_label_failure;
 unsigned archive_labels_attempted,archive_labels_returned,archive_labels_count;long archive_labels_result;
 sl_linux_selinux_context *archive_contexts,*actual_archive_contexts;unsigned long archive_context_capacity,actual_archive_context_capacity,archive_context_count;
 unsigned archive_contexts_attempted,archive_contexts_returned;long archive_contexts_result;
 sl_linux_archive_access *archive_accesses;unsigned long archive_access_capacity,archive_access_count;
 unsigned archive_accesses_attempted,archive_accesses_returned;long archive_accesses_result;
 unsigned archive_precreation_attempted,archive_precreation_returned;long archive_precreation_result;
 sl_linux_archive_creation *archive_creations;unsigned long archive_creation_capacity;
 sl_linux_control_roster control_roster;
 sl_linux_genfs_correspondence *genfs_correspondences;unsigned long genfs_correspondence_capacity;
 sl_linux_migration_labels *migration_labels;unsigned long migration_label_capacity;
 sl_linux_migration_access *migration_access;unsigned long migration_access_capacity;
 sl_linux_migration_routes *migration_routes;unsigned long migration_route_capacity;
 unsigned migration_routes_attempted,migration_routes_returned;long migration_routes_result;
 sl_linux_selinux_context *archive_fscreate_contexts;unsigned long archive_fscreate_context_capacity;

 sl_linux_candidate_target candidate_target;sl_linux_family_events initial_family;long initial_family_result;
 sl_linux_public_controller *controller_original;
 sl_candidate_profile *candidate_original,*actual_candidate;
 sl_linux_candidate_policy *candidate_policies,*actual_candidate_policies;unsigned long candidate_policy_capacity,actual_candidate_policy_capacity,candidate_policy_count;
 sl_linux_candidate_roles *candidate_roles,*actual_candidate_roles;unsigned long candidate_role_capacity,actual_candidate_role_capacity,candidate_role_count;
 sl_linux_candidate_payload_context *candidate_payloads,*actual_candidate_payloads;unsigned long candidate_payload_capacity,actual_candidate_payload_capacity,candidate_payload_count;
 sl_linux_candidate_routes *candidate_routes,*actual_candidate_routes;unsigned long candidate_route_capacity,actual_candidate_route_capacity,candidate_route_count;
 sl_linux_candidate_socket_context *candidate_socket_contexts,*actual_candidate_socket_contexts;unsigned long candidate_socket_context_capacity,actual_candidate_socket_context_capacity,candidate_socket_context_count;
 unsigned candidate_socket_context_attempted,candidate_socket_context_returned;long candidate_socket_context_result;
 sl_linux_candidate_socket_create *candidate_socket_creates,*actual_candidate_socket_creates;unsigned long candidate_socket_create_capacity,actual_candidate_socket_create_capacity,candidate_socket_create_count;
 sl_linux_candidate_socket_create *candidate_socket_uses;unsigned long candidate_socket_use_capacity,candidate_socket_use_count;
 unsigned candidate_socket_use_attempted,candidate_socket_use_returned;long candidate_socket_use_result;
 unsigned candidate_socket_create_attempted,candidate_socket_create_returned;long candidate_socket_create_result;
 sl_linux_candidate_file_entry *candidate_file_entry,*actual_candidate_file_entry;unsigned long candidate_file_entry_capacity,actual_candidate_file_entry_capacity,candidate_file_entry_count;
 unsigned candidate_file_entry_attempted,candidate_file_entry_returned;long candidate_file_entry_result;
 sl_linux_candidate_preexec *candidate_preexec,*actual_candidate_preexec;unsigned long candidate_preexec_capacity,actual_candidate_preexec_capacity;
 unsigned candidate_preexec_attempted,candidate_preexec_returned;long candidate_preexec_result;unsigned long candidate_preexec_selected;
 sl_linux_candidate_ptrace *candidate_ptrace,*actual_candidate_ptrace;unsigned long candidate_ptrace_capacity,actual_candidate_ptrace_capacity,candidate_ptrace_count;
 unsigned candidate_ptrace_attempted,candidate_ptrace_returned;long candidate_ptrace_result;
 sl_linux_candidate_ptrace *candidate_owner_entry,*actual_candidate_owner_entry;unsigned long candidate_owner_entry_capacity,actual_candidate_owner_entry_capacity,candidate_owner_entry_count;
 unsigned candidate_owner_entry_attempted,candidate_owner_entry_returned;long candidate_owner_entry_result;
 unsigned candidate_routes_attempted,candidate_routes_returned;long candidate_routes_result;
 unsigned candidate_roles_attempted,candidate_roles_returned;long candidate_roles_result;
 int argc;char **argv;char **environment;
 int actual_argc;char **actual_argv;char **actual_environment;
 sl_linux_public_controller *prepared_controller;
 sl_linux_local_installation installation;
 sl_linux_local_membership membership;
 sl_linux_local_mounts mounts;
 sl_linux_mount_location *locations,*actual_locations;
 unsigned long location_capacity,actual_location_capacity,location_count;
 sl_linux_cgroup_children *children,*actual_children;
 unsigned long children_capacity,actual_children_capacity,children_count;
 sl_linux_cgroup_control_label *controls,*actual_controls;
 unsigned long control_capacity,actual_control_capacity,control_count;
 unsigned control_closes_attempted,control_closes_returned;long control_close_result;sl_linux_cgroup_control_label *control_close_primary;
 sl_linux_selinux_snapshot *policies,*actual_policies;
 unsigned long policy_capacity,actual_policy_capacity,policy_count;
 sl_linux_policy_parse *parses,*actual_parses;
 unsigned long parse_capacity,actual_parse_capacity,parse_count;
 sl_linux_lsm_self self_context,archive_fscreate;long archive_fscreate_result;
 sl_linux_selinux_context *contexts,*actual_contexts;
 unsigned long context_capacity,actual_context_capacity,context_count;long self_context_result;
 sl_linux_selinux_context *exec_contexts,*actual_exec_contexts;unsigned long exec_context_capacity,actual_exec_context_capacity,exec_context_count;
 sl_linux_selinux_context *control_contexts,*actual_control_contexts;unsigned long control_context_capacity,actual_control_context_capacity,control_context_count;
 sl_linux_control_access *control_access,*actual_control_access;unsigned long control_access_capacity,actual_control_access_capacity,control_access_count;
 sl_linux_context_reachability *reachability,*actual_reachability;unsigned long reachability_capacity,actual_reachability_capacity,reachability_count;
 sl_linux_bound_domains *domains,*actual_domains;
 unsigned long domain_capacity,actual_domain_capacity,domain_count;
 sl_linux_immutable_payload immutable;
 sl_linux_exec_label public_label,payload_label;long public_label_result,payload_label_result;
 sl_linux_capability_domain capability_domain;
 long stdio_result,installation_result,membership_result,capability_result,immutable_result,mounts_result,result;
 enum sl_linux_startup_stage stage;
 unsigned attempted,returned;
} sl_linux_public_startup;
/* Caller supplies source-owned fresh finite backing in the embedded
 * acquisition owners before entry. The actual public main and complete native
 * profile are still separate source obligations. This operation neither
 * selects a cgroup nor calls clone3, and never releases the child gate. */
long sl_linux_acquire_public_startup(sl_linux_public_startup *startup);
/* Requires actual selected last borrowers settled; no current eligibility grant. */
long sl_linux_bind_startup_candidate_roles(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_candidate_routes(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_candidate_ptrace(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_candidate_owner_entry(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_candidate_file_entry(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_socket_contexts(sl_linux_public_startup *startup);
long sl_linux_bind_controller_preexec(sl_linux_public_startup *startup);
long sl_linux_close_cgroup_controls(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_socket_creates(sl_linux_public_startup *startup);
long sl_linux_interpret_startup_socket_uses(sl_linux_public_startup *startup);
long sl_linux_observe_archive_directory_before_creation(sl_linux_public_startup *);
long sl_linux_interpret_archive_precreation(sl_linux_public_startup *);
#endif
