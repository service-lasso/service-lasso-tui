#ifndef SERVICE_LASSO_CANDIDATE_FILE_ENTRY_LINUX_H
#define SERVICE_LASSO_CANDIDATE_FILE_ENTRY_LINUX_H
#include "candidate_payload_context_linux.h"
#include "candidate_exec_context_linux.h"
typedef struct sl_linux_candidate_file_entry {
 sl_linux_candidate_policy *policy_original,*actual_policy;
 sl_linux_selinux_context *owner_original,*actual_owner,*public_original,*actual_public,*file_original,*actual_file;
 sl_linux_candidate_payload_context *payload_original,*actual_payload;
 const sl_candidate_record *declarations[2],*evidence[2];sl_candidate_span statements[2];
 uint64_t evidence_index[2],statement_offset[2],statement_extent[2],declared_type[2];unsigned long matches[2];
 sl_selinux_transition_matrix matrix;
 sl_selinux_exec_type exec_type;long exec_type_result;
 sl_linux_candidate_exec_context exec_context;long exec_context_result;
 sl_linux_selinux_status_sample after_entry;
 unsigned long examined;long matrix_result,primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_candidate_file_entry;
long sl_linux_interpret_candidate_file_entry(sl_linux_candidate_file_entry *owner);
#endif