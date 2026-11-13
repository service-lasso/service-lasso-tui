#if defined(__linux__) && defined(__x86_64__)
#include "local_installation_linux.h"
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <stdint.h>
static void installation_initialize(sl_linux_call *call){atomic_init(&call->entered,0);atomic_init(&call->returned,0);}
static int installation_overlaps(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
static long installation_lift_result(sl_linux_fd_lift *lift,long result){
 if(result<0)return result;if(result>2147483647L)return -1;
 return sl_linux_lift_internal_fd(lift,(int)result);
}
long sl_linux_acquire_local_installation(sl_linux_local_installation *installation){
 long result;unsigned long i,last_slash=0;
 unsigned mask=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 if(!installation||installation->attempted)return -1;
 installation->attempted=1;installation->result=-1;installation->proc_fd=-1;installation->executable_fd=-1;installation->public_fd=-1;
 if(!installation->entry_original||!installation->entry_original->public_streams_prepared||
    !installation->executable_path||!installation->directory_path||installation->executable_capacity<2||
    installation->executable_capacity>2147483647UL||installation->directory_capacity<2||
    installation_overlaps(installation->executable_path,installation->executable_capacity,installation->directory_path,installation->directory_capacity)||
    installation_overlaps(installation->executable_path,installation->executable_capacity,installation,sizeof(*installation))||
    installation_overlaps(installation->directory_path,installation->directory_capacity,installation,sizeof(*installation)))goto done;
 installation->prepared_executable_path=installation->executable_path;installation->prepared_directory_path=installation->directory_path;
 installation->prepared_executable_capacity=installation->executable_capacity;installation->prepared_directory_capacity=installation->directory_capacity;
 installation_initialize(&installation->open_proc);installation_initialize(&installation->proc_filesystem);installation_initialize(&installation->open_executable);
 installation_initialize(&installation->executable_identity);installation_initialize(&installation->readlink);installation_initialize(&installation->open_public);installation_initialize(&installation->public_identity);
 result=sl_linux_call6(&installation->open_proc,"local.installation.proc-self.open",SYS_openat,AT_FDCWD,(long)"/proc/self",O_RDONLY|O_DIRECTORY|O_CLOEXEC,0,0,0);
 result=installation_lift_result(&installation->proc_lift,result);installation->result=result;if(result<0)goto done;installation->proc_fd=(int)result;
 result=sl_linux_call6(&installation->proc_filesystem,"local.installation.proc-self.fstatfs",SYS_fstatfs,installation->proc_fd,(long)&installation->proc_fs,0,0,0,0);
 installation->result=result;if(result||installation->proc_fs.f_type!=PROC_SUPER_MAGIC){installation->result=result?result:-1;goto done;}
 result=sl_linux_call6(&installation->open_executable,"local.installation.actual-exe.open",SYS_openat,installation->proc_fd,(long)"exe",O_RDONLY|O_NONBLOCK|O_CLOEXEC,0,0,0);
 result=installation_lift_result(&installation->executable_lift,result);installation->result=result;if(result<0)goto done;installation->executable_fd=(int)result;
 result=sl_linux_call6(&installation->executable_identity,"local.installation.actual-exe.statx",SYS_statx,installation->executable_fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&installation->executable_state,0);
 installation->result=result;if(result||(installation->executable_state.stx_mask&mask)!=mask||!S_ISREG(installation->executable_state.stx_mode)){
    installation->result=result?result:-1;goto done;}
 result=sl_linux_call6(&installation->readlink,"local.installation.actual-exe.readlink",SYS_readlinkat,installation->proc_fd,(long)"exe",(long)installation->prepared_executable_path,installation->prepared_executable_capacity,0,0);
 installation->raw_link_count=result;installation->result=result;
 if(result<=0||result>=(long)installation->prepared_executable_capacity){if(result>=0)installation->result=-1;goto done;}
 if(installation->prepared_executable_path[0]!='/'){installation->result=-1;goto done;}
 for(i=0;i<(unsigned long)result;i++){
  if(!installation->prepared_executable_path[i]){installation->result=-1;goto done;}
  if(installation->prepared_executable_path[i]=='/')last_slash=i;
 }
 installation->directory_length=last_slash?last_slash:1;
 if(installation->directory_length>=installation->prepared_directory_capacity){installation->result=-1;goto done;}
 for(i=0;i<installation->directory_length;i++)installation->prepared_directory_path[i]=installation->prepared_executable_path[i];
 installation->prepared_directory_path[installation->directory_length]=0;
 installation->payload.entry_original=installation->entry_original;
 installation->payload.directory_locator=installation->prepared_directory_path;
 installation->payload.directory_length=installation->directory_length;installation->payload.directory_capacity=installation->prepared_directory_capacity;
 result=sl_linux_acquire_installed_payload(&installation->payload);installation->result=result;if(result)goto done;
 /* Public member is reopened relative to SAME held installation directory.
  * Identity correspondence is necessary but not loaded/published byte proof. */
 result=sl_linux_call6(&installation->open_public,"local.installation.public-member.open",SYS_openat2,
   installation->payload.directory_fd,(long)"service-lasso-tui",(long)&installation->payload.payload_how,sizeof(installation->payload.payload_how),0,0);
 result=installation_lift_result(&installation->public_lift,result);installation->result=result;if(result<0)goto done;installation->public_fd=(int)result;
 result=sl_linux_call6(&installation->public_identity,"local.installation.public-member.statx",SYS_statx,installation->public_fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&installation->public_state,0);
 installation->result=result;
 if(result||(installation->public_state.stx_mask&mask)!=mask||!S_ISREG(installation->public_state.stx_mode)||
   installation->public_state.stx_ino!=installation->executable_state.stx_ino||
   installation->public_state.stx_dev_major!=installation->executable_state.stx_dev_major||
   installation->public_state.stx_dev_minor!=installation->executable_state.stx_dev_minor||
   installation->public_state.stx_mnt_id!=installation->executable_state.stx_mnt_id){installation->result=result?result:-1;goto done;}
 installation->result=0;
done:
 installation->returned=1;return installation->result;
}
#endif