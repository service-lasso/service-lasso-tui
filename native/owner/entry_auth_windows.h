#ifndef SERVICE_LASSO_ENTRY_AUTH_WINDOWS_H
#define SERVICE_LASSO_ENTRY_AUTH_WINDOWS_H
#ifdef _WIN32
#include "owner_windows.h"

typedef struct sl_win_endpoint_observation {
 HANDLE endpoint_original,actual_endpoint;
 sl_win_call server_process_query,information_query;
 DWORD server_process_id,flags,outbound_bytes,inbound_bytes,maximum_instances;
 sl_win_object_security security;
 int result;
 unsigned attempted,returned;
} sl_win_endpoint_observation;

/* Genuine native acquisition, not creator authentication. The acquired server
 * ID, pipe properties and descriptor must join held process/image/token/birth/
 * loaded-provider/own-writer/lifetime originals before bootstrap bytes. */
int sl_win_observe_entry_endpoint(sl_win_endpoint_observation *original);
#endif
#endif
