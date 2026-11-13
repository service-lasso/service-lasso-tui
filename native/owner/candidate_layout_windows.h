#ifndef SERVICE_LASSO_CANDIDATE_LAYOUT_WINDOWS_H
#define SERVICE_LASSO_CANDIDATE_LAYOUT_WINDOWS_H
#include <stddef.h>
#include <stdint.h>

/* Pure source geometry. No declaration below issues native custody, admission,
 * source membership, reservation, a clock, or permission to enter startup. */
enum sl_win_candidate_category {
 SL_WIN_COST_CONTROL,SL_WIN_COST_QUERY,SL_WIN_COST_ARGUMENT,SL_WIN_COST_IO,
 SL_WIN_COST_IMAGE_PROVIDER,SL_WIN_COST_STACK_TLS,SL_WIN_COST_NATIVE_KERNEL,
 SL_WIN_COST_DEADLINE_CUSTODY,SL_WIN_COST_FAILURE_OVERLAP,SL_WIN_COST_CATEGORY_COUNT
};
enum sl_win_candidate_role {
 SL_WIN_LAYOUT_STARTUP,SL_WIN_LAYOUT_PIPE_NAME,SL_WIN_LAYOUT_TOKEN_USER,
 SL_WIN_LAYOUT_ACL,SL_WIN_LAYOUT_CONNECTIONS,SL_WIN_LAYOUT_SETTLEMENTS,
 SL_WIN_LAYOUT_ARGUMENT_OUTPUT,SL_WIN_LAYOUT_MODULE_ARENA,SL_WIN_LAYOUT_MODULE_QUERIES,
 SL_WIN_LAYOUT_COMMAND,SL_WIN_LAYOUT_SERVICES,SL_WIN_LAYOUT_ATTRIBUTES,
 SL_WIN_LAYOUT_SECURITY_0,SL_WIN_LAYOUT_SECURITY_1,SL_WIN_LAYOUT_SECURITY_2,
 SL_WIN_LAYOUT_SECURITY_3,SL_WIN_LAYOUT_SECURITY_4,SL_WIN_LAYOUT_SECURITY_5,
 SL_WIN_LAYOUT_TOKEN_0,SL_WIN_LAYOUT_TOKEN_1,SL_WIN_LAYOUT_ENDPOINT_SECURITY,
 SL_WIN_LAYOUT_TURNS, /* 21 external startup regions + separate startup =22. */
 SL_WIN_LAYOUT_FACTORY_CONTROL,SL_WIN_LAYOUT_DEBUG_EVENTS,SL_WIN_LAYOUT_MAPPED_ROOTS,
 SL_WIN_LAYOUT_ROOT_WRITES,SL_WIN_LAYOUT_DEBUG_RELEASES,SL_WIN_LAYOUT_ARGUMENT_ROOTS,
 SL_WIN_LAYOUT_READER_CANCELS,SL_WIN_LAYOUT_PROFILE_CONTROL,SL_WIN_LAYOUT_PROFILE_BYTES,
 SL_WIN_LAYOUT_PROFILE_ROWS,SL_WIN_LAYOUT_QUERY_REGIONS,SL_WIN_LAYOUT_GO_ARGUMENTS,
 SL_WIN_LAYOUT_GO_CREATOR_IMAGES,SL_WIN_LAYOUT_IMAGES_PROVIDERS,
 SL_WIN_LAYOUT_STACKS_TLS,SL_WIN_LAYOUT_NATIVE_IO,SL_WIN_LAYOUT_SOURCE_HELPERS,
 SL_WIN_LAYOUT_DEADLINE_CUSTODY,SL_WIN_LAYOUT_FAILURE_OVERLAP,SL_WIN_LAYOUT_ROLE_COUNT
};
_Static_assert(SL_WIN_LAYOUT_TURNS+1==22,"21 external startup regions plus separately charged control");
typedef struct sl_win_candidate_source_span {
 const void *original;size_t extent;
 unsigned category,actor_index,occurrence_index;
} sl_win_candidate_source_span;
typedef struct sl_win_candidate_layout_item {
 const sl_win_candidate_source_span *source_original;
 sl_win_candidate_source_span source_snapshot;
 size_t count,element_extent,alignment,offset,extent;unsigned role;
} sl_win_candidate_layout_item;
typedef struct sl_win_candidate_cost_item {
 const sl_win_candidate_source_span *source_original;
 sl_win_candidate_source_span source_snapshot;
 uint64_t resident_bytes,reserved_bytes,kernel_bytes,retained_failure_bytes;
 const void *overlap_original;size_t overlap_extent;unsigned role;
} sl_win_candidate_cost_item;
typedef struct sl_win_candidate_layout_source {
 const sl_win_candidate_layout_item *items_original;size_t item_count;
 const sl_win_candidate_cost_item *costs_original;size_t cost_count;
 sl_win_candidate_source_span actors_original,profile_original,abi_original;
} sl_win_candidate_layout_source;
typedef struct sl_win_candidate_layout {
 const sl_win_candidate_layout_source *source_original;
 const sl_win_candidate_layout_source *actual_source;
 sl_win_candidate_layout_source source_snapshot;
 sl_win_candidate_layout_item *copies_original;size_t copy_capacity;
 sl_win_candidate_cost_item *cost_copies_original;size_t cost_capacity;
 sl_win_candidate_layout_item *actual_copies;
 sl_win_candidate_cost_item *actual_cost_copies;
 size_t bytes,alignment,failed_index;
 uint64_t resident_bytes,reserved_bytes,kernel_bytes,retained_failure_bytes;
 uint64_t total_cost_bytes;
 uint64_t category_bytes[SL_WIN_COST_CATEGORY_COUNT];
 unsigned attempted,returned;int result;
} sl_win_candidate_layout;
/* Returns1 for checked representation only. All source/copy/control originals
 * stay live and disjoint; missing full physical operands refuse. */
int sl_win_plan_candidate_layout(sl_win_candidate_layout *);
int sl_win_candidate_layout_matches_original(const sl_win_candidate_layout *);
int sl_win_candidate_layout_excludes_output(const sl_win_candidate_layout *,const void *,size_t);
#endif
