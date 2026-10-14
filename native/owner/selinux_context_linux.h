#ifndef SERVICE_LASSO_SELINUX_CONTEXT_LINUX_H
#define SERVICE_LASSO_SELINUX_CONTEXT_LINUX_H
#include "policy_parse_linux.h"
#include "lsm_self_linux.h"
#include "cgroup_control_label_linux.h"
#include "exec_label_linux.h"
#include "archive_label_linux.h"
/* Interpretation of exact retained bytes, not authentication of their native
 * producer. Constructor must bind disjoint fresh episode backing before entry. */
typedef struct sl_linux_selinux_context {
 sl_linux_policy_parse *parse_original,*actual_parse;
 sl_linux_cgroup_control_label *control_original,*actual_control;unsigned control_attempted,control_returned;int control_result;
 sl_linux_exec_label *exec_original,*actual_exec;unsigned exec_attempted,exec_returned;int exec_result;
 sl_linux_archive_label *archive_original,*actual_archive;unsigned archive_attempted,archive_returned;int archive_result;
 sl_linux_lsm_self *self_original,*actual_self;unsigned self_attempted,self_returned;int self_result;
 const unsigned char *body,*actual_body;size_t extent,actual_extent,examined;
 sl_sepol_memory_owner memory;sl_sepol_diagnostic_owner diagnostics;
 struct sepol_handle handle;
 context_struct_t *context_original;
 sl_linux_selinux_status_sample after_context;
 unsigned user,role,type;
 int memory_begin,diagnostic_begin,call_result,call_errno,status_result;
 int diagnostic_finish,memory_finish,result;
 unsigned attempted,call_entered,call_returned,returned;
} sl_linux_selinux_context;
int sl_linux_interpret_selinux_context(sl_linux_selinux_context *root);
int sl_linux_interpret_selinux_self(sl_linux_selinux_context *root,sl_linux_lsm_self *self);
int sl_linux_interpret_exec_context(sl_linux_selinux_context *root);
int sl_linux_interpret_control_context(sl_linux_selinux_context *root);
int sl_linux_interpret_archive_context(sl_linux_selinux_context *root);
#endif
