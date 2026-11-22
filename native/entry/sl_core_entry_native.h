#ifndef SERVICE_LASSO_CORE_ENTRY_NATIVE_H
#define SERVICE_LASSO_CORE_ENTRY_NATIVE_H

#include "sl_core_initial_entry.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sl_core_entry_span {
    const uint8_t *bytes;
    uint64_t length;
};

struct sl_core_entry_operands {
    struct sl_core_entry_span charge;
    struct sl_core_entry_span source;
    struct sl_core_entry_span domain;
    struct sl_core_entry_span actors;
    struct sl_core_entry_span bases;
    struct sl_core_entry_span public_input;
    struct sl_core_entry_span deadline;
    struct sl_core_entry_span issuer;
    struct sl_core_entry_span custody;
};

long sl_core_entry_native(const struct sl_core_initial_entry *entry,
                          struct sl_core_entry_operands *out);

#ifdef __cplusplus
}
#endif

#endif
