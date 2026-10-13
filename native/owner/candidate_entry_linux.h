#ifndef SERVICE_LASSO_CANDIDATE_ENTRY_LINUX_H
#define SERVICE_LASSO_CANDIDATE_ENTRY_LINUX_H
#include "candidate_routes_linux.h"
typedef struct {
 sl_linux_candidate_roles *roles_original,*actual_roles;
 sl_linux_selinux_context *owner_original,*actual_owner,*target_original,*actual_target;
 sl_linux_context_reachability *payload_original,*actual_payload;
 const char *class_original,*permission_original;unsigned permission_kind;
 sl_selinux_access_sources inventory;
 sl_candidate_route_source *sources,*actual_sources;size_t capacity,actual_capacity,count;
 sl_linux_selinux_status_sample after_ptrace;
 long inventory_result,primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_candidate_ptrace;
/* Separate process/ptrace and task-SID file/read observations; exact proc routes and loaded-entry correspondence remain separate. */
long sl_linux_interpret_candidate_ptrace(sl_linux_candidate_ptrace *root);
long sl_linux_interpret_candidate_owner_proc_read(sl_linux_candidate_ptrace *root);
long sl_linux_interpret_candidate_owner_transition(sl_linux_candidate_ptrace *root);
long sl_linux_interpret_candidate_owner_dyntransition(sl_linux_candidate_ptrace *root);
long sl_linux_interpret_candidate_payload_transition(sl_linux_candidate_ptrace *root);
long sl_linux_interpret_candidate_payload_dyntransition(sl_linux_candidate_ptrace *root);
#endif