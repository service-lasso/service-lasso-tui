#ifndef SERVICE_LASSO_LOCAL_CGROUP_LINUX_H
#define SERVICE_LASSO_LOCAL_CGROUP_LINUX_H
#include "owner_linux.h"
#include <linux/openat2.h>
#include <linux/stat.h>
#include <linux/capability.h>
typedef struct sl_linux_existing_cgroup {
 sl_linux_entry *entry_original;
 const char *root_locator,*relative_locator;
 unsigned long root_length,root_capacity,relative_length,relative_capacity;
 const char *prepared_root_locator,*prepared_relative_locator;
 struct open_how root_how,target_how;
 sl_linux_call open_root,open_target,root_filesystem,target_filesystem,root_identity,target_identity,open_processes;
 sl_linux_call credentials_uid,credentials_gid,capabilities;
 struct statfs root_fs,target_fs;struct statx root_state,target_state;
 unsigned real_uid,effective_uid,saved_uid,real_gid,effective_gid,saved_gid;
 struct __user_cap_header_struct capability_header;struct __user_cap_data_struct capability_data[2];
 sl_linux_fd_lift root_lift,target_lift,processes_lift;
 int root_fd,target_fd,processes_fd;long result;
 unsigned attempted,returned;
} sl_linux_existing_cgroup;
/* Acquires an existing object and observes current rights/identity. This is
 * NOT native birth admission: namespace/delegation/protection/migration
 * exclusion must separately be genuinely established before clone3. */
long sl_linux_acquire_existing_cgroup(sl_linux_existing_cgroup *acquisition);
#endif
