#include "sl_core_entry_native.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct sl_core_initial_entry_view views[9];
static uint8_t image[512];

int main(void) {
    struct sl_core_initial_entry entry = {0};
    struct sl_core_entry_operands operands;
    for (uint32_t index = 0; index < 9; index++) {
        views[index].kind = SL_CORE_VIEW_CHARGE + index;
        views[index].offset = 128 + index * 40;
        views[index].length = 16;
        views[index].alignment = 8;
        views[index].item_count = 1;
    }
    entry.abi_version = SL_CORE_INITIAL_ENTRY_ABI_VERSION;
    entry.view_count = 9;
    entry.image_base = image;
    entry.image_bytes = sizeof(image);
    entry.views = views;
    if (sl_core_entry_native(&entry, &operands) != SL_CORE_ENTRY_ACQUIRED) {
        fprintf(stderr, "valid views refused\n");
        return 1;
    }
    if (operands.charge.length != 16 || operands.custody.bytes != image + 128 + 8 * 40) {
        fprintf(stderr, "view mapping incorrect\n");
        return 2;
    }
    if (sl_core_entry_native(NULL, &operands) != SL_CORE_ENTRY_REFUSED) {
        fprintf(stderr, "null entry admitted\n");
        return 3;
    }
    views[0].length = 0;
    if (sl_core_entry_native(&entry, &operands) != SL_CORE_ENTRY_REFUSED) {
        fprintf(stderr, "invalid view admitted\n");
        return 4;
    }
    return 0;
}
