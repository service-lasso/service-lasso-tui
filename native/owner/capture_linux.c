#if defined(__linux__) && defined(__x86_64__)
#include "capture_linux.h"
#include <sys/syscall.h>
#include <errno.h>
long sl_linux_capture_prepare(sl_linux_capture_frame *frame) {
    unsigned long index;
    if (!frame || frame->prepare_attempted || frame->ready_attempted || frame->read_attempted || frame->forward_attempted) return -1;
    frame->prepare_attempted=1;
    if (!frame->writer_original || frame->input_fd<0 || !frame->public_original ||
        !frame->public_writes || !frame->controller_signal || frame->public_write_capacity<sizeof(frame->bytes)) return -1;
    if (!atomic_load_explicit(&frame->public_original->original_flags.returned,memory_order_acquire)) return -1;
    if (frame->public_original->original_flags.result==-EBADF && frame->public_original->observed_absent) {
        frame->prepared_public_fd=-1;
    } else {
        if (frame->public_original->original_flags.result<0 ||
            !atomic_load_explicit(&frame->public_original->duplicate.returned,memory_order_acquire) ||
            frame->public_original->duplicate.result<3 || frame->public_original->duplicate.result>2147483647L) return -1;
        frame->prepared_public_fd=(int)frame->public_original->duplicate.result;
    }
    atomic_init(&frame->public_thread.entered,0);atomic_init(&frame->public_thread.returned,0);
    atomic_init(&frame->public_pending_before.entered,0);atomic_init(&frame->public_pending_before.returned,0);
    atomic_init(&frame->public_pending_after.entered,0);atomic_init(&frame->public_pending_after.returned,0);
    atomic_init(&frame->ready.entered,0);atomic_init(&frame->ready.returned,0);
    atomic_init(&frame->read.entered,0);
    atomic_init(&frame->read.returned,0);
    for (index=0;index<frame->public_write_capacity;index++) {
        atomic_init(&frame->public_writes[index].entered,0);
        atomic_init(&frame->public_writes[index].returned,0);
    }
    frame->prepared=1;
    return 0;
}
/* The exact source frame and actual writer association belong to the outside
 * owner before read. No decoder, allocation, forwarding or EOF projection can
 * intervene between the kernel return and retention of the raw result. */
long sl_linux_capture_read(sl_linux_capture_frame *frame) {
    long result;
    if (!frame || !frame->prepared || frame->ready_attempted || frame->read_attempted || !frame->writer_original || frame->input_fd<0) return -1;
    frame->ready_attempted=1;
    frame->readiness=(struct pollfd){.fd=frame->input_fd,.events=POLLIN};
    result=sl_linux_call6(&frame->ready,"capture.raw.poll",SYS_poll,(long)&frame->readiness,1,-1,0,0,0);
    if(result<=0||frame->readiness.revents&POLLNVAL||!(frame->readiness.revents&(POLLIN|POLLHUP|POLLERR)))return result<0?result:-1;
    frame->read_attempted=1;
    result=sl_linux_call6(&frame->read,"capture.raw.read",SYS_read,
                         frame->input_fd,(long)frame->bytes,sizeof(frame->bytes),0,0,0);
    frame->raw_count=result;
    frame->read_returned=1;
    /* Zero is only this descriptor's observed read EOF, never family zero.
     * Negative result and invalid extent remain actual frame observations. */
    return result;
}

long sl_linux_capture_forward(sl_linux_capture_frame *frame) {
    long result;
    if (!frame || !frame->prepared || frame->raw_only || frame->forward_attempted || !frame->read_returned || frame->raw_count<=0 ||
        (unsigned long)frame->raw_count>sizeof(frame->bytes) ||
        !frame->public_writes || frame->public_write_capacity<(unsigned long)frame->raw_count) return -1;
    frame->forward_attempted=1;
    /* Borrow the same actual controller mask episode established after birth.
     * Never unblock it between frames while original graphs remain live. */
    if (!frame->controller_signal || !frame->controller_signal->blocked ||
        frame->controller_signal->finish_attempted ||
        !atomic_load_explicit(&frame->controller_signal->pending_before.returned,memory_order_acquire) ||
        frame->controller_signal->pending_before.result<0) return -1;
    result=sl_linux_call6(&frame->public_thread,"capture.public.controller-thread",SYS_gettid,0,0,0,0,0,0);
    if (result<0 || result!=frame->controller_signal->thread.result) return -1;
    result=sl_linux_call6(&frame->public_pending_before,"capture.public.pending-before",SYS_rt_sigpending,
                         (long)&frame->public_pending_before_bits,8,0,0,0,0);
    frame->public_signal_result=result;
    if (result<0) return result;
    /* Capacity covers the genuine worst progress of one byte per write. It
     * must already be held by the caller; this is not a whole-fit admission.
     * Each short/error/zero result has its own never-reused actual native cell.
     * No retry follows an uncertain/error return, including EINTR. */
    while (frame->forwarded<(unsigned long)frame->raw_count) {
        unsigned long remaining=(unsigned long)frame->raw_count-frame->forwarded;
        sl_linux_call *call=&frame->public_writes[frame->public_write_count++];
        result=sl_linux_call6(call,"capture.public.write",SYS_write,
                             frame->prepared_public_fd,(long)(frame->bytes+frame->forwarded),remaining,0,0,0);
        if (result<=0 || (unsigned long)result>remaining) {
            frame->public_signal_result=sl_linux_call6(&frame->public_pending_after,"capture.public.pending-after",SYS_rt_sigpending,
                         (long)&frame->public_pending_after_bits,8,0,0,0,0);
            frame->forward_returned=1;
            return result;
        }
        frame->forwarded+=(unsigned long)result;
    }
    frame->public_signal_result=sl_linux_call6(&frame->public_pending_after,"capture.public.pending-after",SYS_rt_sigpending,
                         (long)&frame->public_pending_after_bits,8,0,0,0,0);
    frame->forward_returned=1;
    if (frame->public_signal_result<0) return frame->public_signal_result;
    return frame->forwarded;
}
#endif