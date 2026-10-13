/* UNRUN controlled raw-call fixture: no real cgroup/rights/native proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../local_cgroup_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <sys/stat.h>
static unsigned calls,opens;static long capability_result,target_result;static long filesystem=CGROUP2_SUPER_MAGIC;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 long result=0;
 call->source=source;call->number=number;call->arguments[0]=a;call->arguments[1]=b;call->arguments[2]=c;
 call->arguments[3]=d;call->arguments[4]=e;call->arguments[5]=f;atomic_store(&call->entered,1);calls++;
 if(number==SYS_capget)result=capability_result;
 else if(number==SYS_openat2){opens++;result=opens==1?7:(target_result?target_result:8);}
 else if(number==SYS_fstatfs)((struct statfs *)b)->f_type=filesystem;
 else if(number==SYS_statx){struct statx *state=(struct statx *)e;state->stx_mask=d;state->stx_mode=S_IFDIR|0700;state->stx_mnt_id=11;state->stx_ino=a;}
 else if(number==SYS_openat){opens++;result=9;}
 call->result=result;atomic_store(&call->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=fd;lift->held_fd=fd;return fd;}
#include "../local_cgroup_linux.c"
static sl_linux_existing_cgroup input(sl_linux_entry *entry){
 return (sl_linux_existing_cgroup){.entry_original=entry,.root_locator="/existing/cgroup",.root_length=16,.root_capacity=17,
  .relative_locator="owned-target",.relative_length=12,.relative_capacity=13};
}
int main(void){
 sl_linux_entry entry={0};sl_linux_existing_cgroup acquisition;unsigned previous;
 entry.public_streams_prepared=1;
 acquisition=input(&entry);capability_result=-EPERM;
 assert(sl_linux_acquire_existing_cgroup(&acquisition)==-EPERM);
 assert(acquisition.capabilities.result==-EPERM&&atomic_load(&acquisition.capabilities.returned)&&opens==0);
 capability_result=0;calls=opens=0;filesystem=0;
 acquisition=input(&entry);assert(sl_linux_acquire_existing_cgroup(&acquisition)==-1);
 assert(acquisition.root_fd==7&&acquisition.target_fd==-1&&opens==1);
 assert(acquisition.root_filesystem.result==0&&acquisition.root_fs.f_type==0);
 filesystem=CGROUP2_SUPER_MAGIC;calls=opens=0;target_result=-EACCES;
 acquisition=input(&entry);assert(sl_linux_acquire_existing_cgroup(&acquisition)==-EACCES);
 assert(acquisition.root_fd==7&&acquisition.target_fd==-1&&acquisition.open_target.result==-EACCES&&opens==2);
 previous=calls;assert(sl_linux_acquire_existing_cgroup(&acquisition)==-1&&calls==previous);
 calls=opens=0;target_result=0;
 acquisition=input(&entry);assert(sl_linux_acquire_existing_cgroup(&acquisition)==0);
 assert(acquisition.root_fd==7&&acquisition.target_fd==8&&acquisition.processes_fd==9&&opens==3);
 assert(acquisition.target_state.stx_mnt_id==acquisition.root_state.stx_mnt_id);
 /* Acquisition success does not bind this FD into birth or grant admission. */
 assert(entry.birth.cgroup_fd==0&&entry.birth.attempted==0);
 calls=opens=0;acquisition=input(&entry);acquisition.relative_locator="../escape";acquisition.relative_length=9;acquisition.relative_capacity=10;
 assert(sl_linux_acquire_existing_cgroup(&acquisition)==-1&&calls==0);
 return 0;
}
#endif
