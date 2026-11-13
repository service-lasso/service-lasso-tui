#ifndef SERVICE_LASSO_ARCHIVE_DIRECTORY_LINUX_H
#define SERVICE_LASSO_ARCHIVE_DIRECTORY_LINUX_H
#include "candidate_profile.h"
#include "owner_linux.h"
#include "selinux_status_linux.h"
#include <linux/openat2.h>
#include <linux/stat.h>
struct sl_linux_public_startup;
typedef struct sl_linux_archive_declaration {
 const sl_candidate_record *declaration_original,*evidence_original;
 sl_candidate_span statement_original,path_original;uint64_t tuple[4];
 uint64_t evidence_index,statement_offset,statement_extent;
 unsigned attempted,returned;long result;
} sl_linux_archive_declaration;
typedef struct sl_linux_archive_directory {
 struct sl_linux_public_startup *startup_original,*actual_startup;
 sl_candidate_profile *candidate_original;
 sl_linux_archive_declaration *declarations,*actual_declarations;
 unsigned long declaration_capacity,actual_declaration_capacity,declaration_count,examined;
 unsigned char *path,*actual_path;unsigned long path_capacity,actual_path_capacity,path_extent;
 sl_linux_selinux_status_sample *statuses,*actual_statuses;unsigned long status_capacity,actual_status_capacity,status_count;
 sl_linux_call open,filesystem_query,identity_query;sl_linux_fd_lift lift;
 struct open_how how;struct statfs filesystem;struct statx identity;
 int raw_fd,fd;long primary_result,status_result,result;
 sl_linux_selinux_status_sample *status_failure_original;
 unsigned attempted,returned;
} sl_linux_archive_directory;
long sl_linux_acquire_archive_directory(sl_linux_archive_directory *root);
/* Pure per-record representation, shared by separate owning acquisition paths. */
long sl_linux_interpret_archive_declaration(sl_linux_archive_declaration *output,
 const sl_candidate_profile *candidate,size_t declaration_index);
int sl_linux_archive_candidate_excludes_output(const sl_candidate_profile *candidate,
 const void *output,size_t extent);
typedef struct sl_linux_archive_declaration_roster {
 const sl_candidate_profile *candidate_original;
 sl_linux_archive_declaration *rows_original;
 size_t capacity,count,examined,first_index;
 long result;unsigned attempted,returned;
} sl_linux_archive_declaration_roster;
long sl_linux_interpret_archive_declaration_roster(sl_linux_archive_declaration_roster *output,
 const sl_candidate_profile *candidate,sl_linux_archive_declaration *rows,size_t capacity);
int sl_linux_archive_declaration_matches_original(const sl_linux_archive_declaration *original,
 const sl_candidate_profile *candidate,size_t declaration_index);
int sl_linux_archive_declaration_roster_matches_original(const sl_linux_archive_declaration_roster *original);
#endif
