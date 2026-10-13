#ifndef SERVICE_LASSO_LOCAL_PAYLOAD_LINUX_H
#define SERVICE_LASSO_LOCAL_PAYLOAD_LINUX_H
#include "owner_linux.h"
#include <linux/openat2.h>
#include <linux/stat.h>
typedef struct sl_linux_installed_payload {
 sl_linux_entry *entry_original;
 const char *directory_locator;unsigned long directory_length,directory_capacity;
 const char *prepared_locator;
 struct open_how directory_how,payload_how;
 sl_linux_call open_directory,open_payload,directory_identity,payload_identity;
 struct statx directory_state,payload_state;
 sl_linux_fd_lift directory_lift,payload_lift;
 int directory_fd,payload_fd;
 unsigned attempted,returned;long result;
} sl_linux_installed_payload;
/* Existing bytes acquisition only. No mutation exclusion, published-member,
 * loaded-image/interpreter or native-family admission follows from result0. */
long sl_linux_acquire_installed_payload(sl_linux_installed_payload *acquisition);
#endif