#if defined(__linux__) && defined(__x86_64__)
#include "owner_channel_linux.h"
static void sl_channel_u64(unsigned char *bytes,uint64_t value) {
    unsigned index;
    for (index=0;index<8;index++) bytes[index]=(unsigned char)(value>>(index*8));
}
static uint64_t sl_channel_read_u64(const unsigned char *bytes) {
    unsigned index;
    uint64_t value=0;
    for (index=0;index<8;index++) value|=(uint64_t)bytes[index]<<(index*8);
    return value;
}
long sl_linux_prepare_owner_channel(sl_linux_owner_channel *channel) {
    unsigned long index;
    if (!channel || channel->prepare_attempted || channel->attempted) return -1;
    channel->prepare_attempted=1;
    if (!channel->collection_original || !channel->collection_original->prepared ||
        !channel->terminal_original || !channel->terminal_original->attempted ||
        !channel->requests || !channel->capacity || channel->pty_fd<3 ||
        channel->capacity>UINTPTR_MAX/sizeof(*channel->requests) ||
        channel->capacity*sizeof(*channel->requests)>UINTPTR_MAX-(uintptr_t)channel->requests) return -1;
    channel->prepared_endpoint=channel->collection_original->prepared_endpoint;
    channel->prepared_pty_fd=channel->pty_fd;channel->prepared_master_nonblocking=channel->master_nonblocking_original;
    channel->prepared_collection=channel->collection_original;
    channel->prepared_terminal=channel->terminal_original;
    channel->prepared_requests=channel->requests;channel->prepared_capacity=channel->capacity;
    for (index=0;index<channel->prepared_capacity;index++) {
        sl_linux_terminal_request *request=&channel->prepared_requests[index];
        request->prefix.endpoint_fd=channel->prepared_endpoint;
        request->reply.endpoint_fd=channel->prepared_endpoint;
        request->reply.bytes=request->response;request->reply.length=sizeof(request->response);
        request->mode.attempted=0;request->mode.restore_original=0;request->mode.input_original=0;
        request->input.owner_original=request;
        if(sl_linux_prepare_terminal_input(&request->input)<0)return -1;
        request->restoration.attempted=0;request->restoration.mode_original=0;
        request->mode.terminal_original=channel->prepared_terminal;
        request->mode.query.result=0;request->mode.change.result=0;request->restoration.restore.result=0;
        atomic_init(&request->mode.query.entered,0);atomic_init(&request->mode.query.returned,0);
        atomic_init(&request->mode.change.entered,0);atomic_init(&request->mode.change.returned,0);
        atomic_init(&request->restoration.restore.entered,0);atomic_init(&request->restoration.restore.returned,0);
        request->window.terminal_original=channel->prepared_terminal;
        request->window.pty_fd=channel->prepared_pty_fd;
        request->window.query.result=0;request->window.copy.result=0;
        request->window.window=(sl_linux_window_size){0};
        atomic_init(&request->window.query.entered,0);atomic_init(&request->window.query.returned,0);
        atomic_init(&request->window.copy.entered,0);atomic_init(&request->window.copy.returned,0);
        if (sl_linux_prepare_custody_prefix(&request->prefix)<0 ||
            sl_linux_prepare_custody_reply(&request->reply)<0) return -1;
    }
    channel->prepared=1;
    return 0;
}
static void sl_linux_terminal_response(sl_linux_owner_channel *channel,sl_linux_terminal_request *request) {
    sl_linux_terminal_views *terminal=channel->prepared_terminal;
    uint64_t entered=0;
    unsigned index;
    if (atomic_load_explicit(&terminal->input_query.entered,memory_order_acquire)) entered|=1;
    if (atomic_load_explicit(&terminal->output_query.entered,memory_order_acquire)) entered|=2;
    if (atomic_load_explicit(&terminal->fallback_open.entered,memory_order_acquire)) entered|=4;
    if (atomic_load_explicit(&terminal->fallback_query.entered,memory_order_acquire)) entered|=8;
    if (atomic_load_explicit(&request->window.query.entered,memory_order_acquire)) entered|=16;
    if (atomic_load_explicit(&request->window.copy.entered,memory_order_acquire)) entered|=32;
    if (atomic_load_explicit(&request->mode.query.entered,memory_order_acquire)) entered|=64;
    if (atomic_load_explicit(&request->mode.change.entered,memory_order_acquire)) entered|=128;
    if (atomic_load_explicit(&request->restoration.restore.entered,memory_order_acquire)) entered|=256;
    sl_channel_u64(request->response,2);
    sl_channel_u64(request->response+8,request->discriminant);
    sl_channel_u64(request->response+16,entered);
    sl_channel_u64(request->response+24,(uint64_t)terminal->input_query.result);
    sl_channel_u64(request->response+32,(uint64_t)terminal->output_query.result);
    sl_channel_u64(request->response+40,(uint64_t)terminal->fallback_open.result);
    sl_channel_u64(request->response+48,(uint64_t)terminal->fallback_query.result);
    sl_channel_u64(request->response+56,(uint64_t)request->window.query.result);
    sl_channel_u64(request->response+64,(uint64_t)request->window.copy.result);
    sl_channel_u64(request->response+72,(uint64_t)request->operation_result);
    sl_channel_u64(request->response+88,(uint64_t)request->mode.query.result);
    sl_channel_u64(request->response+96,(uint64_t)request->mode.change.result);
    sl_channel_u64(request->response+104,(uint64_t)request->restoration.restore.result);
    for (index=0;index<4;index++) {
        uint16_t value=index==0?request->window.window.rows:index==1?request->window.window.columns:
                       index==2?request->window.window.x_pixels:request->window.window.y_pixels;
        request->response[80+index*2]=(unsigned char)value;
        request->response[81+index*2]=(unsigned char)(value>>8);
    }
}
long sl_linux_serve_owner_channel(sl_linux_owner_channel *channel) {
    unsigned long index;
    long result=-1;
    if (!channel || !channel->prepared || channel->attempted) return -1;
    channel->attempted=1;
    /* Exactly ONE reader consumes this endpoint for terminal requests AND
     * local collection. Never start a competing collection/socket reader. */
    for (index=0;index<channel->prepared_capacity;index++) {
        sl_linux_terminal_request *request=&channel->prepared_requests[index];
        uint64_t operation;
        channel->count=index+1;
        result=sl_linux_read_custody_prefix(&request->prefix);
        if (result!=8) goto finished;
        request->discriminant=sl_channel_read_u64(request->prefix.bytes);
        if (!(request->discriminant & (UINT64_C(1)<<63))) {
            channel->local_prefix_original=&request->prefix;
            result=sl_linux_collect_prefixed_local_representation(channel->prepared_collection,&request->prefix);
            goto finished;
        }
        operation=request->discriminant & ~(UINT64_C(1)<<63);
        /* Complete discriminant precedes source-specific operation. Further
         * raw/input/restore bodies are a retained unfinished source obligation;
         * unknown operation never reaches storage or a guessed native call. */
        if (operation<1 || operation>4) {result=-1;goto finished;}
        request->routed=1;
        if (operation==1) request->operation_result=sl_linux_open_public_input_fallback(channel->prepared_terminal);
                else if (operation==2) {
            request->operation_result=sl_linux_make_public_raw(&request->mode);
            if(request->operation_result==0){request->operation_result=sl_linux_start_terminal_input(&request->input,&request->mode,channel->prepared_pty_fd,channel->prepared_master_nonblocking);}
        }
        else if (operation==3) request->operation_result=sl_linux_observe_public_window(&request->window);
        else {
            sl_linux_mode_episode *mode=channel->prepared_terminal->active_mode;
            long input_result=0;
            if(mode&&mode->input_original)input_result=sl_linux_stop_join_terminal_input(&request->input_stop,mode->input_original);
            if(input_result!=0)request->operation_result=input_result;
            else {
                request->operation_result=sl_linux_restore_public_mode(channel->prepared_terminal,&request->restoration);
                if(mode&&mode->input_original&&mode->input_original->create_returned&&!mode->input_original->create_result&&
                   mode->input_original->outcome!=SL_INPUT_STOPPED&&mode->input_original->outcome!=SL_INPUT_EOF&&request->operation_result==0)request->operation_result=-1;
            }
        }
        sl_linux_terminal_response(channel,request);
        result=sl_linux_send_custody_reply(&request->reply);
        if (result!=(long)sizeof(request->response)) goto finished;
    }
    channel->exhausted=1;result=-1;
finished:
    channel->result=result;channel->returned=1;
    /* Replies carry raw observations only. No native/source/endpoint trust,
     * local disposition, native recovery, family EOF or FINAL follows here. */
    return result;
}
#endif
