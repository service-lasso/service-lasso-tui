#ifndef SERVICE_LASSO_GENFS_CORRESPONDENCE_LINUX_H
#define SERVICE_LASSO_GENFS_CORRESPONDENCE_LINUX_H
#include "candidate_policy_linux.h"
typedef struct {
 const sl_candidate_record *declaration_original,*evidence_original;
 sl_candidate_span statement_original,interpretation_original,identities[4];
 uint64_t evidence_index,statement_offset,statement_extent;
 unsigned identity_checked[4],attempted,returned;long result;
} sl_linux_genfs_declaration;
typedef struct {
 sl_linux_candidate_policy *policy_original;
 sl_linux_genfs_declaration *declarations,*actual_declarations;
 size_t capacity,actual_capacity,count,examined;
 sl_candidate_profile *candidate_original;
 sl_linux_selinux_status_sample after_correspondence;
 long primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_genfs_correspondence;
/* Representation correspondence only: no initial-facts or birth grant. */
long sl_linux_interpret_genfs_correspondence(sl_linux_genfs_correspondence *);
#endif
