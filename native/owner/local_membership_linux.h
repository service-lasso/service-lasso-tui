#ifndef SERVICE_LASSO_LOCAL_MEMBERSHIP_LINUX_H
#define SERVICE_LASSO_LOCAL_MEMBERSHIP_LINUX_H
#include "owner_linux.h"
typedef struct sl_linux_membership_frame {
 unsigned char *bytes;unsigned long capacity;
 sl_linux_call read;long raw_count;
} sl_linux_membership_frame;
typedef struct sl_linux_local_membership {
 sl_linux_entry *entry_original;
 sl_linux_call open,filesystem_query;struct statfs filesystem;sl_linux_fd_lift lift;int fd;
 sl_linux_membership_frame *frames,*prepared_frames;
 unsigned long capacity,prepared_capacity,count;
 char *path,*prepared_path;unsigned long path_capacity,prepared_path_capacity,path_length;
 unsigned attempted,returned,eof;long result;
} sl_linux_local_membership;
/* Current-process locator observation only, never native family admission. */
long sl_linux_observe_local_membership(sl_linux_local_membership *observation);
#endif