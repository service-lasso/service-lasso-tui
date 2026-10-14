#ifndef SERVICE_LASSO_CAPTURE_LINUX_H
#define SERVICE_LASSO_CAPTURE_LINUX_H
#ifdef __linux__
#include "owner_linux.h"
#include "signal_linux.h"
#include <stddef.h>
#include <poll.h>
/* One nonreused frame of an admitted whole-profile backing. This geometry is
 * not a stream limit. The controller must supply additional preowned frames
 * and independent failure capacity before effects, never overwrite a frame. */
typedef struct sl_linux_capture_frame {
    void *writer_original;
    struct sl_linux_schedule_wake *schedule_wake;
    int input_fd;
    const sl_linux_public_stream *public_original;
    int prepared_public_fd;
    unsigned char bytes[16384];
    sl_linux_call ready,read;
    struct pollfd readiness;
    sl_linux_write_signal *controller_signal;
    sl_linux_call public_thread,public_pending_before,public_pending_after;
    uint64_t public_pending_before_bits,public_pending_after_bits;
    long public_signal_result;
    sl_linux_call *public_writes;
    unsigned long public_write_capacity,public_write_count;
    long raw_count;
    unsigned long forwarded;
    unsigned raw_only; /* controlling input-only PTY: retain, never misroute to stdout */
    unsigned prepared,prepare_attempted;
    unsigned ready_attempted,read_attempted,read_returned,forward_attempted,forward_returned;
} sl_linux_capture_frame;
long sl_linux_capture_prepare(sl_linux_capture_frame *frame);
long sl_linux_capture_read(sl_linux_capture_frame *frame);
long sl_linux_capture_forward(sl_linux_capture_frame *frame);
#endif
#endif