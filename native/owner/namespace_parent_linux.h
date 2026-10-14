#ifndef SERVICE_LASSO_NAMESPACE_PARENT_LINUX_H
#define SERVICE_LASSO_NAMESPACE_PARENT_LINUX_H
#include "owner_linux.h"
#include "pidfd_proc_linux.h"
typedef struct sl_linux_namespace_view {
 sl_linux_call acquire,filesystem_query,identity_query,type_query,close_held,close_raw;
 unsigned close_held_attempted,close_raw_attempted;
 sl_linux_fd_lift lift;struct statfs filesystem;struct statx identity;
 int fd;long raw_fd,type;
} sl_linux_namespace_view;
typedef struct sl_linux_parent_namespace {
 sl_linux_birth *birth_original;
 sl_linux_authority_inspection *transition_original;
 sl_linux_root_observation root_before,root_after;
 sl_linux_namespace_view views[6];
 sl_linux_pidfd_proc pidfd_info;
 char pid_digits[11];const char *pid_locator;
 unsigned attempted,returned,closes_attempted,closes_returned;long result,close_result;
 sl_linux_call *close_primary,*close_unsettled_original;sl_linux_namespace_view *close_unmapped_original;
} sl_linux_parent_namespace;
/* Actual held owning-userns observation only. Partial views remain outside
 * family; their final once-close/disposition is a separate native obligation. */
long sl_linux_observe_parent_namespace(sl_linux_parent_namespace *observation);
/* Raw once-close traversal only. Caller must establish the actual selected
 * last-borrower/disposition cut; acquisition return does not establish it.
 * No current startup/failure path invokes this operation. */
long sl_linux_close_namespace_views(sl_linux_parent_namespace *observation);
#endif