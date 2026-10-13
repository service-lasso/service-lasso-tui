#ifndef SERVICE_LASSO_CANDIDATE_POLICY_LINUX_H
#define SERVICE_LASSO_CANDIDATE_POLICY_LINUX_H
#include "candidate_profile.h"
#include "policy_parse_linux.h"
#include "owned_sha256.h"
enum sl_candidate_policy_failure {SL_CANDIDATE_POLICY_NONE=0,SL_CANDIDATE_POLICY_ROOT,SL_CANDIDATE_POLICY_PARSE,SL_CANDIDATE_POLICY_EXTENT,SL_CANDIDATE_POLICY_DIGEST,SL_CANDIDATE_POLICY_MISMATCH,SL_CANDIDATE_POLICY_STATUS};
typedef struct {
 sl_candidate_profile *candidate_original,*actual_candidate;
 sl_linux_policy_parse *parse_original,*actual_parse;
 sl_owned_sha256 digest;
 sl_candidate_span declared_identity,kernel_interpretation_original;
 uint64_t declared_extent;size_t observed_extent,mismatch_offset;
 sl_linux_selinux_status_sample after_binding;
 long digest_result,status_result,result;enum sl_candidate_policy_failure failure;
 unsigned attempted,returned;
} sl_linux_candidate_policy;
long sl_linux_bind_candidate_policy(sl_linux_candidate_policy *binding);
enum sl_candidate_role_failure {SL_CANDIDATE_ROLE_NONE=0,SL_CANDIDATE_ROLE_ROOT,SL_CANDIDATE_ROLE_POLICY,SL_CANDIDATE_ROLE_CAPACITY,SL_CANDIDATE_ROLE_STATEMENT,SL_CANDIDATE_ROLE_DATUM,SL_CANDIDATE_ROLE_STATUS};
typedef struct {
 const sl_candidate_record *declaration_original,*evidence_original;
 sl_candidate_span statement_original,actor_meaning_original,episode_meaning_original;
 uint64_t user_value,role_value,type_value;
 user_datum_t *user_original;role_datum_t *role_original;type_datum_t *type_original;
 unsigned observed_user,observed_role,observed_type,observed_role_flavor,observed_type_flavor;
 unsigned attempted,returned;long result;
} sl_candidate_role_binding;
typedef struct {
 sl_candidate_profile *candidate_original,*actual_candidate;
 sl_linux_policy_parse *parse_original,*actual_parse;
 sl_candidate_role_binding *bindings,*actual_bindings;
 size_t capacity,actual_capacity,count;
 policydb_t *policy_original;
 sl_linux_selinux_status_sample after_roles;
 long status_result,result;enum sl_candidate_role_failure failure;
 const sl_candidate_record *failed_declaration;
 unsigned attempted,returned;
} sl_linux_candidate_roles;
/* Bind exact declared subject referents only. Author correspondence remains the
 * independently qualified original evidence meaning; trusted source-domain
 * classification requires full selected policy/entry/route consumers. */
long sl_linux_bind_candidate_roles(sl_linux_candidate_roles *roles);
#endif