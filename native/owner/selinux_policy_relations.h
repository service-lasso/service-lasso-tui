#ifndef SERVICE_LASSO_SELINUX_POLICY_RELATIONS_H
#define SERVICE_LASSO_SELINUX_POLICY_RELATIONS_H
#include <sepol/policydb/policydb.h>
/* These occurrences interpret SAME parsed objects. They do not authenticate a
 * parser, loaded policy, kernel mapping, enforcement or native profile. */
typedef struct sl_selinux_lookup_source {const char *bytes;unsigned long extent;} sl_selinux_lookup_source;
extern const char sl_selinux_name_file[];
extern const char sl_selinux_name_dir[];
extern const char sl_selinux_name_process[];
extern const char sl_selinux_name_filesystem[];
extern const char sl_selinux_name_unix_stream_socket[];
extern const char sl_selinux_name_write[];
extern const char sl_selinux_name_append[];
extern const char sl_selinux_name_relabelfrom[];
extern const char sl_selinux_name_relabelto[];
extern const char sl_selinux_name_associate[];
extern const char sl_selinux_name_ptrace[];
extern const char sl_selinux_name_transition[];
extern const char sl_selinux_name_dyntransition[];
extern const char sl_selinux_name_read[];
extern const char sl_selinux_name_create[];
extern const char sl_selinux_name_entrypoint[];
extern const char sl_selinux_name_execute_no_trans[];
extern const char sl_selinux_name_setattr[];
extern const char sl_selinux_name_ioctl[];
extern const char sl_selinux_name_unlink[];
extern const char sl_selinux_name_link[];
extern const char sl_selinux_name_add_name[];
extern const char sl_selinux_name_remove_name[];
extern const char sl_selinux_name_rename[];
extern const char sl_selinux_name_reparent[];
extern const sl_selinux_lookup_source sl_selinux_lookup_sources[25];
const sl_selinux_lookup_source *sl_selinux_lookup_source_for(const char *name);
typedef struct sl_selinux_bounds {
 policydb_t *policy_original,*actual_policy;
 unsigned old_type,new_type,actual_old,actual_new;
 type_datum_t **visited,**actual_visited;
 unsigned long capacity,actual_capacity,count;
 int result;unsigned attempted,returned;
} sl_selinux_bounds;
typedef struct sl_selinux_rule_visit {
 avtab_t *table_original;avtab_ptr_t node_original;
 unsigned long bucket;unsigned conditional,matched;
} sl_selinux_rule_visit;
enum sl_selinux_helper_kind {SL_SELINUX_HASH_BUCKET=1,SL_SELINUX_HASH_NODE,SL_SELINUX_BITMAP_NODE,SL_SELINUX_AVTAB_BUCKET,SL_SELINUX_FILENAME_BUCKET,SL_SELINUX_FILENAME_NODE,SL_SELINUX_FILENAME_DATUM,SL_SELINUX_GENFS_NODE,SL_SELINUX_OCONTEXT_NODE};
typedef struct sl_selinux_helper_visit {
 enum sl_selinux_helper_kind kind;
 avtab_t *avtab_original;
 hashtab_t table_original;hashtab_ptr_t hash_original;
 const char *name_original,*key_original;const char **name_slot_original;const sl_selinux_lookup_source *name_source_original;unsigned long bucket,compared;
 unsigned long name_read_extent,key_read_extent;unsigned name_terminated,key_terminated;
 ebitmap_t *bitmap_original;ebitmap_node_t *bitmap_node_original;
 filename_trans_key_t *filename_key_original;filename_trans_datum_t *filename_datum_original;
 genfs_t *genfs_original;ocontext_t *ocontext_original;unsigned context_list;
 unsigned bit,startbit,highbit;MAPTYPE map;int matched;
} sl_selinux_helper_visit;
typedef struct sl_selinux_te_union {
 policydb_t *policy_original,*actual_policy;
 unsigned source_type,target_type,actual_source,actual_target;
 const char *class_name,*permission_name,*actual_class_name,*actual_permission_name;
 sepol_security_class_t actual_class;
 class_datum_t *class_original;perm_datum_t *permission_original;common_datum_t *common_original;
 sepol_access_vector_t actual_permission,unconditional,conditional,possible;
 ebitmap_t *source_attributes,*target_attributes;
 sl_selinux_rule_visit *visits,*actual_visits;
 unsigned long capacity,actual_capacity,count;
 sl_selinux_helper_visit *helpers,*actual_helpers;
 unsigned long helper_capacity,actual_helper_capacity,helper_count,string_capacity,actual_string_capacity;
 int result;unsigned attempted,returned;
} sl_selinux_te_union;
int sl_selinux_interpret_bounds(sl_selinux_bounds *o);
int sl_selinux_interpret_te_union(sl_selinux_te_union *o);

