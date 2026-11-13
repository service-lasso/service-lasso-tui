#ifndef SERVICE_LASSO_IMMUTABLE_PAYLOAD_LINUX_H
#define SERVICE_LASSO_IMMUTABLE_PAYLOAD_LINUX_H
#include "local_installation_linux.h"
typedef struct sl_linux_payload_attempt {
 struct sl_linux_payload_frame *frame_original;unsigned attempted;
 sl_linux_call call;long raw_count;
} sl_linux_payload_attempt;
typedef struct sl_linux_payload_frame {
 struct sl_linux_immutable_payload *owner_original;
 unsigned char *source_bytes,*readback_bytes;unsigned long capacity;
 sl_linux_call read;long raw_count;unsigned long offset,written,readback_count;
 sl_linux_payload_attempt *writes,*readbacks;unsigned long write_capacity,readback_capacity,write_count,read_count;
} sl_linux_payload_frame;
typedef struct sl_linux_immutable_payload {
 sl_linux_local_installation *installation_original;
 sl_linux_payload_frame *frames,*prepared_frames;unsigned long capacity,prepared_capacity,count,extent;
 sl_linux_call create,add_seals,get_seals,readback_eof;sl_linux_fd_lift lift;
 int fd;unsigned char eof_byte;long raw_eof_count;
 unsigned attempted,returned,source_eof,seals_observed,readback_complete;long result;
} sl_linux_immutable_payload;
/* Immutable held copy only: no digest/loaded/native admission grant. */
long sl_linux_prepare_immutable_payload(sl_linux_immutable_payload *payload);
#endif