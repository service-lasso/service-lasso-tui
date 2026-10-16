#ifndef SERVICE_LASSO_CANDIDATE_DEADLINE_WINDOWS_H
#define SERVICE_LASSO_CANDIDATE_DEADLINE_WINDOWS_H
#ifdef _WIN32
/* Selected build must supply the actual held CLI header and its complete nested
 * source closure. No copied declarations or ambient checkout include fallback. */
#ifndef SERVICE_LASSO_PRODUCT_STORAGE_HEADER
#error exact source-qualified CLI product storage header required
#endif
#include SERVICE_LASSO_PRODUCT_STORAGE_HEADER
#include "candidate_prepare_windows.h"
enum sl_win_deadline_row_kind {
 SL_WIN_DEADLINE_STARTUP,SL_WIN_DEADLINE_DEBUG_EVENT,SL_WIN_DEADLINE_MAPPED_ROOT,
 SL_WIN_DEADLINE_ROOT_WRITE,SL_WIN_DEADLINE_DEBUG_RELEASE,SL_WIN_DEADLINE_ARGUMENT_ROOT,
 SL_WIN_DEADLINE_READER_CANCEL,SL_WIN_DEADLINE_DUPLEX_COMPLETION,
 SL_WIN_DEADLINE_DUPLEX_SETTLEMENT,SL_WIN_DEADLINE_MODULE_QUERY,
 SL_WIN_DEADLINE_SERVICE,SL_WIN_DEADLINE_SEND_TURN,SL_WIN_DEADLINE_ROW_KIND_COUNT
};
typedef struct sl_win_candidate_deadline_row {
 sl_win_call *native_row_original;
 const void *owner_original;SIZE_T owner_extent;
 unsigned kind;SIZE_T occurrence,ordinal;
} sl_win_candidate_deadline_row;
struct sl_win_candidate_deadline_sequence;
typedef struct sl_win_candidate_deadline_step {
 struct sl_win_candidate_deadline_sequence *sequence_original;
 const sl_win_candidate_deadline_row *source_original;
 sl_win_candidate_deadline_row source_snapshot;
 const struct slcli_source_deadline_original *deadline_original;
 const struct slcli_product_windows_operation_caller *caller_original;
 struct slcli_native_clock before,after;
} sl_win_candidate_deadline_step;
typedef struct sl_win_candidate_deadline_source {
 const sl_win_candidate_preparation *preparation_original;
 const struct slcli_product_windows_operation_caller *caller_original;
 const sl_win_candidate_deadline_row *rows_original;SIZE_T row_count;
} sl_win_candidate_deadline_source;
typedef struct sl_win_candidate_deadline_sequence {
 const sl_win_candidate_deadline_source *source_original,*actual_source;
 sl_win_candidate_deadline_source source_snapshot;
 struct slcli_arena *arena_original;
 struct slcli_memory_owner *memory_original;
 const struct slcli_product_storage_windows_source *initial_source_original;
 const struct slcli_source_deadline_original *deadline_original;
 const struct slcli_native_clock *origin_original;
 uint64_t origin_tick,expires,origin_raw[2],origin_status[2],origin_args[2];
 sl_win_candidate_deadline_step *steps_original;SIZE_T capacity;
 unsigned attempted,returned;int result;
} sl_win_candidate_deadline_sequence;
/* Pure retained relation only: no clock read, allocation, dispatch or issuer.
 * Real CLI child-local entry/family and actual later native cuts remain absent. */
int sl_win_bind_prepared_candidate_deadline(sl_win_candidate_deadline_sequence *);
int sl_win_candidate_deadline_matches_original(const sl_win_candidate_deadline_sequence *);
#endif
#endif
