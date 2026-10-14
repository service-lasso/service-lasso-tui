#if defined(__linux__) && defined(__x86_64__)
#include "schedule_linux.h"
#include "capture_workers_linux.h"
#include "forward_workers_linux.h"
#include <errno.h>
static void *sl_linux_raw_reader(void *argument) {
    sl_linux_capture_worker *worker=argument;
    unsigned long index;
    atomic_store_explicit(&worker->entered,1,memory_order_release);
    for (index=0;index<worker->prepared_capacity;index++) {
        sl_linux_capture_frame *frame=&worker->prepared_frames[index];
        long result;
        worker->read_count=index+1;
        result=sl_linux_capture_read(frame);
        atomic_store_explicit(&worker->completed_count,index+1,memory_order_release);
        if(worker->prepared_frames[index].schedule_wake)sl_linux_schedule_notify(worker->prepared_frames[index].schedule_wake);
        if(worker->forward_original)sl_linux_notify_forward_worker(worker->forward_original,
                           &worker->forward_original->prepared_publications[index]);
        if (result==-EAGAIN&&frame->read_returned) continue;
        if (result<0) {worker->outcome=SL_READER_READ_FAILURE;break;}
        if ((unsigned long)result>sizeof(frame->bytes)) {worker->outcome=SL_READER_INVALID_EXTENT;break;}
        if (!result) {worker->outcome=SL_READER_READ_EOF;break;}
    }
    if (index==worker->prepared_capacity) worker->outcome=SL_READER_BACKING_EXHAUSTED;
    worker->worker_result=worker;
    atomic_store_explicit(&worker->returned,1,memory_order_release);
    if(worker->terminal_wake)sl_linux_schedule_notify(worker->terminal_wake);
    if(worker->forward_original)sl_linux_notify_forward_worker(worker->forward_original,
                           &worker->forward_original->terminal_notification);
    /* No public forwarding here: a blocked public pipe cannot prevent this
     * independent source from retaining raw bytes in its own finite backing.
     * Return ends this producer only; outer originals/backing remain live. */
    return worker->worker_result;
}
int sl_linux_prepare_raw_readers(sl_linux_capture_worker *workers,unsigned long count) {
    unsigned long index,frame_index,other;
    if (!workers || !count) return -1;
    /* Validate the whole connected source set before the first dispatch. */
    for (index=0;index<count;index++) {
        sl_linux_capture_worker *worker=&workers[index];
        if(worker->forward_original&&
          (worker->forward_original->prepared_frames!=worker->frames||
           worker->forward_original->prepared_frame_capacity!=worker->capacity))return -1;
        if (!worker->controller_original || !worker->frames || !worker->capacity || worker->prepared || worker->start_attempted ||
            worker->capacity>UINTPTR_MAX/sizeof(*worker->frames) ||
            worker->capacity*sizeof(*worker->frames)>UINTPTR_MAX-(uintptr_t)worker->frames) return -1;
        for (other=0;other<index;other++) {
            uintptr_t start=(uintptr_t)worker->frames,previous=(uintptr_t)workers[other].frames;
            if (worker->frames[0].input_fd==workers[other].frames[0].input_fd ||
                worker->frames[0].writer_original==workers[other].frames[0].writer_original) return -1;
            if (start<previous+workers[other].capacity*sizeof(*worker->frames) &&
                previous<start+worker->capacity*sizeof(*worker->frames)) return -1;
        }
        for (frame_index=0;frame_index<worker->capacity;frame_index++) {
            sl_linux_capture_frame *frame=&worker->frames[frame_index];
            if (!frame->prepared || frame->ready_attempted || frame->read_attempted ||
                frame->input_fd!=worker->frames[0].input_fd ||
                frame->writer_original!=worker->frames[0].writer_original) return -1;
        }
    }
    for (index=0;index<count;index++) {
        sl_linux_capture_worker *worker=&workers[index];
        worker->prepared_frames=worker->frames;worker->prepared_capacity=worker->capacity;
        atomic_init(&worker->entered,0);atomic_init(&worker->returned,0);
        atomic_init(&worker->completed_count,0);
        worker->prepared=1;
    }
    return 0;
}
int sl_linux_start_raw_readers(sl_linux_capture_worker *workers,unsigned long count) {
    unsigned long index;
    if(!workers||!count)return -1;
    /* All publication atomics and backing must precede every borrower,
     * including forwarding threads. Dispatch never reinitializes them. */
    for(index=0;index<count;index++)if(!workers[index].prepared||workers[index].start_attempted)return -1;
    for(index=0;index<count;index++){
        sl_linux_capture_worker *worker=&workers[index];
        worker->start_attempted=1;
        worker->create_result=pthread_create(&worker->thread,0,sl_linux_raw_reader,worker);
        worker->create_returned=1;
        /* Partial dispatch stays owned; do not cancel/join earlier readers in
         * this failure path or pretend unstarted sources reached EOF. */
        if (worker->create_result) return worker->create_result;
    }
    return 0;
}
static int observe_raw_reader_join_body(sl_linux_reader_join *observation) {
    sl_linux_capture_worker *worker;
    if (!observation || observation->attempted || !observation->worker_original) return -1;
    observation->attempted=1;
    worker=observation->worker_original;
    if (!worker->create_returned || worker->create_result || worker->joined_by) return -1;
    /* A separate preowned observation for each call, including EBUSY. Never
     * block controller servicing and never infer native terminal from the
     * worker body's release publication. No retry overwrites this result. */
    observation->result=pthread_tryjoin_np(worker->thread,&observation->joined_result);
    observation->returned=1;
    if (!observation->result) worker->joined_by=observation;
    /* Actual join output, including mismatch, is retained. No join result is
     * custody ACK, family closure, native disposition or FINAL acceptance. */
    if (observation->result) return observation->result;
    if (observation->joined_result!=worker || !atomic_load_explicit(&worker->returned,memory_order_acquire)) return -1;
    if(worker->terminal_wake){long result=sl_linux_schedule_joined_notification(worker->terminal_wake,worker);if(result)return result;}
    for(unsigned long i=0;i<atomic_load_explicit(&worker->completed_count,memory_order_acquire);i++){
        sl_linux_capture_frame *frame=&worker->prepared_frames[i];
        if(frame->schedule_wake){long result=sl_linux_schedule_joined_notification(frame->schedule_wake,frame);if(result)return result;}
    }
    return 0;
}
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *o){
 if(!o||o->attempted)return -1;
 long result=observe_raw_reader_join_body(o);o->interpretation_result=result;o->interpretation_returned=1;return result;
}
#endif
