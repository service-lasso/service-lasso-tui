#ifndef SERVICE_LASSO_LOCAL_INSTALLATION_LINUX_H
#define SERVICE_LASSO_LOCAL_INSTALLATION_LINUX_H
#include "local_payload_linux.h"
typedef struct sl_linux_local_installation {
 sl_linux_entry *entry_original;
 char *executable_path,*directory_path;unsigned long executable_capacity,directory_capacity;
 char *prepared_executable_path,*prepared_directory_path;
 unsigned long prepared_executable_capacity,prepared_directory_capacity,directory_length;
 sl_linux_call open_proc,proc_filesystem,open_executable,executable_identity,readlink,open_public,public_identity;
 struct statfs proc_fs;struct statx executable_state,public_state;
 sl_linux_fd_lift proc_lift,executable_lift,public_lift;
 int proc_fd,executable_fd,public_fd;long raw_link_count;
 sl_linux_installed_payload payload;
 unsigned attempted,returned;long result;
} sl_linux_local_installation;
long sl_linux_acquire_local_installation(sl_linux_local_installation *installation);
#endif