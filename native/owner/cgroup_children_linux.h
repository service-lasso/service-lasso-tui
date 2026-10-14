#ifndef SERVICE_LASSO_CGROUP_CHILDREN_LINUX_H
#define SERVICE_LASSO_CGROUP_CHILDREN_LINUX_H
#include "mount_location_linux.h"
typedef struct sl_linux_child_frame {
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity,length;
 sl_linux_call read;unsigned attempted,returned;
} sl_linux_child_frame;
typedef struct sl_linux_child_view {
 sl_linux_child_frame *frame_original;unsigned long offset,name_offset,name_length;
 unsigned char raw_type;unsigned dot,not_directory;
 struct open_how how;sl_linux_call open,filesystem_query,identity_query,close;
 sl_linux_fd_lift lift;struct statfs filesystem;struct statx identity;
 int fd;long result;unsigned attempted,returned;
} sl_linux_child_view;
typedef struct sl_linux_cgroup_children {
 sl_linux_mount_location *location_original,*actual_location;
 sl_linux_child_frame *frames,*actual_frames;sl_linux_child_view *views,*actual_views;
 unsigned long frame_capacity,view_capacity,actual_frame_capacity,actual_view_capacity,frame_count,view_count,candidate_count;
 struct open_how how;sl_linux_call open,filesystem_query,identity_query,close;
 sl_linux_fd_lift lift;struct statfs filesystem;struct statx identity;
 int fd;long result;unsigned attempted,returned,eof;
} sl_linux_cgroup_children;
/* Acquires only direct views of actual held membership. No target selection,
 * policy/profile authority or birth. Actual final borrower/disposition must
 * drive independent once-close of retained views; not yet connected here. */
long sl_linux_acquire_cgroup_children(sl_linux_cgroup_children *owner);
#endif