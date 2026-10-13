#ifndef SERVICE_LASSO_LSM_SELF_LINUX_H
#define SERVICE_LASSO_LSM_SELF_LINUX_H
#include "owner_linux.h"
#include <stddef.h>
/* Explicit Linux native x86_64 UAPI recipe; not x32 or another architecture. */
typedef struct sl_linux_lsm_header {uint64_t id,flags,len,ctx_len;} sl_linux_lsm_header;
typedef struct sl_linux_lsm_self {
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity;
 sl_linux_call before,query,after;
 sl_linux_call *initialized_calls[3];
 uint32_t supplied_size,reported_size;
 uint64_t requested_id,requested_flags,requested_attr;
 sl_linux_lsm_header observed;
 const unsigned char *context;unsigned long extent,examined,padding_examined;
 long raw_before,raw_query,raw_after,result;
 unsigned attempted,returned,framing_complete;
} sl_linux_lsm_self;
/* Caller supplies aligned distinct fresh source-owned backing before entry.
 * Result0 observes ONLY actual current thread context, not exec/profile rights. */
int sl_linux_prepare_selinux_self(sl_linux_lsm_self *root);
long sl_linux_observe_selinux_self(sl_linux_lsm_self *root);
/* Read-only explicit pending EXEC; empty context is a genuine distinct result. */
long sl_linux_observe_selinux_exec(sl_linux_lsm_self *root);
long sl_linux_observe_selinux_fscreate(sl_linux_lsm_self *root);
#endif