#ifndef SERVICE_LASSO_PIDFD_PROC_LINUX_H
#define SERVICE_LASSO_PIDFD_PROC_LINUX_H
#include "owner_linux.h"
struct sl_linux_namespace_view;
typedef struct sl_linux_pidfd_proc {
 sl_linux_birth *birth_original;struct sl_linux_namespace_view *procfs_original;
 sl_linux_call open,filesystem_query,close,*reads;
 sl_linux_fd_lift lift;struct statfs filesystem;
 unsigned char *bytes,*actual_bytes;sl_linux_call *actual_reads;
 unsigned long capacity,read_capacity,actual_capacity,actual_read_capacity,length,read_count;
 unsigned long pid_offset,pid_length;
 char fd_digits[11],locator[30];
 int fd,actual_pidfd;long raw_fd,raw_pid,result;
 unsigned attempted,returned,eof,pid_seen;
} sl_linux_pidfd_proc;
long sl_linux_observe_pidfd_proc(sl_linux_pidfd_proc *observation);
#endif