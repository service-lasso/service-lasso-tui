#ifndef SERVICE_LASSO_MOUNT_LOCATION_LINUX_H
#define SERVICE_LASSO_MOUNT_LOCATION_LINUX_H
#include "local_mounts_linux.h"
#include <linux/openat2.h>
typedef struct sl_linux_mount_location {
 sl_linux_local_mounts *mounts_original;unsigned long row_index;
 sl_linux_local_mounts *actual_mounts;sl_linux_mount_row *actual_row;
 sl_linux_local_membership *actual_membership;
 char *root,*mountpoint,*relative,*actual_root,*actual_mountpoint,*actual_relative;
 unsigned long capacity,actual_capacity,root_length,mountpoint_length,relative_length;
 struct open_how mount_how,member_how;
 sl_linux_call mount_open,member_open,mount_filesystem,member_filesystem,mount_identity,member_identity;
 sl_linux_fd_lift mount_lift,member_lift;
 struct statfs mount_fs,member_fs;struct statx mount_state,member_state;
 int mount_fd,member_fd;long result;
 unsigned attempted,returned,outside_mount;
} sl_linux_mount_location;
/* Observed current-membership view, never an admitted payload target. */
long sl_linux_acquire_mount_location(sl_linux_mount_location *location);
#endif