#ifndef SERVICE_LASSO_POLICY_PARSE_LINUX_H
#define SERVICE_LASSO_POLICY_PARSE_LINUX_H
#include "selinux_snapshot_linux.h"
#include "sepol_parse_owner.h"
typedef struct sl_linux_policy_parse {
 sl_linux_selinux_snapshot *snapshot_original,*actual_snapshot;
 sl_sepol_parse_owner parser;
 sl_linux_selinux_status_sample after_parse;int status_result;
 int result;unsigned attempted,returned;
} sl_linux_policy_parse;
int sl_linux_parse_policy_snapshot(sl_linux_policy_parse *root);
#endif