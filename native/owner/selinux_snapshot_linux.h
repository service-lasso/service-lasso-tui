#ifndef SERVICE_LASSO_SELINUX_SNAPSHOT_LINUX_H
#define SERVICE_LASSO_SELINUX_SNAPSHOT_LINUX_H
#include "local_mounts_linux.h"
#include "selinux_status_linux.h"
#include <linux/openat2.h>
typedef struct sl_linux_policy_frame {
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity,length;
 sl_linux_call read;unsigned attempted,returned;
} sl_linux_policy_frame;
typedef struct sl_linux_selinux_snapshot {
 sl_linux_local_mounts *mounts_original,*actual_mounts;unsigned long row_index;sl_linux_mount_row *actual_row;
 char *path,*actual_path;unsigned long path_capacity,actual_path_capacity,path_length;
 unsigned char *body,*actual_body;unsigned long body_capacity,actual_body_capacity,length;
 sl_linux_policy_frame *frames,*actual_frames;unsigned long frame_capacity,actual_frame_capacity,frame_count;
 struct open_how root_how,policy_how;
 sl_linux_call root_open,root_filesystem,root_identity,root_close,policy_open,policy_filesystem,policy_identity,policy_close;
 sl_linux_fd_lift root_lift,policy_lift;struct statfs root_fs,policy_fs;struct statx root_state,policy_state;
 sl_linux_selinux_status status;sl_linux_selinux_status_sample before_policy,after_policy;
 int root_fd,policy_fd;long result;unsigned attempted,returned,eof;
} sl_linux_selinux_snapshot;
/* Exact byte acquisition only; no loaded-policy/epoch/enforcement/profile
 * authentication. SAME held views remain for required status/context/label
 * borrowers and eventual independent once-close, not pathname authority. */
long sl_linux_acquire_selinux_snapshot(sl_linux_selinux_snapshot *snapshot);
#endif