#ifndef SERVICE_LASSO_CANDIDATE_ROUTES_LINUX_H
#define SERVICE_LASSO_CANDIDATE_ROUTES_LINUX_H
#include "candidate_policy_linux.h"
#include "control_access_linux.h"
#include "context_reachability_linux.h"
typedef struct {
 sl_selinux_source_access *source_original;
 sl_candidate_role_binding *role_original,*first_role_original;
 const sl_candidate_record *evidence_original;
 sl_candidate_span statement_original;
 unsigned observed_type,payload_reachable,internal_payload_closure;size_t matching_roles,unsupported_scope_roles;
 unsigned attempted,returned;long result;
} sl_candidate_route_source;
typedef struct {
 sl_linux_candidate_roles *roles_original,*actual_roles;
 sl_linux_control_access *control_original,*actual_control;
 sl_linux_context_reachability *payload_original,*actual_payload;
 sl_candidate_route_source *sources,*actual_sources;size_t capacity,actual_capacity,count;
 sl_linux_selinux_status_sample after_routes;
 long primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_candidate_routes;
/* Migration-control relation only. No complete six-N/entry/admission grant. */
long sl_linux_interpret_candidate_routes(sl_linux_candidate_routes *routes);
#endif