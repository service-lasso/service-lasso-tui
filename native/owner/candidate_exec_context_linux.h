#ifndef SERVICE_LASSO_CANDIDATE_EXEC_CONTEXT_LINUX_H
#define SERVICE_LASSO_CANDIDATE_EXEC_CONTEXT_LINUX_H
#include "selinux_context_linux.h"
#include "selinux_policy_relations.h"
enum sl_exec_visit_kind {SL_EXEC_ROLE=1,SL_EXEC_RANGE_BUCKET,SL_EXEC_RANGE_NODE,SL_EXEC_BITMAP};
typedef struct sl_exec_context_visit {
 enum sl_exec_visit_kind kind;role_trans_t *role_original;
 hashtab_t table_original;hashtab_ptr_t node_original;range_trans_t *key_original;mls_range_t *range_original;
 const ebitmap_t *bitmap_original;ebitmap_node_t *bitmap_node_original;
 unsigned long bucket;unsigned startbit;MAPTYPE map;
} sl_exec_context_visit;
typedef struct sl_exec_level_comparison {
 const mls_level_t *expected,*left,*right;unsigned intersection,sensitivity;
 unsigned long begins[3],counts[3],examined;int result;
} sl_exec_level_comparison;
typedef struct sl_linux_candidate_exec_context {
 sl_linux_selinux_context *source_original,*actual_source,*file_original,*actual_file;
 sl_selinux_exec_type *type_original;
 class_datum_t *class_original;unsigned default_user,default_role,default_range,new_user,new_role;
 role_trans_t *role_original;mls_range_t *range_original;
 sl_exec_context_visit *visits,*actual_visits;unsigned long capacity,actual_capacity,count,role_matches,range_matches;
 sl_exec_level_comparison levels[2];int result;unsigned attempted,returned;
} sl_linux_candidate_exec_context;
int sl_linux_interpret_exec_context_stability(sl_linux_candidate_exec_context *owner);
#endif