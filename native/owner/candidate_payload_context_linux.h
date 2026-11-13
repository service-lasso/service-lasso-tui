#ifndef SERVICE_LASSO_CANDIDATE_PAYLOAD_CONTEXT_LINUX_H
#define SERVICE_LASSO_CANDIDATE_PAYLOAD_CONTEXT_LINUX_H
#include "candidate_policy_linux.h"
#include "context_reachability_linux.h"
typedef struct {
 sl_linux_candidate_policy *policy_original,*actual_policy;
 const sl_candidate_record *declaration_original,*evidence_original;
 sl_candidate_span statement_original,desired_original;
 uint64_t evidence_index,statement_offset,statement_extent;
 sl_linux_selinux_context context;
 sl_linux_context_reachability reachable;
 size_t declarations_examined;long context_result,graph_result,status_result,result;
 unsigned attempted,returned;
} sl_linux_candidate_payload_context;
/* Desired source context and overapproximation only; never actual child CURRENT. */
long sl_linux_interpret_candidate_payload_context(sl_linux_candidate_payload_context *owner);
#endif