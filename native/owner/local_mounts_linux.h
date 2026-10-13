#ifndef SERVICE_LASSO_LOCAL_MOUNTS_LINUX_H
#define SERVICE_LASSO_LOCAL_MOUNTS_LINUX_H
#include "local_membership_linux.h"
typedef struct sl_linux_mount_field {unsigned long offset,length;} sl_linux_mount_field;
typedef struct sl_linux_mount_row {
 sl_linux_mount_field line,id,parent,device,root,mountpoint,options,optional,filesystem,source,super_options;
 unsigned long mount_id,parent_id;
} sl_linux_mount_row;
typedef struct sl_linux_local_mounts {
 sl_linux_local_membership *membership_original,*actual_membership;
 sl_linux_call open,filesystem_query,close,*reads,*actual_reads;
 sl_linux_fd_lift lift;struct statfs filesystem;
 unsigned char *bytes,*actual_bytes;
 sl_linux_mount_row *rows,*actual_rows,*selinux_rows,*actual_selinux_rows;
 unsigned long selinux_capacity,actual_selinux_capacity,selinux_count;
 unsigned long capacity,read_capacity,row_capacity,actual_capacity,actual_read_capacity,actual_row_capacity;
 unsigned long length,read_count,row_count;
 int fd;long raw_fd,result;
 unsigned attempted,returned,eof;
} sl_linux_local_mounts;
/* Raw locator roster only. No selected mount/target/profile or gate grant. */
long sl_linux_observe_local_mounts(sl_linux_local_mounts *observation);
#endif