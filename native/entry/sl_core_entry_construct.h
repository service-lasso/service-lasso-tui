#ifndef SERVICE_LASSO_CORE_ENTRY_CONSTRUCT_H
#define SERVICE_LASSO_CORE_ENTRY_CONSTRUCT_H

#include "containing_charge_linux.h"
#include "deadline_linux.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sl_core_entry_construction_input {
    sl_linux_containing_charge *charge;
    const sl_linux_containing_source *source;
    const sl_linux_candidate_layout_plan *plan;
    sl_linux_charge_item *copies;
    unsigned long capacity;
    sl_linux_deadline_source *deadline_source;
    sl_linux_deadline *deadline;
    sl_linux_deadline_sequence *sequence;
};

struct sl_core_entry_construction_output {
    sl_linux_containing_charge *charge;
    sl_linux_deadline *deadline;
    sl_linux_deadline_sequence *sequence;
};

enum sl_core_entry_construction_result {
    SL_CORE_ENTRY_CONSTRUCTED = 0,
    SL_CORE_ENTRY_CONSTRUCTION_REFUSED = -1
};

long sl_core_entry_construct(struct sl_core_entry_construction_output *out,
                             const struct sl_core_entry_construction_input *input);

#ifdef __cplusplus
}
#endif

#endif
