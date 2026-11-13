#ifndef SERVICE_LASSO_FAILED_PARENT_DIRECTORY_LINUX_H
#define SERVICE_LASSO_FAILED_PARENT_DIRECTORY_LINUX_H
#include "archive_directory_linux.h"
#include "deadline_fd_lift_linux.h"
#include "outer_birth_linux.h"
typedef struct sl_linux_failed_parent_directory {
 const sl_candidate_profile *candidate_original;size_t declaration_index;
 const sl_linux_outer_birth *birth_original;
 sl_linux_deadline_sequence *sequence_original;
 unsigned char *path_original;size_t path_capacity,path_extent;
 sl_linux_archive_declaration_roster declarations;
 struct open_how how;sl_linux_call open,filesystem_query,identity_query;
 sl_linux_deadline_boundary open_boundary,filesystem_boundary,identity_boundary;
 sl_linux_deadline_fd_lift lift;
 struct statfs filesystem;struct statx identity;
 long open_raw,filesystem_raw,identity_raw,boundary_result,lift_result,result;
 int raw_fd,fd;unsigned attempted,returned;
} sl_linux_failed_parent_directory;
/* Native acquisition observations only; actual parent policy/caller admission
 * must dominate invocation in the complete production entry. */
long sl_linux_acquire_failed_parent_directory(sl_linux_failed_parent_directory *output,
 const sl_candidate_profile *candidate,const sl_linux_outer_birth *birth,
 sl_linux_deadline_sequence *sequence,unsigned char *path,size_t path_capacity,
 sl_linux_archive_declaration *declarations,size_t declaration_capacity);
int sl_linux_failed_parent_directory_matches_original(const sl_linux_failed_parent_directory *original);
#endif
