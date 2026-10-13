#ifndef SERVICE_LASSO_CANDIDATE_SOCKET_CONTEXT_LINUX_H
#define SERVICE_LASSO_CANDIDATE_SOCKET_CONTEXT_LINUX_H
#include "candidate_policy_linux.h"
#include "selinux_context_linux.h"
typedef struct {
 sl_linux_candidate_policy *policy_original,*actual_policy;
 sl_linux_socket_peer *peer_original,*actual_peer;
 const sl_candidate_record *declaration_original,*evidence_original;
 sl_candidate_span statement_original,desired_original;
 uint64_t evidence_index,statement_offset,statement_extent;
 sl_linux_selinux_context context;
 size_t declarations_examined,matches,bytes_compared;
 long context_result,result;unsigned attempted,returned;
} sl_linux_candidate_socket_context;
long sl_linux_interpret_candidate_socket_context(sl_linux_candidate_socket_context *owner);
#endif