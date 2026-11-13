#ifndef SERVICE_LASSO_DOMAIN_REACHABILITY_H
#define SERVICE_LASSO_DOMAIN_REACHABILITY_H
#include "selinux_policy_relations.h"
typedef struct sl_selinux_reach_step {
 type_datum_t *source_original,*target_original;
 sl_selinux_transition_edge *edge_original;unsigned source,target;
 unsigned source_before,source_after,target_before,target_after;
} sl_selinux_reach_step;
typedef struct sl_selinux_reachability {
 policydb_t *policy_original,*actual_policy;unsigned seed,actual_seed;
 sl_selinux_transition_matrix matrix;int matrix_result;
 unsigned *reached,*actual_reached;unsigned long type_capacity,actual_type_capacity;
 sl_selinux_reach_step *steps,*actual_steps;unsigned long step_capacity,actual_step_capacity,step_count,processed;
 unsigned attempted,returned;int result;
} sl_selinux_reachability;
/* Single table union then process each reached source once; no exec authority. */
int sl_selinux_interpret_reachability(sl_selinux_reachability *root);
#endif