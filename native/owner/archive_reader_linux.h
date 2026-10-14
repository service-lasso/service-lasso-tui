#ifndef SERVICE_LASSO_ARCHIVE_READER_LINUX_H
#define SERVICE_LASSO_ARCHIVE_READER_LINUX_H
#include "archive_files_linux.h"
#include "final_capture_linux.h"
#include "schedule_linux.h"
#include "shared_range_archive_linux.h"
#include <pthread.h>
struct sl_linux_public_startup;
typedef struct sl_linux_archive_read_view {
 struct sl_linux_public_startup *startup_original,*actual_startup;
 sl_linux_archive_file *file_original;
 sl_linux_archive_directory *directory_original;
 struct open_how how;
 sl_linux_call open,filesystem_query,identity_query;
 sl_linux_fd_lift lift;
 struct statfs filesystem;struct statx identity;
 int raw_fd,fd;long result;unsigned attempted,returned;
} sl_linux_archive_read_view;
/* Distinct actual readonly view, not a seal or independent ACK. Required view
 * remains held through later exact native-reader/ACK and disposition cuts. */
long sl_linux_acquire_archive_read_view(sl_linux_archive_read_view *root);
struct sl_linux_archive_reader_join;
typedef struct sl_linux_archive_readback {
 sl_linux_shared_range_archive_roster shared_ranges;
 struct sl_linux_public_controller *controller_original;
 sl_linux_archive_read_view *view_original,*actual_view;
 sl_linux_final_capture *capture_original,*actual_capture;
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity;
 sl_linux_call *reads,*actual_reads;unsigned long read_capacity,actual_read_capacity,read_count,extent;
 sl_linux_final_record *records_original;unsigned long record_count,expected_extent;
 sl_linux_schedule_wake *terminal_wake;
 pthread_t thread;int create_result;unsigned create_entered,create_returned;
 _Atomic unsigned body_entered,body_returned;
 struct sl_linux_archive_reader_join *joined_by;
 sl_linux_call before,after;struct statx before_state,after_state;
 unsigned char eof_byte;long primary_result,after_result,result;
 unsigned prepare_attempted,prepared,start_attempted,readback_equal;
} sl_linux_archive_readback;
typedef struct sl_linux_archive_reader_join {
 sl_linux_archive_readback *reader_original;
 sl_linux_schedule_wait *wait_original;
 void *output;int native_result;long interpretation_result;
 unsigned attempted,returned,interpretation_returned;
} sl_linux_archive_reader_join;
long sl_linux_prepare_archive_readback(sl_linux_archive_readback *root);
long sl_linux_start_archive_readback(sl_linux_archive_readback *root);
long sl_linux_join_archive_readback(sl_linux_archive_reader_join *root);
/* Separate native readback observation only. Dispatch requires the owning
 * controller's eventual positively established seal/last-use cut; these APIs
 * do not establish that cut or authorize FINAL acknowledgment. */
#endif
