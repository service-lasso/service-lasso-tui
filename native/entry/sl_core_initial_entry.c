#include "sl_core_initial_entry.h"

#include <string.h>

static int required_kind(uint32_t kind) {
    return kind >= SL_CORE_VIEW_CHARGE && kind <= SL_CORE_VIEW_CUSTODY;
}

static int valid_entry(const struct sl_core_initial_entry *entry) {
    uint32_t index;
    uint32_t other;
    uint32_t required_seen = 0;
    if (!entry || entry->abi_version != SL_CORE_INITIAL_ENTRY_ABI_VERSION ||
        !entry->image_base || !entry->views || entry->view_count == 0 ||
        entry->view_count > SL_CORE_INITIAL_ENTRY_MAX_VIEWS || entry->image_bytes == 0) {
        return 0;
    }
    for (index = 0; index < entry->view_count; index++) {
        const struct sl_core_initial_entry_view *view = &entry->views[index];
        if (view->kind < SL_CORE_VIEW_CHARGE || view->kind > SL_CORE_VIEW_OBSERVATION ||
            view->flags != 0 || view->length == 0 || view->item_count == 0 ||
            view->alignment != 8 || (view->offset % 8) != 0 ||
            view->offset > entry->image_bytes ||
            view->length > entry->image_bytes - view->offset ||
            view->item_count > view->length) {
            return 0;
        }
        if (required_kind(view->kind)) {
            uint32_t bit = 1u << (view->kind - 1u);
            if (required_seen & bit) return 0;
            required_seen |= bit;
        }
        for (other = 0; other < index; other++) {
            const struct sl_core_initial_entry_view *previous = &entry->views[other];
            if (previous->kind == view->kind) return 0;
            if (previous->offset < view->offset + view->length &&
                view->offset < previous->offset + previous->length) {
                return 0;
            }
        }
    }
    for (index = SL_CORE_VIEW_CHARGE; index <= SL_CORE_VIEW_CUSTODY; index++) {
        if ((required_seen & (1u << (index - 1u))) == 0) return 0;
    }
    return 1;
}

long sl_core_initial_entry_acquire(struct sl_core_initial_entry *entry,
                                   const struct sl_core_initial_entry_source *source) {
    if (!entry || !source) return SL_CORE_ENTRY_REFUSED;
    memset(entry, 0, sizeof(*entry));
    if (!source->originals) return SL_CORE_ENTRY_RETAINED_UNAVAILABLE;
    entry->abi_version = SL_CORE_INITIAL_ENTRY_ABI_VERSION;
    entry->view_count = source->view_count;
    entry->image_base = source->image_base;
    entry->image_bytes = source->image_bytes;
    entry->views = source->views;
    entry->binder = source->binder;
    if (!valid_entry(entry)) {
        memset(entry, 0, sizeof(*entry));
        return SL_CORE_ENTRY_REFUSED;
    }
    return SL_CORE_ENTRY_ACQUIRED;
}

const struct sl_core_initial_entry *sl_core_initial_entry(
    const struct sl_core_initial_entry *entry) {
    return valid_entry(entry) ? entry : NULL;
}

int sl_core_initial_entry_matches_original(
    const struct sl_core_initial_entry *entry,
    const struct sl_core_initial_entry *original) {
    uint32_t index;
    if (!valid_entry(entry) || !valid_entry(original)) return 0;
    if (entry->abi_version != original->abi_version ||
        entry->view_count != original->view_count ||
        entry->image_base != original->image_base ||
        entry->image_bytes != original->image_bytes ||
        entry->binder != original->binder) {
        return 0;
    }
    for (index = 0; index < entry->view_count; index++) {
        const struct sl_core_initial_entry_view *left = &entry->views[index];
        const struct sl_core_initial_entry_view *right = &original->views[index];
        if (left->kind != right->kind || left->flags != right->flags ||
            left->offset != right->offset || left->length != right->length ||
            left->alignment != right->alignment || left->item_count != right->item_count) {
            return 0;
        }
    }
    return 1;
}

long sl_core_initial_entry_handoff(struct sl_core_initial_entry *destination,
                                   struct sl_core_initial_entry *source) {
    if (!destination || !source || !valid_entry(source)) return SL_CORE_ENTRY_REFUSED;
    *destination = *source;
    memset(source, 0, sizeof(*source));
    return SL_CORE_ENTRY_ACQUIRED;
}


long sl_core_initial_entry_owned(const struct sl_core_initial_entry *entry,
                                 sl_core_owned_entry_fn owned) {
    if (!owned || !valid_entry(entry)) return SL_CORE_ENTRY_REFUSED;
    return owned(entry);
}
