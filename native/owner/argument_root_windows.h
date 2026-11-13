#ifndef SERVICE_LASSO_ARGUMENT_ROOT_WINDOWS_H
#define SERVICE_LASSO_ARGUMENT_ROOT_WINDOWS_H
#ifdef _WIN32
#include "public_startup_windows.h"
#include "windows_root_layout.h"
typedef struct sl_win_argument_root_source {
 const sl_win_public_startup *startup_original,*actual_startup;
 sl_windows_native_root_storage root;
 unsigned attempted,returned;
 int result;
} sl_win_argument_root_source;
/* Source tuple construction only. Complete native authentication/D/finance
 * must dominate remote write/release; this grants no effect or read access. */
int sl_win_prepare_argument_root_source(sl_win_argument_root_source *original);
#endif
#endif
