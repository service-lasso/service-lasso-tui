#ifndef SERVICE_LASSO_CAPTURE_WORKERS_LINUX_H
#define SERVICE_LASSO_CAPTURE_WORKERS_LINUX_H
#include "capture_linux.h"
#include <pthread.h>
enum sl_linux_reader_outcome {
    SL_READER_NOT_RETURNED, SL_READER_READ_EOF, SL_READER_READ_FAILURE,
    SL_READER_BACKING_EXHAUSTED, SL_READER_INVALID_EXTENT
};
struct sl_linux_reader_join;
struct sl_linux_forward_worker;
typedef struct sl_linux_capture_worker {
    void *controller_original;
    struct sl_linux_schedule_wake *terminal_wake;
    struct sl_linux_forward_worker *forward_original;
    sl_linux_capture_frame *frames;
    unsigned long capacity,read_count;
    sl_linux_capture_frame *prepared_frames;
    unsigned long prepared_capacity;
    pthread_t thread;
    int create_result;
    void *worker_result;
    struct sl_linux_reader_join *joined_by;
    enum sl_linux_reader_outcome outcome;
    _Atomic unsigned entered,returned;
    _Atomic unsigned long completed_count;
    unsigned prepared,start_attempted,create_returned;
} sl_linux_capture_worker;
typedef struct sl_linux_reader_join {
    sl_linux_capture_worker *worker_original;
    void *joined_result;
    int result;
    long interpretation_result;unsigned interpretation_returned;
    unsigned attempted,returned;
} sl_linux_reader_join;
int sl_linux_start_raw_readers(sl_linux_capture_worker *workers,unsigned long count);
int sl_linux_prepare_raw_readers(sl_linux_capture_worker *workers,unsigned long count);
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *observation);
#endif
