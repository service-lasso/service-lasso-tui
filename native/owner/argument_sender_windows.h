#ifndef SERVICE_LASSO_ARGUMENT_SENDER_WINDOWS_H
#define SERVICE_LASSO_ARGUMENT_SENDER_WINDOWS_H
#ifdef _WIN32
#include "private_arguments_windows.h"
#include "duplex_windows.h"

typedef struct sl_win_argument_span {
 const unsigned char *bytes;
 DWORD extent;
 unsigned source_part; /* 0 header, 1 raw original, 2 module original */
 SIZE_T source_offset;
} sl_win_argument_span;

/* Four is source-derived: header + raw + at most two DWORD module chunks.
 * Planned views perform no native writes and authenticate no endpoint. */
typedef struct sl_win_argument_sender {
 const sl_win_private_arguments *arguments_original,*actual_arguments;
 const sl_win_duplex *duplex_original,*actual_duplex;
 HANDLE actual_server;
 sl_win_argument_span spans[4];
 SIZE_T span_count,body_extent;
 unsigned attempted,returned;
 int result;
} sl_win_argument_sender;
int sl_win_prepare_argument_sender(sl_win_argument_sender *original);

typedef struct sl_win_argument_write {
 struct sl_win_argument_worker *worker_original;
 sl_win_argument_sender *sender_original,*actual_sender;
 SIZE_T span_index;
 sl_win_argument_span actual_span;
 HANDLE actual_server,event;
 OVERLAPPED overlap;
 sl_win_call thread_query,event_create,write,worker_query,suspension;
 sl_win_call cancellation_check,worker_cancel;
 LONG cancellation_requested,worker_cancellation_observed;
 ULONG_PTR worker_internal_before,worker_internal_after;
 DWORD worker_transferred;
 unsigned worker_macro_before,worker_macro_after,worker_settled;
 DWORD issuing_thread_id;
 unsigned attempted,returned,initiation_supported,operation_settled,delivered;
 int result;
} sl_win_argument_write;

typedef struct sl_win_argument_worker {
 sl_win_argument_write *write_original,*actual_write;
 SIZE_T stack_reservation,actual_stack_reservation;
 HANDLE thread;
 DWORD thread_id,previous_suspend_count;
 sl_win_call create,resume;
 unsigned attempted,returned;
 int result;
} sl_win_argument_worker;
typedef struct sl_win_argument_worker_join {
 sl_win_argument_worker *worker_original,*actual_worker;
 sl_win_call wait,exit_query;
 DWORD status,exit_code;
 unsigned attempted,returned,terminal,completed;
} sl_win_argument_worker_join;
typedef struct sl_win_argument_write_cancel {
 sl_win_argument_worker *worker_original,*actual_worker;
 sl_win_call publication,request;
 LONG previous_cancellation;
 unsigned attempted,returned;
 int result;
} sl_win_argument_write_cancel;

typedef struct sl_win_argument_write_observation {
 sl_win_argument_write *write_original,*actual_write;
 sl_win_argument_worker_join *join_original,*actual_join;
 sl_win_call query;
 ULONG_PTR internal_before,internal_after;
 DWORD transferred;
 unsigned macro_before,macro_after,attempted,returned,settled,delivered;
 int result;
} sl_win_argument_write_observation;

/* Dedicated native worker entry: actual CreateThread/held native thread/stack/
 * token/source/lifetime factory binding is required before activation. A normal
 * C worker return is not native IO completion or public process success. */
DWORD WINAPI sl_win_argument_write_worker(LPVOID original);
int sl_win_start_argument_write_worker(sl_win_argument_worker *original);
int sl_win_observe_argument_worker_join(sl_win_argument_worker_join *original);
/* Fresh preowned observations only; no wait/retry loop, close or reuse. */
int sl_win_observe_argument_write(sl_win_argument_write_observation *original);
int sl_win_request_argument_write_cancel(sl_win_argument_write_cancel *original);

typedef struct sl_win_argument_send_sequence {
 sl_win_argument_sender *sender_original,*actual_sender;
 SIZE_T stack_reservation,actual_stack_reservation,span_count,active_span;
 struct sl_win_argument_send_turn *turns,*actual_turns;
 SIZE_T turn_capacity,actual_turn_capacity,turn_index;
 sl_win_argument_write writes[4];
 sl_win_argument_worker workers[4];
 unsigned attempted,returned,prepared,cancelled,failed,delivered,active_settled,turn_exhausted;
} sl_win_argument_send_sequence;
typedef struct sl_win_argument_send_turn {
 sl_win_argument_send_sequence *sequence_original,*actual_sequence;
 SIZE_T span;
 sl_win_argument_worker_join join;
 sl_win_argument_write_observation completion;
 int start_result,join_result,completion_result;
 unsigned attempted,returned,advanced,delivered,failed,settled;
} sl_win_argument_send_turn;
typedef struct sl_win_argument_send_cancellation {
 sl_win_argument_send_sequence *sequence_original,*actual_sequence;
 SIZE_T span;
 sl_win_argument_write_cancel cancellation;
 unsigned attempted,returned,dispatch_stopped;
 int request_result;
} sl_win_argument_send_cancellation;
/* Protocol scheduling only: genuine creator/entry/reader/D/finance and finite
 * preowned turn rosters must dominate activation. No close/reuse/disposition. */
int sl_win_prepare_argument_send_sequence(sl_win_argument_send_sequence *original);
int sl_win_service_argument_send(sl_win_argument_send_turn *original);
int sl_win_service_next_argument_send(sl_win_argument_send_sequence *original);
int sl_win_cancel_argument_send(sl_win_argument_send_cancellation *original);
#endif
#endif
