#ifndef SERVICE_LASSO_CANDIDATE_TARGET_LINUX_H
#define SERVICE_LASSO_CANDIDATE_TARGET_LINUX_H
#include "candidate_profile.h"
#include "cgroup_children_linux.h"
#include "selinux_status_linux.h"
#include "cgroup_control_label_linux.h"
struct sl_linux_public_startup;
typedef struct sl_linux_target_close_view {
 sl_linux_call *open_original;sl_linux_fd_lift *lift_original;int raw_fd,held_fd;
 sl_linux_call close_raw,close_held,*failure_original,*unsettled_original;
 unsigned attempted,returned,raw_attempted,held_attempted,unmapped;long result;
} sl_linux_target_close_view;
typedef struct sl_linux_candidate_target {
 struct sl_linux_public_startup *startup_original,*actual_startup;
 sl_candidate_profile *candidate_original;
 const sl_candidate_record *declaration_original,*first_declaration,*evidence_original;
 sl_candidate_span statement_original;
 uint64_t tuple[4],evidence_index,statement_offset,statement_extent;
 unsigned long declarations,children_examined,views_examined,control_episodes;
 sl_linux_cgroup_children *children_original;sl_linux_child_view *view_original;
 sl_linux_cgroup_control_label *procs_original;
 struct open_how target_how,procs_how;
 sl_linux_call open_target,open_procs,target_filesystem,procs_filesystem,target_identity,procs_identity;
 sl_linux_fd_lift target_lift,procs_lift;
 struct statfs target_fs,procs_fs;struct statx target_state,procs_state;
 sl_linux_selinux_status_sample *statuses,*actual_statuses;unsigned long status_capacity,actual_status_capacity,status_count;
 int raw_target_fd,target_fd,raw_procs_fd,procs_fd;
 sl_linux_selinux_status_sample *status_failure_original;long primary_result,status_result,result;sl_linux_target_close_view closes[2];unsigned closes_attempted,closes_returned;long close_result;sl_linux_target_close_view *close_failure_original;unsigned attempted,returned,bound;
} sl_linux_candidate_target;
long sl_linux_bind_candidate_target(sl_linux_candidate_target *root);
struct sl_linux_public_controller;
long sl_linux_close_failed_target_before_birth(struct sl_linux_public_controller *controller);
#endif