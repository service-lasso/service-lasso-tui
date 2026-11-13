#ifndef SERVICE_LASSO_ARCHIVE_FILES_LINUX_H
#define SERVICE_LASSO_ARCHIVE_FILES_LINUX_H
#include "archive_directory_linux.h"
#include "candidate_target_linux.h"
#include <time.h>
enum {SL_ARCHIVE_FILE_NAME_BYTES=(sizeof("service-lasso-tui.")-1)+64+1+10+1+19+1+9+(sizeof(".local")-1)+1};
typedef struct sl_linux_archive_file {
 sl_linux_archive_directory *directory_original;
 unsigned char name[SL_ARCHIVE_FILE_NAME_BYTES];unsigned long name_extent;
 struct open_how how;sl_linux_call open,filesystem_query,identity_query;sl_linux_fd_lift lift;
 struct statfs filesystem;struct statx identity;
 int raw_fd,fd;long result;unsigned attempted,returned;
} sl_linux_archive_file;
typedef struct sl_linux_archive_files {
 struct sl_linux_public_startup *startup_original,*actual_startup;
 sl_linux_archive_directory *directory_original;
 sl_candidate_span source_identity_original;
 sl_linux_call process_query,clock_query;long process_result,clock_result;struct timespec instant;
 sl_linux_selinux_status_sample *statuses,*actual_statuses,*status_failure_original;unsigned long status_capacity,actual_status_capacity,status_count;long primary_result,status_result;
 sl_linux_archive_file local,final;
 sl_linux_target_close_view closes[4],*close_failure_original;unsigned closes_attempted,closes_returned;long close_result;
 long result;unsigned attempted,returned;
} sl_linux_archive_files;
long sl_linux_create_archive_files(sl_linux_archive_files *root);
struct sl_linux_public_controller;
long sl_linux_close_failed_archive_before_birth(struct sl_linux_public_controller *controller);
#endif