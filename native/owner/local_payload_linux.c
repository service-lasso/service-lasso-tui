#if defined(__linux__) && defined(__x86_64__)
#include "local_payload_linux.h"
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
static void initialize(sl_linux_call *call){atomic_init(&call->entered,0);atomic_init(&call->returned,0);}
long sl_linux_acquire_installed_payload(sl_linux_installed_payload *acquisition){
 unsigned long index;long result;unsigned mask=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 if(!acquisition||acquisition->attempted)return -1;
 acquisition->attempted=1;acquisition->result=-1;acquisition->directory_fd=-1;acquisition->payload_fd=-1;
 if(!acquisition->entry_original||!acquisition->entry_original->public_streams_prepared||
    !acquisition->directory_locator||!acquisition->directory_length||
    acquisition->directory_capacity<=acquisition->directory_length||
    acquisition->directory_locator[0]!='/'||acquisition->directory_locator[acquisition->directory_length])goto done;
 for(index=0;index<acquisition->directory_length;index++)if(!acquisition->directory_locator[index])goto done;
 acquisition->prepared_locator=acquisition->directory_locator;
 initialize(&acquisition->open_directory);initialize(&acquisition->open_payload);
 initialize(&acquisition->directory_identity);initialize(&acquisition->payload_identity);
 acquisition->directory_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC,
    .resolve=RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS};
 result=sl_linux_call6(&acquisition->open_directory,"local.payload.directory.open",SYS_openat2,
    AT_FDCWD,(long)acquisition->prepared_locator,(long)&acquisition->directory_how,sizeof(acquisition->directory_how),0,0);
 acquisition->result=result;if(result<0||result>2147483647L){if(result>2147483647L)acquisition->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&acquisition->directory_lift,(int)result);acquisition->result=result;if(result<0)goto done;
 acquisition->directory_fd=(int)result;
 result=sl_linux_call6(&acquisition->directory_identity,"local.payload.directory.statx",SYS_statx,
    acquisition->directory_fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&acquisition->directory_state,0);
 acquisition->result=result;if(result||(acquisition->directory_state.stx_mask&mask)!=mask||!S_ISDIR(acquisition->directory_state.stx_mode)){
    acquisition->result=result?result:-1;goto done;}
 acquisition->payload_how=(struct open_how){.flags=O_RDONLY|O_NONBLOCK|O_CLOEXEC,
    .resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};
 result=sl_linux_call6(&acquisition->open_payload,"local.payload.private.open",SYS_openat2,
    acquisition->directory_fd,(long)".service-lasso-tui/service-lasso-tui-go",(long)&acquisition->payload_how,sizeof(acquisition->payload_how),0,0);
 acquisition->result=result;if(result<0||result>2147483647L){if(result>2147483647L)acquisition->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&acquisition->payload_lift,(int)result);acquisition->result=result;if(result<0)goto done;
 acquisition->payload_fd=(int)result;
 result=sl_linux_call6(&acquisition->payload_identity,"local.payload.private.statx",SYS_statx,
    acquisition->payload_fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&acquisition->payload_state,0);
 acquisition->result=result;
 if(result||(acquisition->payload_state.stx_mask&mask)!=mask||!S_ISREG(acquisition->payload_state.stx_mode)||
    !acquisition->payload_state.stx_size||acquisition->payload_state.stx_mnt_id!=acquisition->directory_state.stx_mnt_id){
    acquisition->result=result?result:-1;goto done;}
 acquisition->result=0;
done:
 acquisition->returned=1;return acquisition->result;
}
#endif