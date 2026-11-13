#ifndef SERVICE_LASSO_ARGUMENT_SENDER_FACTORY_WINDOWS_H
#define SERVICE_LASSO_ARGUMENT_SENDER_FACTORY_WINDOWS_H
#ifdef _WIN32
#include "public_startup_windows.h"
typedef struct sl_win_argument_sender_factory {
 sl_win_public_startup *owner_original;
 sl_win_argument_send_turn *turns_original;
 SIZE_T turn_capacity,stack_reservation;
 unsigned attempted,returned;
} sl_win_argument_sender_factory;
/* Private once-only per-entry backing association. Not a containing reservation,
 * authenticated factory entry, complete native factory or activation grant. */
int sl_win_bind_argument_sender_factory(sl_win_argument_sender_factory *,sl_win_public_startup *,
 sl_win_argument_send_turn *,SIZE_T,SIZE_T);
#endif
#endif
