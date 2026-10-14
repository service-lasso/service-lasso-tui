#ifndef SERVICE_LASSO_SELINUX_STATUS_LINUX_H
#define SERVICE_LASSO_SELINUX_STATUS_LINUX_H
#include "owner_linux.h"
#include <linux/openat2.h>
#include <stdint.h>
struct sl_linux_selinux_snapshot;
typedef struct sl_linux_selinux_status {
 struct sl_linux_selinux_snapshot *snapshot_original;
 sl_linux_call open,filesystem,identity,mapping;
 sl_linux_fd_lift lift;struct statfs fs;struct statx state;struct open_how how;
 char name[7];int fd;long raw_fd,raw_mapping,result;
 volatile uint32_t *actual_words;
 unsigned attempted,returned;
} sl_linux_selinux_status;
typedef struct sl_linux_selinux_status_sample {
 sl_linux_selinux_status *status_original,*actual_status;
 uint32_t sequence_before,sequence_after,version,enforcing,policyload,deny_unknown;
 int result;unsigned attempted,fields_read,returned;
} sl_linux_selinux_status_sample;
long sl_linux_acquire_selinux_status(sl_linux_selinux_status *owner);
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *sample);
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b);
#endif