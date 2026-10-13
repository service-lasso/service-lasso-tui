#ifndef SERVICE_LASSO_CUSTODY_LINUX_H
#define SERVICE_LASSO_CUSTODY_LINUX_H
#include "owner_linux.h"
typedef struct sl_linux_custody_io {
    sl_linux_call *attempts;
    unsigned long capacity,count,completed;
} sl_linux_custody_io;
typedef struct sl_linux_custody_prefix {
    unsigned char bytes[8];
    sl_linux_custody_io receive,prepared_receive;
    int endpoint_fd,prepared_endpoint;
    long result;
    unsigned prepare_attempted,prepared,attempted,returned;
} sl_linux_custody_prefix;
typedef struct sl_linux_custody_reply {
    unsigned char *bytes,*prepared_bytes;
    unsigned long length,prepared_length;
    int endpoint_fd,prepared_endpoint;
    sl_linux_custody_io send,prepared_send;
    long result;
    unsigned prepare_attempted,prepared,attempted,returned;
} sl_linux_custody_reply;
typedef struct sl_linux_local_collection {
    void *birth_original,*storage_original,*directory_original;
    int endpoint_fd,storage_fd,directory_fd;
    unsigned char *body,*readback;
    unsigned long capacity,body_length;
    unsigned char header[8],reply[8];
    const sl_linux_custody_prefix *header_original;
    sl_linux_custody_io receive_header,receive_body,write_body,read_body,send_reply;
    sl_linux_custody_io prepared_receive_header,prepared_receive_body,prepared_write_body,prepared_read_body,prepared_send_reply;
    sl_linux_call flush,directory_flush,readback_eof;
    unsigned char eof_byte;
    int prepared_endpoint,prepared_storage,prepared_directory;
    unsigned char *prepared_body,*prepared_readback;
    unsigned long prepared_capacity;
    void *prepared_birth,*prepared_storage_original,*prepared_directory_original;
    long result;
    unsigned prepare_attempted,prepared,attempted,returned,readback_equal;
} sl_linux_local_collection;
long sl_linux_prepare_local_collection(sl_linux_local_collection *collection);
long sl_linux_collect_local_representation(sl_linux_local_collection *collection);
long sl_linux_collect_prefixed_local_representation(sl_linux_local_collection *collection,const sl_linux_custody_prefix *prefix);
long sl_linux_prepare_custody_prefix(sl_linux_custody_prefix *prefix);
long sl_linux_read_custody_prefix(sl_linux_custody_prefix *prefix);
long sl_linux_prepare_custody_reply(sl_linux_custody_reply *reply);
long sl_linux_send_custody_reply(sl_linux_custody_reply *reply);
#endif
