#ifndef SERVICE_LASSO_CAPABILITY_DOMAIN_LINUX_H
#define SERVICE_LASSO_CAPABILITY_DOMAIN_LINUX_H
#include "owner_linux.h"
typedef struct sl_linux_capability_domain {
 sl_linux_entry *entry_original;
 sl_linux_call open,filesystem_query,reads[4];
 sl_linux_fd_lift lift;struct statfs filesystem;
 unsigned char bytes[4];long raw_counts[4];
 int fd;unsigned count,eof,last_cap,attempted,returned;long result;
} sl_linux_capability_domain;
/* Source v3 capability bit domain observation only. Actual kernel/source and
 * parent profile association remain required before any child transitions. */
long sl_linux_observe_capability_domain(sl_linux_capability_domain *domain);
#endif