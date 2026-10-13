#ifndef SERVICE_LASSO_CHANNELS_WINDOWS_H
#define SERVICE_LASSO_CHANNELS_WINDOWS_H
#ifdef _WIN32
#include "owner_windows.h"
typedef struct sl_win_channels {
 HANDLE process,read[3],write[3],inherited_gate;
 DWORD suggested_bytes[3],actual_suggested_bytes[3];
 sl_win_call process_query,create[3],gate_duplicate;
 unsigned attempted,returned,created_count,failed_index;
 int result;
} sl_win_channels;
/* Acquires gate/input/output pipes; failed CreatePipe outputs are retained
 * indeterminate observations, never adopted as valid handles or closed here. */
int sl_win_construct_channels(sl_win_channels *owner);
#endif
#endif
