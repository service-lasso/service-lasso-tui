#ifndef SERVICE_LASSO_PRIVATE_ARGUMENTS_WINDOWS_H
#define SERVICE_LASSO_PRIVATE_ARGUMENTS_WINDOWS_H
#ifdef _WIN32
#include "owner_windows.h"
/* Fixed candidate-owned backing, bound before effects; quantities are source
 * configuration/comparison operands, never a runtime allocation request. */
typedef struct sl_win_module_query {
 struct sl_win_private_arguments *owner_original;
 WCHAR *actual_backing;
 SIZE_T actual_capacity;
 sl_win_call query;
 DWORD returned_units;
 unsigned attempted,returned;
} sl_win_module_query;
typedef struct sl_win_private_arguments {
 WCHAR *output,*actual_output,*module,*actual_module;
 SIZE_T output_capacity,module_capacity,original_capacity;
 SIZE_T actual_output_capacity,actual_module_capacity,actual_original_capacity;
 WCHAR *actual_module_arena;
 SIZE_T actual_module_arena_capacity;
 sl_win_module_query *module_queries,*actual_module_queries;
 SIZE_T module_query_capacity,actual_module_query_capacity,module_query_count;
 const WCHAR *raw_original,*public_original;
 SIZE_T raw_units,public_units,public_count,output_units;
 HANDLE endpoint_original;
 SIZE_T security_label_bytes,terminal_calls,representation_bytes;
 /* Fixed SLWARGV1 first-phase header and original body spans. No stream IO. */
 unsigned char transport_header[48];
 const WCHAR *transport_raw,*transport_module;
 SIZE_T transport_raw_units,transport_module_units,transport_body_bytes;
 unsigned transport_prepared;
 sl_win_call command_line_query;
 unsigned attempted,returned,empty_fallback,module_failed,exhausted;
 int result;
} sl_win_private_arguments;
int sl_win_construct_private_arguments(sl_win_private_arguments *owner);
#endif
#endif
