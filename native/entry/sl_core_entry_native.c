#include "sl_core_entry_native.h"

#include <string.h>

long sl_core_entry_native(const struct sl_core_initial_entry *entry,
                          struct sl_core_entry_operands *out) {
    uint32_t index;
    if (!entry || !out || !sl_core_initial_entry(entry)) return SL_CORE_ENTRY_REFUSED;
    memset(out, 0, sizeof(*out));
    for (index = 0; index < entry->view_count; index++) {
        const struct sl_core_initial_entry_view *view = &entry->views[index];
        struct sl_core_entry_span span = {
            entry->image_base + view->offset,
            view->length,
        };
        switch (view->kind) {
        case SL_CORE_VIEW_CHARGE: out->charge = span; break;
        case SL_CORE_VIEW_SOURCE: out->source = span; break;
        case SL_CORE_VIEW_DOMAIN: out->domain = span; break;
        case SL_CORE_VIEW_ACTORS: out->actors = span; break;
        case SL_CORE_VIEW_BASES: out->bases = span; break;
        case SL_CORE_VIEW_PUBLIC_INPUT: out->public_input = span; break;
        case SL_CORE_VIEW_DEADLINE: out->deadline = span; break;
        case SL_CORE_VIEW_ISSUER: out->issuer = span; break;
        case SL_CORE_VIEW_CUSTODY: out->custody = span; break;
        default: break;
        }
    }
    if (!out->charge.bytes || !out->source.bytes || !out->domain.bytes ||
        !out->actors.bytes || !out->bases.bytes || !out->public_input.bytes ||
        !out->deadline.bytes || !out->issuer.bytes || !out->custody.bytes) {
        memset(out, 0, sizeof(*out));
        return SL_CORE_ENTRY_REFUSED;
    }
    return SL_CORE_ENTRY_ACQUIRED;
}
