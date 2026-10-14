#ifndef SERVICE_LASSO_DUPLEX_WINDOWS_H
#define SERVICE_LASSO_DUPLEX_WINDOWS_H
#ifdef _WIN32
#include "owner_windows.h"
struct sl_win_duplex;
typedef struct sl_win_duplex_completion {
 struct sl_win_duplex *owner_original;
 sl_win_call query;
 DWORD transferred;unsigned attempted,returned;int result;
} sl_win_duplex_completion;
typedef struct sl_win_duplex_settlement {
 struct sl_win_duplex *owner_original;sl_win_call query;
 ULONG_PTR internal_before,internal_after;DWORD transferred;
 unsigned macro_before,macro_after,attempted,returned,completed;
 int outcome;
} sl_win_duplex_settlement;
typedef struct sl_win_duplex {
 WCHAR *name,*actual_name;SIZE_T name_capacity,actual_name_capacity,name_units;
 void *token_body,*actual_token_body;DWORD token_capacity,actual_token_capacity,token_required;
 void *acl_body,*actual_acl_body;DWORD acl_capacity,actual_acl_capacity;
 DWORD input_bytes,output_bytes;
 HANDLE process,token,server,event,client,inherited_client;
 PSID user_sid;DWORD sid_bytes,acl_bytes;
 SECURITY_DESCRIPTOR descriptor;SECURITY_ATTRIBUTES attributes;OVERLAPPED connect;
 sl_win_duplex_completion *completions,*actual_completions;
 SIZE_T completion_capacity,actual_completion_capacity,completion_count;
 sl_win_duplex_completion *completed_by;
 sl_win_duplex_settlement *settlements,*actual_settlements,*settled_by;
 SIZE_T settlement_capacity,actual_settlement_capacity,settlement_count;
 sl_win_call cancel_request;unsigned cancel_attempted,cancel_returned,operation_settled;
 sl_win_call process_query,token_open,user_query,sid_valid,sid_length;
 sl_win_call acl_initialize,acl_add,descriptor_initialize,descriptor_dacl,descriptor_control;
 sl_win_call pid_query,tick_query,server_create,event_create,connect_begin,client_open,client_duplicate;
 DWORD pid;ULONGLONG tick;
 unsigned attempted,returned,pending,connected,exhausted,completion_failed;
 int result;
} sl_win_duplex;
/* 1 completed, 2 pending, 0 failed. None is native admission authority. */
int sl_win_construct_duplex(sl_win_duplex *owner);
/* Fresh finite observations only; pending IO retains SAME OVERLAPPED/event. */
int sl_win_observe_duplex_completion(sl_win_duplex *owner);
int sl_win_request_duplex_cancel(sl_win_duplex *owner);
int sl_win_observe_duplex_settlement(sl_win_duplex *owner);
#endif
#endif