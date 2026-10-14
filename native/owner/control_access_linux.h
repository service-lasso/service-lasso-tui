#ifndef SERVICE_LASSO_CONTROL_ACCESS_LINUX_H
#define SERVICE_LASSO_CONTROL_ACCESS_LINUX_H
#include "selinux_context_linux.h"
#include "selinux_policy_relations.h"
typedef enum sl_linux_control_access_kind { SL_CONTROL_WRITE_APPEND=0, SL_CONTROL_RELABEL_FROM=1 } sl_linux_control_access_kind;
typedef struct sl_linux_control_access {
 sl_linux_control_access_kind kind_original,actual_kind;
 sl_linux_selinux_context *context_original,*actual_context;
 sl_selinux_access_sources inventory;
 sl_linux_selinux_status_sample after_access;
 int inventory_result,status_result,result;unsigned attempted,returned;
} sl_linux_control_access;
/* Native label and policy vocabulary observations, not actor trust or gate. */
int sl_linux_interpret_control_access(sl_linux_control_access *root);
#endif