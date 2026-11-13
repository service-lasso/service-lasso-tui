#ifndef SERVICE_LASSO_OWNER_CHANNEL_LINUX_H
#define SERVICE_LASSO_OWNER_CHANNEL_LINUX_H
#include "custody_linux.h"
#include "terminal_linux.h"
#include "input_workers_linux.h"
typedef struct sl_linux_terminal_request {
    sl_linux_custody_prefix prefix;
    sl_linux_custody_reply reply;
    sl_linux_window_observation window;
    sl_linux_mode_episode mode;
    sl_linux_mode_restore restoration;
    sl_linux_input_worker input;
    sl_linux_input_stop input_stop;
    unsigned char response[112];
    uint64_t discriminant;
    long operation_result;
    unsigned routed;
} sl_linux_terminal_request;
typedef struct sl_linux_owner_channel {
    sl_linux_local_collection *collection_original;
    sl_linux_terminal_views *terminal_original;
    sl_linux_terminal_request *requests;
    unsigned long capacity,count;
    int pty_fd,prepared_endpoint,prepared_pty_fd;
    const sl_linux_call *master_nonblocking_original,*prepared_master_nonblocking;
    sl_linux_terminal_request *prepared_requests;
    unsigned long prepared_capacity;
    sl_linux_local_collection *prepared_collection;
    sl_linux_terminal_views *prepared_terminal;
    const sl_linux_custody_prefix *local_prefix_original;
    long result;
    unsigned prepare_attempted,prepared,attempted,returned,exhausted;
} sl_linux_owner_channel;
long sl_linux_prepare_owner_channel(sl_linux_owner_channel *channel);
long sl_linux_serve_owner_channel(sl_linux_owner_channel *channel);
#endif