typedef struct sl_selinux_creation_types {
 sl_selinux_te_union custody;
 unsigned *types,*actual_types;type_datum_t **referents,**actual_referents;
 unsigned long capacity,actual_capacity,count;
 type_datum_t *owner_original,*directory_original;
 type_datum_t *explicit_original;unsigned explicit_type;
 unsigned default_type;unsigned attempted,returned;int result;
} sl_selinux_creation_types;
/* Conservative original-policy creation TYPE union only, not newSID/FS authority. */
int sl_selinux_interpret_creation_types(sl_selinux_creation_types *root);
typedef struct sl_selinux_genfs_types {
 sl_selinux_creation_types targets,filesystem;
 context_struct_t **observed,**actual_observed;
 unsigned long observed_count,actual_observed_count,checked_observed;
 ocontext_t *unlabeled_original,*filesystem_original;const char *class_names[2];
 unsigned file_class,directory_class,matched_filesystems,matched_rules;
 unsigned attempted,returned;int result;
} sl_selinux_genfs_types;
/* Parsed possible-label observation only. Initial/future native labeling,
 * SAME parser ownership, purpose/actor enforcement remain separate consumers. */
int sl_selinux_interpret_cgroup_genfs_types(sl_selinux_genfs_types *root);
typedef struct sl_selinux_source_access {
 type_datum_t *type_original;unsigned type,flavor,permissive;ebitmap_t *permissive_original;
 sepol_access_vector_t unconditional,conditional,possible;
} sl_selinux_source_access;
typedef struct sl_selinux_access_sources {
 sl_selinux_te_union custody;
 sl_selinux_creation_types *creation_original,*actual_creation;
 sl_selinux_source_access *sources,*actual_sources;unsigned long source_capacity,actual_source_capacity,source_count;
 const char *permission_names[2];perm_datum_t *permission_originals[2];sepol_access_vector_t permission_bits[2];unsigned permission_count;
 perm_datum_t *write_original,*append_original;
 sepol_access_vector_t write_permission,append_permission,requested_permissions;
 unsigned attempted,returned;int result;
} sl_selinux_access_sources;
/* All parsed primary source types; observation only, no trusted actor labels. */
int sl_selinux_interpret_access_sources(sl_selinux_access_sources *root);
int sl_selinux_interpret_permission_sources(sl_selinux_access_sources *root,const char *class_name,const char *permission_name);
int sl_selinux_interpret_permission_pair_sources(sl_selinux_access_sources *root,const char *class_name,const char *first,const char *second);
int sl_selinux_interpret_creation_permission_sources(sl_selinux_access_sources *root,sl_selinux_creation_types *creation,const char *first,const char *second);
int sl_selinux_interpret_target_permission_sources(sl_selinux_access_sources *root,sl_selinux_creation_types *targets,const char *class_name,const char *first,const char *second);
typedef struct sl_selinux_transition_edge {
 type_datum_t *source_original,*target_original;
 avtab_ptr_t transition_original,dyntransition_original;
 unsigned transition,dyntransition,permissive;ebitmap_t *permissive_original;
} sl_selinux_transition_edge;
typedef struct sl_selinux_transition_matrix {
 sl_selinux_te_union custody;
 sl_selinux_transition_edge *edges,*actual_edges;unsigned long edge_capacity,actual_edge_capacity;
 unsigned *matches,*actual_matches;unsigned long match_capacity,actual_match_capacity;
 const char *permission_names[2]; /* Original first/second permission operands. */
 perm_datum_t *transition_original,*dyntransition_original;
 sepol_access_vector_t transition_permission,dyntransition_permission;
 unsigned long types;unsigned attempted,returned;int result;
} sl_selinux_transition_matrix;
int sl_selinux_interpret_transition_matrix(sl_selinux_transition_matrix *root);
/* Generic matrix edges retain first/second bits in transition/dyntransition fields. */
int sl_selinux_interpret_permission_matrix(sl_selinux_transition_matrix *root,const char *class_name,const char *first,const char *second);
typedef struct sl_selinux_exec_type {
 sl_selinux_te_union custody;
 type_datum_t *source_original,*file_original;
 avtab_ptr_t first_original,last_original;unsigned long matched,permanent_matched,conditional_matched;unsigned changed_type,default_type;
 unsigned attempted,returned;int result;
} sl_selinux_exec_type;
int sl_selinux_interpret_exec_type_stability(sl_selinux_exec_type *root);
#endif
