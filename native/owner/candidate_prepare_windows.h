#ifndef SERVICE_LASSO_CANDIDATE_PREPARE_WINDOWS_H
#define SERVICE_LASSO_CANDIDATE_PREPARE_WINDOWS_H
#ifdef _WIN32
#include "candidate_layout_windows.h"
#include "argument_sender_factory_windows.h"
typedef struct sl_win_candidate_preparation {
 const sl_win_candidate_layout *plan_original;
 void *backing_original;SIZE_T backing_extent,writer_stack_original;
 sl_win_public_startup *startup_original;
 sl_win_argument_sender_factory *factory_original;
 unsigned attempted,returned;int result;
} sl_win_candidate_preparation;
/* Representation/prebinding only. Does not call startup/factory or any native
 * operation; real CLI child-local source/operation/finance binding is absent. */
int sl_win_prepare_candidate_views(sl_win_candidate_preparation *);
#endif
#endif
