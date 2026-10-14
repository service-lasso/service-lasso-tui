#if defined(__linux__) && defined(__x86_64__)
#include "custody_linux.h"
#include <sys/syscall.h>
#include <sys/socket.h>
static int sl_custody_io_prepare(sl_linux_custody_io *io,unsigned long required) {
    unsigned long index;
    if (!io->attempts || io->capacity<required || io->count || io->completed ||
        io->capacity>UINTPTR_MAX/sizeof(*io->attempts) ||
        io->capacity*sizeof(*io->attempts)>UINTPTR_MAX-(uintptr_t)io->attempts) return -1;
    for (index=0;index<io->capacity;index++) {
        atomic_init(&io->attempts[index].entered,0);
        atomic_init(&io->attempts[index].returned,0);
    }
    return 0;
}
long sl_linux_prepare_local_collection(sl_linux_local_collection *collection) {
    uintptr_t body,readback;
    sl_linux_custody_io *inputs[5],*prepared[5];
    unsigned long index,other;
    if (!collection || collection->prepare_attempted) return -1;
    collection->prepare_attempted=1;
    if (!collection->birth_original || !collection->storage_original || !collection->directory_original ||
        collection->endpoint_fd<0 || collection->storage_fd<0 || collection->directory_fd<0 ||
        collection->directory_fd==collection->endpoint_fd || collection->directory_fd==collection->storage_fd ||
        collection->endpoint_fd==collection->storage_fd || !collection->body ||
        !collection->readback || !collection->capacity || collection->capacity>((unsigned long)-1>>1)) return -1;
    body=(uintptr_t)collection->body;readback=(uintptr_t)collection->readback;
    if (collection->capacity>UINTPTR_MAX-body || collection->capacity>UINTPTR_MAX-readback ||
        sizeof(*collection)>UINTPTR_MAX-(uintptr_t)collection ||
        (body<readback+collection->capacity && readback<body+collection->capacity) ||
        (body<(uintptr_t)collection+sizeof(*collection) && (uintptr_t)collection<body+collection->capacity) ||
        (readback<(uintptr_t)collection+sizeof(*collection) && (uintptr_t)collection<readback+collection->capacity)) return -1;
    inputs[0]=&collection->receive_header;inputs[1]=&collection->receive_body;
    inputs[2]=&collection->write_body;inputs[3]=&collection->read_body;inputs[4]=&collection->send_reply;
    prepared[0]=&collection->prepared_receive_header;prepared[1]=&collection->prepared_receive_body;
    prepared[2]=&collection->prepared_write_body;prepared[3]=&collection->prepared_read_body;
    prepared[4]=&collection->prepared_send_reply;
    for (index=0;index<5;index++) {
        uintptr_t start=(uintptr_t)inputs[index]->attempts;
        unsigned long bytes;
        if (inputs[index]->capacity>UINTPTR_MAX/sizeof(sl_linux_call)) return -1;
        bytes=inputs[index]->capacity*sizeof(sl_linux_call);
        if (bytes>UINTPTR_MAX-start ||
            (start<(uintptr_t)collection+sizeof(*collection) && (uintptr_t)collection<start+bytes) ||
            (start<body+collection->capacity && body<start+bytes) ||
            (start<readback+collection->capacity && readback<start+bytes)) return -1;
        for (other=0;other<index;other++) {
            uintptr_t previous=(uintptr_t)inputs[other]->attempts;
            if (start<previous+inputs[other]->capacity*sizeof(sl_linux_call) && previous<start+bytes) return -1;
        }
        *prepared[index]=*inputs[index];
    }
    if (sl_custody_io_prepare(prepared[0],8) || sl_custody_io_prepare(prepared[1],collection->capacity) ||
        sl_custody_io_prepare(prepared[2],collection->capacity) || sl_custody_io_prepare(prepared[3],collection->capacity) ||
        sl_custody_io_prepare(prepared[4],8)) return -1;
    atomic_init(&collection->flush.entered,0);atomic_init(&collection->flush.returned,0);
    atomic_init(&collection->directory_flush.entered,0);atomic_init(&collection->directory_flush.returned,0);
    atomic_init(&collection->readback_eof.entered,0);atomic_init(&collection->readback_eof.returned,0);
    collection->prepared_directory=collection->directory_fd;collection->prepared_directory_original=collection->directory_original;
    collection->prepared_endpoint=collection->endpoint_fd;
    collection->prepared_storage=collection->storage_fd;
    collection->prepared_body=collection->body;collection->prepared_readback=collection->readback;
    collection->prepared_capacity=collection->capacity;
    collection->prepared_birth=collection->birth_original;
    collection->prepared_storage_original=collection->storage_original;
    collection->prepared=1;
    return 0;
}
enum sl_custody_operation { SL_CUSTODY_RECEIVE,SL_CUSTODY_PERSIST,SL_CUSTODY_READBACK,SL_CUSTODY_REPLY };
static long sl_custody_exact(sl_linux_custody_io *io,enum sl_custody_operation operation,
                            int fd,unsigned char *bytes,unsigned long length) {
    while (io->completed<length) {
        sl_linux_call *call;
        unsigned long remaining=length-io->completed;
        long result;
        if (io->count>=io->capacity) return -1;
        call=&io->attempts[io->count++];
        if (operation==SL_CUSTODY_RECEIVE)
            result=sl_linux_call6(call,"custody.local.receive",SYS_read,fd,(long)(bytes+io->completed),remaining,0,0,0);
        else if (operation==SL_CUSTODY_PERSIST)
            result=sl_linux_call6(call,"custody.local.persist",SYS_pwrite64,fd,(long)(bytes+io->completed),remaining,io->completed,0,0);
        else if (operation==SL_CUSTODY_READBACK)
            result=sl_linux_call6(call,"custody.local.held-readback",SYS_pread64,fd,(long)(bytes+io->completed),remaining,io->completed,0,0);
        else
            /* Socket-local suppression affects only this send, not child or
             * process signal policy. Actual EPIPE remains in the raw cell. */
            result=sl_linux_call6(call,"custody.local.reply",SYS_sendto,fd,(long)(bytes+io->completed),remaining,MSG_NOSIGNAL,0,0);
        if (result<=0 || (unsigned long)result>remaining) return result<0?result:-1;
        io->completed+=(unsigned long)result;
    }
    return (long)io->completed;
}
long sl_linux_prepare_custody_prefix(sl_linux_custody_prefix *prefix) {
    if (!prefix || prefix->prepare_attempted || prefix->attempted) return -1;
    prefix->prepare_attempted=1;
    if (prefix->endpoint_fd<0) return -1;
    prefix->prepared_receive=prefix->receive;
    if (sl_custody_io_prepare(&prefix->prepared_receive,8)) return -1;
    prefix->prepared_endpoint=prefix->endpoint_fd;prefix->prepared=1;
    return 0;
}
long sl_linux_read_custody_prefix(sl_linux_custody_prefix *prefix) {
    if (!prefix || !prefix->prepared || prefix->attempted) return -1;
    prefix->attempted=1;
    prefix->result=sl_custody_exact(&prefix->prepared_receive,SL_CUSTODY_RECEIVE,
                                   prefix->prepared_endpoint,prefix->bytes,8);
    prefix->returned=1;
    return prefix->result;
}
long sl_linux_prepare_custody_reply(sl_linux_custody_reply *reply) {
    if (!reply || reply->prepare_attempted || reply->attempted) return -1;
    reply->prepare_attempted=1;
    if (!reply->bytes || !reply->length || reply->length>((unsigned long)-1>>1) || reply->endpoint_fd<0) return -1;
    reply->prepared_send=reply->send;
    if (sl_custody_io_prepare(&reply->prepared_send,reply->length)) return -1;
    reply->prepared_bytes=reply->bytes;reply->prepared_length=reply->length;
    reply->prepared_endpoint=reply->endpoint_fd;reply->prepared=1;
    return 0;
}
long sl_linux_send_custody_reply(sl_linux_custody_reply *reply) {
    if (!reply || !reply->prepared || reply->attempted) return -1;
    reply->attempted=1;
    reply->result=sl_custody_exact(&reply->prepared_send,SL_CUSTODY_REPLY,
                                 reply->prepared_endpoint,reply->prepared_bytes,reply->prepared_length);
    reply->returned=1;
    return reply->result;
}
static long sl_linux_collect_local_body(sl_linux_local_collection *collection) {
    unsigned long index;
    uint64_t length=0;
    long result;
    for (index=0;index<8;index++) length|=(uint64_t)collection->header[index]<<(index*8);
    if (length>collection->prepared_capacity) {result=-1;goto finished;}
    collection->body_length=(unsigned long)length;
    result=sl_custody_exact(&collection->prepared_receive_body,SL_CUSTODY_RECEIVE,
                          collection->prepared_endpoint,collection->prepared_body,collection->body_length);
    if (result<0) goto finished;
    result=sl_custody_exact(&collection->prepared_write_body,SL_CUSTODY_PERSIST,
                          collection->prepared_storage,collection->prepared_body,collection->body_length);
    if (result<0) goto finished;
    result=sl_linux_call6(&collection->flush,"custody.local.flush",SYS_fsync,collection->prepared_storage,0,0,0,0,0);
    if (result<0) goto finished;
    result=sl_linux_call6(&collection->directory_flush,"custody.local.directory-flush",SYS_fsync,collection->prepared_directory,0,0,0,0,0);
    if (result<0) goto finished;
    result=sl_custody_exact(&collection->prepared_read_body,SL_CUSTODY_READBACK,
                          collection->prepared_storage,collection->prepared_readback,collection->body_length);
    if (result<0) goto finished;
    result=sl_linux_call6(&collection->readback_eof,"custody.local.held-readback-eof",SYS_pread64,collection->prepared_storage,(long)&collection->eof_byte,1,collection->body_length,0,0);
    if (result!=0) {if(result>0)result=-1;goto finished;}
    for (index=0;index<collection->body_length;index++)
        if (collection->prepared_body[index]!=collection->prepared_readback[index]) {result=-1;goto finished;}
    collection->readback_equal=1;
    for (index=0;index<8;index++) collection->reply[index]=collection->header[index];
    result=sl_custody_exact(&collection->prepared_send_reply,SL_CUSTODY_REPLY,
                          collection->prepared_endpoint,collection->reply,8);
finished:
    collection->result=result;collection->returned=1;
    /* This is exact wire/storage observation only. Native endpoint/birth,
     * private fresh namespace/exclusion and required source-body authentication
     * must be genuinely bound by controller before any local disposition.
     * No close/retry/family-EOF/FINAL or graph transfer is performed here. */
    return result;
}
long sl_linux_collect_local_representation(sl_linux_local_collection *collection) {
    long result;
    if (!collection || !collection->prepared || collection->attempted) return -1;
    collection->attempted=1;
    result=sl_custody_exact(&collection->prepared_receive_header,SL_CUSTODY_RECEIVE,
                          collection->prepared_endpoint,collection->header,8);
    if (result<0) {collection->result=result;collection->returned=1;return result;}
    return sl_linux_collect_local_body(collection);
}
long sl_linux_collect_prefixed_local_representation(sl_linux_local_collection *collection,const sl_linux_custody_prefix *prefix) {
    unsigned index;
    if (!collection || !collection->prepared || collection->attempted || !prefix) return -1;
    collection->attempted=1;
    collection->header_original=prefix;
    if (!prefix->prepared || !prefix->returned || prefix->result!=8 ||
        prefix->prepared_receive.completed!=8 || prefix->prepared_endpoint!=collection->prepared_endpoint) {
        collection->result=-1;collection->returned=1;return -1;
    }
    /* Retain SAME actual prefix/read graph; this copy is the collection's
     * derived length/reply representation, never a fabricated native read. */
    for (index=0;index<8;index++) collection->header[index]=prefix->bytes[index];
    return sl_linux_collect_local_body(collection);
}
#endif
