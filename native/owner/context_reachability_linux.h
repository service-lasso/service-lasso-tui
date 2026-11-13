#ifndef SERVICE_LASSO_CONTEXT_REACHABILITY_LINUX_H
#define SERVICE_LASSO_CONTEXT_REACHABILITY_LINUX_H
#include "selinux_context_linux.h"
#include "domain_reachability.h"
typedef struct sl_linux_context_reachability {
 sl_linux_selinux_context *context_original,*actual_context;
 sl_selinux_reachability graph;sl_linux_selinux_status_sample after_graph;
 int graph_result,status_result,result;unsigned attempted,returned;
} sl_linux_context_reachability;
int sl_linux_interpret_context_reachability(sl_linux_context_reachability *root);
#endif