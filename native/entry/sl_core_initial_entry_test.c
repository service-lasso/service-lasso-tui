#include "sl_core_initial_entry.h"

#include <stdint.h>
#include <stdio.h>

static struct sl_core_initial_entry_view views[9];
static uint8_t image[512];

static void prepare_views(void) {
    for (uint32_t index = 0; index < 9; index++) {
        views[index].kind = SL_CORE_VIEW_CHARGE + index;
        views[index].offset = 128 + index * 40;
        views[index].length = 16;
        views[index].alignment = 8;
        views[index].item_count = 1;
    }
}

int main(void) {
    struct sl_core_initial_entry_source source = {0};
    struct sl_core_initial_entry entry;
    struct sl_core_initial_entry moved;
    int marker = 0;
    prepare_views();
    source.image_base = image;
    source.image_bytes = sizeof(image);
    source.views = views;
    source.view_count = 9;

    if (sl_core_initial_entry_acquire(&entry, &source) != SL_CORE_ENTRY_RETAINED_UNAVAILABLE) {
        fprintf(stderr, "missing originals was not refused\n");
        return 1;
    }

    source.originals = &marker;
    if (sl_core_initial_entry_acquire(&entry, &source) != SL_CORE_ENTRY_ACQUIRED) {
        fprintf(stderr, "valid source was not acquired\n");
        return 2;
    }
    if (!sl_core_initial_entry(&entry) ||
        !sl_core_initial_entry_matches_original(&entry, &entry)) {
        fprintf(stderr, "entry did not match itself\n");
        return 3;
    }
    if (sl_core_initial_entry_handoff(&moved, &entry) != SL_CORE_ENTRY_ACQUIRED ||
        sl_core_initial_entry(&entry) != NULL || !sl_core_initial_entry(&moved)) {
        fprintf(stderr, "handoff did not move ownership\n");
        return 4;
    }

    views[0].length = 0;
    if (sl_core_initial_entry_acquire(&entry, &source) != SL_CORE_ENTRY_REFUSED) {
        fprintf(stderr, "invalid view was not refused\n");
        return 5;
    }
    return 0;
}
