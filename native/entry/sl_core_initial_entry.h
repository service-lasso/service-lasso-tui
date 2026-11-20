#ifndef SERVICE_LASSO_CORE_INITIAL_ENTRY_H
#define SERVICE_LASSO_CORE_INITIAL_ENTRY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SL_CORE_INITIAL_ENTRY_ABI_VERSION 1u
#define SL_CORE_INITIAL_ENTRY_MAX_VIEWS 11u

enum sl_core_initial_entry_view_kind {
    SL_CORE_VIEW_CHARGE = 1,
    SL_CORE_VIEW_SOURCE = 2,
    SL_CORE_VIEW_DOMAIN = 3,
    SL_CORE_VIEW_ACTORS = 4,
    SL_CORE_VIEW_BASES = 5,
    SL_CORE_VIEW_PUBLIC_INPUT = 6,
    SL_CORE_VIEW_DEADLINE = 7,
    SL_CORE_VIEW_ISSUER = 8,
    SL_CORE_VIEW_CUSTODY = 9,
    SL_CORE_VIEW_CAPTURE = 10,
    SL_CORE_VIEW_OBSERVATION = 11
};

struct sl_core_initial_entry_view {
    uint32_t kind;
    uint32_t flags;
    uint64_t offset;
    uint64_t length;
    uint64_t alignment;
    uint64_t item_count;
};

struct sl_core_initial_entry {
    uint32_t abi_version;
    uint32_t view_count;
    const uint8_t *image_base;
    uint64_t image_bytes;
    const struct sl_core_initial_entry_view *views;
    void *binder;
};

/* The source is supplied only by the Core-owned launch/binder path. A non-null
 * originals field means authentic Core originals were retained for this attempt;
 * a null field is a refusal, never a reason to synthesize data. */
struct sl_core_initial_entry_source {
    const uint8_t *image_base;
    uint64_t image_bytes;
    const struct sl_core_initial_entry_view *views;
    uint32_t view_count;
    void *binder;
    const void *originals;
};

enum sl_core_initial_entry_result {
    SL_CORE_ENTRY_ACQUIRED = 0,
    SL_CORE_ENTRY_RELEASED = 1,
    SL_CORE_ENTRY_RETAINED_UNAVAILABLE = 2,
    SL_CORE_ENTRY_REFUSED = 3
};

long sl_core_initial_entry_acquire(struct sl_core_initial_entry *entry,
                                   const struct sl_core_initial_entry_source *source);
const struct sl_core_initial_entry *sl_core_initial_entry(
    const struct sl_core_initial_entry *entry);
int sl_core_initial_entry_matches_original(
    const struct sl_core_initial_entry *entry,
    const struct sl_core_initial_entry *original);
long sl_core_initial_entry_handoff(struct sl_core_initial_entry *destination,
                                   struct sl_core_initial_entry *source);

#ifdef __cplusplus
}
#endif

#endif
