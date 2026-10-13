#if defined(__linux__) && defined(__x86_64__)
#include "local_cgroup_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <sys/stat.h>
static int locator_valid(const char *bytes,unsigned long length,unsigned long capacity,int relative){
 unsigned long index,start=0;
 if(!bytes||!length||capacity<=length||bytes[length]!=0)return 0;
 if(relative&&bytes[0]=='/')return 0;
 if(!relative&&bytes[0]!='/')return 0;
 for(index=0;index<length;index++){
  if(!bytes[index])return 0;
  if(relative&&(bytes[index]=='/'||index+1==length)){
   unsigned long end=bytes[index]=='/'?index:index+1,size=end-start;
   if(!size||(size==1&&bytes[start]=='.')||(size==2&&bytes[start]=='.'&&bytes[start+1]=='.'))return 0;
   start=index+1;
  }
 }
 return !relative||bytes[length-1]!='/';
}
static void initialize(sl_linux_call *call){atomic_init(&call->entered,0);atomic_init(&call->returned,0);}
long sl_linux_acquire_existing_cgroup(sl_linux_existing_cgroup *acquisition){
 long result;unsigned mask=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID;
 if(!acquisition||acquisition->attempted)return -1;
 acquisition->attempted=1;acquisition->root_fd=-1;acquisition->target_fd=-1;acquisition->processes_fd=-1;acquisition->result=-1;
 if(!acquisition->entry_original||!acquisition->entry_original->public_streams_prepared||
    !locator_valid(acquisition->root_locator,acquisition->root_length,acquisition->root_capacity,0)||
    !locator_valid(acquisition->relative_locator,acquisition->relative_length,acquisition->relative_capacity,1))goto done;
 acquisition->prepared_root_locator=acquisition->root_locator;acquisition->prepared_relative_locator=acquisition->relative_locator;
 initialize(&acquisition->open_root);initialize(&acquisition->open_target);initialize(&acquisition->open_processes);
 initialize(&acquisition->root_filesystem);initialize(&acquisition->target_filesystem);
 initialize(&acquisition->root_identity);initialize(&acquisition->target_identity);
 initialize(&acquisition->credentials_uid);initialize(&acquisition->credentials_gid);initialize(&acquisition->capabilities);
 result=sl_linux_call6(&acquisition->credentials_uid,"local.cgroup.getresuid",SYS_getresuid,
   (long)&acquisition->real_uid,(long)&acquisition->effective_uid,(long)&acquisition->saved_uid,0,0,0);
 acquisition->result=result;if(result)goto done;
 result=sl_linux_call6(&acquisition->credentials_gid,"local.cgroup.getresgid",SYS_getresgid,
   (long)&acquisition->real_gid,(long)&acquisition->effective_gid,(long)&acquisition->saved_gid,0,0,0);
 acquisition->result=result;if(result)goto done;
 acquisition->capability_header.version=_LINUX_CAPABILITY_VERSION_3;acquisition->capability_header.pid=0;
 result=sl_linux_call6(&acquisition->capabilities,"local.cgroup.capget",SYS_capget,
   (long)&acquisition->capability_header,(long)acquisition->capability_data,0,0,0,0);
 acquisition->result=result;if(result)goto done;
 acquisition->root_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC,.resolve=RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS};
 result=sl_linux_call6(&acquisition->open_root,"local.cgroup.existing-root.open",SYS_openat2,
   AT_FDCWD,(long)acquisition->prepared_root_locator,(long)&acquisition->root_how,sizeof(acquisition->root_how),0,0);
 acquisition->result=result;if(result<0||result>2147483647L){if(result>2147483647L)acquisition->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&acquisition->root_lift,(int)result);acquisition->result=result;if(result<0)goto done;
 acquisition->root_fd=(int)result;
 result=sl_linux_call6(&acquisition->root_filesystem,"local.cgroup.root.fstatfs",SYS_fstatfs,
   acquisition->root_fd,(long)&acquisition->root_fs,0,0,0,0);
 acquisition->result=result;if(result||acquisition->root_fs.f_type!=CGROUP2_SUPER_MAGIC){acquisition->result=result?result:-1;goto done;}
 result=sl_linux_call6(&acquisition->root_identity,"local.cgroup.root.statx",SYS_statx,
   acquisition->root_fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&acquisition->root_state,0);
 acquisition->result=result;if(result||(acquisition->root_state.stx_mask&mask)!=mask||!S_ISDIR(acquisition->root_state.stx_mode)){acquisition->result=result?result:-1;goto done;}
 acquisition->target_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC,
   .resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};
 result=sl_linux_call6(&acquisition->open_target,"local.cgroup.existing-target.open",SYS_openat2,
   acquisition->root_fd,(long)acquisition->prepared_relative_locator,(long)&acquisition->target_how,sizeof(acquisition->target_how),0,0);
 acquisition->result=result;if(result<0||result>2147483647L){if(result>2147483647L)acquisition->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&acquisition->target_lift,(int)result);acquisition->result=result;if(result<0)goto done;
 acquisition->target_fd=(int)result;
 result=sl_linux_call6(&acquisition->target_filesystem,"local.cgroup.target.fstatfs",SYS_fstatfs,
   acquisition->target_fd,(long)&acquisition->target_fs,0,0,0,0);
 acquisition->result=result;if(result||acquisition->target_fs.f_type!=CGROUP2_SUPER_MAGIC){acquisition->result=result?result:-1;goto done;}
 result=sl_linux_call6(&acquisition->target_identity,"local.cgroup.target.statx",SYS_statx,
   acquisition->target_fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&acquisition->target_state,0);
 acquisition->result=result;if(result||(acquisition->target_state.stx_mask&mask)!=mask||!S_ISDIR(acquisition->target_state.stx_mode)||
   acquisition->target_state.stx_mnt_id!=acquisition->root_state.stx_mnt_id){acquisition->result=result?result:-1;goto done;}
 result=sl_linux_call6(&acquisition->open_processes,"local.cgroup.procs.write-view.open",SYS_openat,
   acquisition->target_fd,(long)"cgroup.procs",O_WRONLY|O_NOFOLLOW|O_CLOEXEC,0,0,0);
 acquisition->result=result;if(result<0||result>2147483647L){if(result>2147483647L)acquisition->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&acquisition->processes_lift,(int)result);acquisition->result=result;if(result<0)goto done;
 acquisition->processes_fd=(int)result;acquisition->result=0;
done:
 acquisition->returned=1;return acquisition->result;
}
#endif
