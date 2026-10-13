#ifndef SERVICE_LASSO_CONTAINING_CHARGE_LINUX_H
#define SERVICE_LASSO_CONTAINING_CHARGE_LINUX_H
#include "candidate_arena_linux.h"
enum sl_linux_charge_category {
 SL_CHARGE_IMAGES_PROVIDERS,SL_CHARGE_STATIC_ROOTS,SL_CHARGE_STACKS_TLS,
 SL_CHARGE_RUNTIME_ALLOCATIONS,SL_CHARGE_KERNEL_OBJECTS,SL_CHARGE_PARSER_HISTORY,
 SL_CHARGE_IO_ARCHIVE,SL_CHARGE_RETAINED_OVERLAP,SL_CHARGE_CATEGORY_COUNT
};
typedef struct sl_linux_charge_item {
 const void *actor_original;unsigned long actor_extent;
 const void *basis_original;unsigned long basis_extent;
 unsigned long bytes;unsigned category;
} sl_linux_charge_item;
typedef struct sl_linux_containing_source {
 const void *source_graph_original;unsigned long source_graph_extent;
 const void *domain_original;unsigned long domain_extent;
 unsigned long available_bytes,page_size;
 const sl_linux_charge_item *items_original;unsigned long count;
} sl_linux_containing_source;
typedef struct sl_linux_containing_charge {
 const sl_linux_containing_source *source_original;
 const sl_linux_candidate_layout_plan *plan_original;
 sl_linux_charge_item *copies_original;unsigned long capacity;
 const sl_linux_containing_source *actual_source;
 const sl_linux_candidate_layout_plan *actual_plan;
 sl_linux_charge_item *actual_copies;unsigned long actual_capacity,count;
 sl_linux_containing_source source_snapshot;
 sl_linux_candidate_layout_plan checked_plan;
 unsigned long categories[SL_CHARGE_CATEGORY_COUNT],arena_bytes,total,remaining;
 long plan_result,result;unsigned attempted,plan_returned,returned;
} sl_linux_containing_charge;
_Static_assert(sizeof(unsigned long)==8&&sizeof(void*)==8,"selected charge ABI");
_Static_assert(sizeof(sl_linux_charge_item)==48&&sizeof(sl_linux_containing_source)==64,"retained charge input geometry");
_Static_assert(offsetof(sl_linux_containing_charge,actual_source)==32&&offsetof(sl_linux_containing_charge,checked_plan)==136,"retained charge original offsets");
_Static_assert(sizeof(sl_linux_containing_charge)==sizeof(sl_linux_candidate_layout_plan)+256,"charge includes independently retained layout reconstruction");
typedef struct sl_linux_charge_attachment {const sl_linux_containing_charge *charge_original;unsigned attempted,returned;} sl_linux_charge_attachment;
_Static_assert(sizeof(sl_linux_charge_attachment)==16,"once containing occurrence association");
int sl_linux_attach_containing_charge(sl_linux_charge_attachment *original,const sl_linux_containing_charge *charge);
int sl_linux_charge_attachment_matches_original(const sl_linux_charge_attachment *original,const sl_linux_containing_charge *charge);
long sl_linux_construct_containing_charge(sl_linux_containing_charge *original);
int sl_linux_containing_charge_matches_original(const sl_linux_containing_charge *original);
int sl_linux_containing_charge_excludes_output(const sl_linux_containing_charge *original,const void *output,unsigned long extent);
/* This is a retained charge derivation, not a source/domain authenticator.
 * Genuine prior domain and complete source-derived actor/lifetime roster must
 * be joined by actual entry before allocation. No free-memory query, Q32,
 * copied field, arithmetic success or category label creates credit.
 * Items charge additional simultaneous/retained objects; the SAME arena is
 * rounded and charged here separately. All provider rounding of other objects
 * and overlaps come from their actual source basis, never invented actors. */
#endif
