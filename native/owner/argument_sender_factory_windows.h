#ifndef SERVICE_LASSO_ARGUMENT_SENDER_FACTORY_WINDOWS_H
#define SERVICE_LASSO_ARGUMENT_SENDER_FACTORY_WINDOWS_H
#ifdef _WIN32
#include "public_startup_windows.h"
/* Private once-only static backing association. Not a containing reservation,
 * authenticated factory entry, complete native factory or activation grant. */
int sl_win_bind_argument_sender_factory(sl_win_public_startup *original);
#endif
#endif
