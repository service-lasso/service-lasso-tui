#ifndef SERVICE_LASSO_CANDIDATE_PROFILE_H
#define SERVICE_LASSO_CANDIDATE_PROFILE_H
#include <stdint.h>
#include <stddef.h>
/* Source-owned construction binds all backing before entry. No runtime size
 * input or decoded authorization value selects an allocation. Sole startup
 * thread owns decoding; all consumers wait for its genuine return. */
typedef struct {const unsigned char *bytes;size_t extent;} sl_candidate_span;
typedef struct sl_candidate_record {
 sl_candidate_span original,fields[8],statement_original;
 unsigned field_count;
 uint64_t evidence_index,statement_offset,statement_extent,policy_user,policy_role,policy_type;
 const struct sl_candidate_record *evidence_original;
} sl_candidate_record;
typedef struct {
 const unsigned char *source;size_t source_extent;
 sl_candidate_record *rows[5];size_t capacities[5];
 sl_candidate_span original,sections[9],source_identity,layout_identity,policy_identity,kernel_interpretation;
 uint64_t policy_extent,security_label_bytes,terminal_calls,representation_bytes;
 sl_candidate_record *actual_rows[5];size_t actual_capacities[5],counts[5];
 size_t cursor,section_index,record_index,field_index;
 unsigned attempted,returned;long result;
} sl_candidate_profile;
/* Structural interpretation only. Author/role/profile/loaded-code predicates
 * require their actual independent consumers; return0 grants none of them. */
long sl_candidate_decode_profile(sl_candidate_profile *profile);
#endif