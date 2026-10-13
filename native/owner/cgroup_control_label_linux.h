#ifndef SERVICE_LASSO_CGROUP_CONTROL_LABEL_LINUX_H
#define SERVICE_LASSO_CGROUP_CONTROL_LABEL_LINUX_H
#include "cgroup_children_linux.h"
enum sl_linux_cgroup_control_kind {SL_CGROUP_PROCS=1,SL_CGROUP_THREADS=2};
typedef struct sl_linux_cgroup_control_label {
 sl_linux_cgroup_children *children_original,*actual_children;
 sl_linux_child_view *view_original,*actual_view;
 enum sl_linux_cgroup_control_kind kind,actual_kind;
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity,raw_extent,extent,examined;
 char name[15],attribute[17];struct open_how how;
 sl_linux_call open,filesystem_query,identity_query,label_query,close_raw,close_held;
 sl_linux_call *close_primary,*close_unsettled;unsigned closes_attempted,closes_returned,close_raw_attempted,close_held_attempted,close_unmapped;long close_result;
 sl_linux_fd_lift lift;struct statfs filesystem;struct statx identity;
 int raw_fd,fd;long raw_label_count,result;unsigned attempted,returned,framing_complete;
} sl_linux_cgroup_control_label;
/* Actual existing child view only; no control write, target selection or close.
 * Partial raw/lifted descriptors and bytes remain in this owner. */
long sl_linux_acquire_cgroup_control_label(sl_linux_cgroup_control_label *root);
#endif