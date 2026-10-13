#ifndef SERVICE_LASSO_PUBLIC_STARTUP_WINDOWS_H
#define SERVICE_LASSO_PUBLIC_STARTUP_WINDOWS_H
#ifdef _WIN32
#include "duplex_windows.h"
#include "channels_windows.h"
#include "private_arguments_windows.h"
#include "entry_auth_windows.h"
#include "argument_sender_windows.h"
typedef struct sl_win_public_startup {
 sl_win_public_controller controller;
 sl_win_duplex duplex;
 sl_win_channels channels;
 sl_win_private_arguments arguments;
 sl_win_endpoint_observation endpoint;
 sl_win_argument_sender argument_sender;
 sl_win_argument_send_sequence argument_sequence;
 SIZE_T command_line_capacity,actual_command_line_capacity;
 WCHAR *actual_command_line;
 int streams_result,channels_result,duplex_result,completion_result,endpoint_result,arguments_result,sender_result,sequence_result,controller_result,result;
 int cancellation_result,settlement_result;unsigned settlement_exhausted;
 unsigned attempted,returned,failed_stage;
} sl_win_public_startup;
/* Owns original streams -> actual duplex -> original argv -> suspended birth.
 * It neither grants admission nor resumes, disposes or exits the owner. */
int sl_win_construct_public_startup(sl_win_public_startup *owner);
/* Read-only prebinding exclusion of a private factory against every existing
 * startup backing region; no reservation, activation or native authority. */
int sl_win_public_startup_factory_disjoint(const sl_win_public_startup *owner,const void *factory,SIZE_T extent);
#endif
#endif
