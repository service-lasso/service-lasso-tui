/* UNRUN controlled source regression. Real pthread borrowers exercise the
 * preparation/dispatch boundary; substituted capture is not native EOF proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../forward_workers_linux.h"
#include <assert.h>
#include <string.h>
long sl_linux_capture_read(sl_linux_capture_frame *frame){
 frame->read_attempted=1;frame->read_returned=1;frame->raw_count=0;return 0;
}
long sl_linux_notify_forward_worker(sl_linux_forward_worker *worker,sl_linux_notification *observation){
 (void)worker;(void)observation;assert(0);return -1;
}
#include "../capture_workers_linux.c"
typedef struct borrower{sl_linux_capture_worker *reader;_Atomic unsigned entered;unsigned long observed;}borrower;
static void *observe_publication(void *argument){
 borrower *owner=argument;
 assert(atomic_load_explicit(&owner->reader->completed_count,memory_order_acquire)==0);
 assert(atomic_load_explicit(&owner->reader->returned,memory_order_acquire)==0);
 atomic_store_explicit(&owner->entered,1,memory_order_release);
 while(!atomic_load_explicit(&owner->reader->returned,memory_order_acquire))
  owner->observed=atomic_load_explicit(&owner->reader->completed_count,memory_order_acquire);
 owner->observed=atomic_load_explicit(&owner->reader->completed_count,memory_order_acquire);
 return owner;
}
int main(void){
 sl_linux_capture_worker reader={0};sl_linux_capture_frame frame={0};
 borrower owner={0};pthread_t observer;void *result=0;
 reader.controller_original=&owner;reader.frames=&frame;reader.capacity=1;
 frame.prepared=1;frame.input_fd=7;frame.writer_original=&frame;
 assert(sl_linux_prepare_raw_readers(&reader,1)==0);
 assert(reader.prepared&&reader.prepared_frames==&frame&&reader.prepared_capacity==1);
 owner.reader=&reader;atomic_init(&owner.entered,0);
 assert(pthread_create(&observer,0,observe_publication,&owner)==0);
 while(!atomic_load_explicit(&owner.entered,memory_order_acquire))atomic_signal_fence(memory_order_seq_cst);
 /* The observer is already a genuine borrower. Start must not atomic_init
  * or replace the publication backing while that borrower exists. */
 assert(sl_linux_start_raw_readers(&reader,1)==0);
 assert(pthread_join(observer,&result)==0&&result==&owner&&owner.observed==1);
 /* Observer completion does not establish reader native termination. */
 assert(pthread_join(reader.thread,&result)==0&&result==&reader);
 assert(reader.outcome==SL_READER_READ_EOF&&atomic_load(&reader.completed_count)==1);
 assert(sl_linux_prepare_raw_readers(&reader,1)==-1);
 assert(atomic_load(&reader.completed_count)==1&&atomic_load(&reader.returned)==1);
 assert(sl_linux_start_raw_readers(&reader,1)==-1);
 return 0;
}
#endif
