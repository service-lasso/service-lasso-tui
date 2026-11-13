#ifndef SERVICE_LASSO_CANDIDATE_PREEXEC_LINUX_H
#define SERVICE_LASSO_CANDIDATE_PREEXEC_LINUX_H
#include "candidate_payload_context_linux.h"
#include "shared_range_linux.h"
typedef struct sl_linux_candidate_preexec_shared {
 sl_linux_bootstrap *bootstrap_original;
 const sl_linux_selinux_context *owner_original;
 const sl_linux_candidate_payload_context *payload_original;
 sl_linux_lsm_self before,after,pending_exec;
 sl_linux_call mutation;
 unsigned char *input;unsigned long input_extent;
 sl_linux_lsm_header input_header;
 long before_result,mutation_result,after_result,exec_result,result;
 unsigned attempted;_Atomic unsigned returned;
} sl_linux_candidate_preexec_shared;
struct sl_linux_candidate_preexec_inspection;
typedef struct sl_linux_candidate_preexec {
 sl_linux_candidate_payload_context *payload_original,*actual_payload;
 sl_linux_selinux_context *owner_original,*actual_owner;
 sl_linux_bootstrap *bootstrap_original,*actual_bootstrap;
 sl_selinux_bounds bounds;long bounds_result;
 sl_linux_selinux_context child_context;
 struct sl_linux_candidate_preexec_inspection *inspected_by;
 unsigned long query_capacity,actual_query_capacity,query_stride,mapping_extent;
 sl_linux_call mapping;long raw_mapping,result;
 sl_linux_shared_range containing_range;
 sl_linux_candidate_preexec_shared *shared;
 unsigned attempted,returned;
} sl_linux_candidate_preexec;
typedef struct sl_linux_candidate_preexec_inspection {
 sl_linux_candidate_preexec *owner_original;
 sl_linux_birth *birth_original;
 sl_linux_candidate_preexec_shared *shared_original;
 struct sl_linux_candidate_preexec_inspection *prior_original;
 long context_result,result;unsigned not_ready,attempted,returned;
} sl_linux_candidate_preexec_inspection;
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *observation);
long sl_linux_prepare_candidate_preexec(sl_linux_candidate_preexec *owner);
long sl_linux_apply_candidate_preexec(sl_linux_candidate_preexec_shared *shared);
#endif
