#ifndef SERVICE_LASSO_ARGUMENT_READER_CANCEL_WINDOWS_H
#define SERVICE_LASSO_ARGUMENT_READER_CANCEL_WINDOWS_H
#ifdef _WIN32
#include "public_startup_windows.h"
#include "debug_root_windows.h"
typedef struct sl_win_argument_reader_cancel {
 sl_win_public_startup *startup_original,*actual_startup;
 const sl_win_debug_release *release_original,*actual_release;
 sl_win_call disconnect,thread_query,thread_wait,cancel;
 sl_win_argument_send_cancellation writer_cancellation;
 HANDLE actual_server,actual_primary;
 DWORD primary_id,wait_status;
 unsigned attempted,returned,dispatch_stopped,disconnected;
 int writer_result,result;
} sl_win_argument_reader_cancel;
/* Caller must bind genuine original D failure, finance, actual issuing-primary
 * association and retained outer custody before invoking this effect primitive.
 * Disconnect/cancel are not read/write completion or disposition authority. */
int sl_win_cancel_argument_reader(sl_win_argument_reader_cancel *original);
#endif
#endif
