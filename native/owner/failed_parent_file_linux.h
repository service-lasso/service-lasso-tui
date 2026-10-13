#ifndef SERVICE_LASSO_FAILED_PARENT_FILE_LINUX_H
#define SERVICE_LASSO_FAILED_PARENT_FILE_LINUX_H
#include "failed_parent_directory_linux.h"
#include "failed_archive_name_linux.h"
typedef struct sl_linux_failed_parent_file {
 const sl_linux_failed_parent_directory *directory_original;
 const sl_linux_failed_archive_name *name_original;
 const struct sl_linux_failed_parent_file *writer_original;
 sl_linux_deadline_sequence *sequence_original;
 struct open_how how;sl_linux_call open,filesystem_query,identity_query;
 sl_linux_deadline_boundary open_boundary,filesystem_boundary,identity_boundary;
 sl_linux_deadline_fd_lift lift;
 struct statfs filesystem;struct statx identity;
 long open_raw,filesystem_raw,identity_raw,boundary_result,lift_result,result;
 int raw_fd,fd;unsigned reader,attempted,returned;
} sl_linux_failed_parent_file;
/* Distinct source entrypoints; no runtime mode is public authority. Both remain
 * uninvoked until complete parent policy/entry/containing qualification. */
long sl_linux_create_failed_parent_writer(sl_linux_failed_parent_file *output,
 const sl_linux_failed_parent_directory *directory,const sl_linux_failed_archive_name *name,
 sl_linux_deadline_sequence *sequence);
long sl_linux_open_failed_parent_reader(sl_linux_failed_parent_file *output,
 const sl_linux_failed_parent_file *writer,sl_linux_deadline_sequence *sequence);
/* Prebirth acquisition correspondence only. This is not post-emission stability
 * or archive/containing authority: the original acquisition observes size zero. */
int sl_linux_failed_parent_file_matches_original(const sl_linux_failed_parent_file *output);
#endif
