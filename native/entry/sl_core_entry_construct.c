#include "sl_core_entry_construct.h"

#include <errno.h>

long sl_core_entry_construct(struct sl_core_entry_construction_output *out,
                             const struct sl_core_entry_construction_input *input) {
    long result;
    if (!out || !input || !input->charge || !input->source || !input->plan ||
        !input->copies || !input->capacity || !input->deadline_source ||
        !input->deadline || !input->sequence) {
        return SL_CORE_ENTRY_CONSTRUCTION_REFUSED;
    }
    out->charge = input->charge;
    out->deadline = input->deadline;
    out->sequence = input->sequence;
    input->charge->source_original = input->source;
    input->charge->plan_original = input->plan;
    input->charge->copies_original = input->copies;
    input->charge->capacity = input->capacity;
    result = sl_linux_construct_containing_charge(input->charge);
    if (result) return SL_CORE_ENTRY_CONSTRUCTION_REFUSED;
    result = sl_linux_construct_deadline(input->deadline, input->deadline_source);
    if (result) return SL_CORE_ENTRY_CONSTRUCTION_REFUSED;
    input->sequence->root_original = input->deadline;
    result = sl_linux_prepare_deadline_sequence(input->sequence);
    if (result) return SL_CORE_ENTRY_CONSTRUCTION_REFUSED;
    return SL_CORE_ENTRY_CONSTRUCTED;
}
