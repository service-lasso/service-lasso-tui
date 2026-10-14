/* UNRUN raw-call fixture, no genuine procfs/installed/loaded-byte proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../local_installation_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <errno.h>
#include <fcntl.h>
static unsigned calls,mismatch;static const char *location="/installed/service-lasso-tui";static long public_error;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 long result=0;unsigned long n;calls++;call->source=source;call->number=number;
 call->arguments[0]=a;call->arguments[1]=b;call->arguments[2]=c;call->arguments[3]=d;call->arguments[4]=e;call->arguments[5]=f;
 atomic_store(&call->entered,1);
 if(number==SYS_openat)result=a==AT_FDCWD?7:8;
 else if(number==SYS_fstatfs)((struct statfs *)b)->f_type=PROC_SUPER_MAGIC;
 else if(number==SYS_readlinkat){n=strlen(location);if(n>(unsigned long)d)n=d;memcpy((void *)c,location,n);result=n;}
 else if(number==SYS_openat2){
  if(a==AT_FDCWD){assert(!strcmp((char *)b,"/installed"));result=9;}
  else{assert(a==9);result=!strcmp((char *)b,"service-lasso-tui")?(public_error?public_error:11):10;}
 }else if(number==SYS_statx){struct statx *state=(struct statx *)e;state->stx_mask=d;state->stx_size=128;
  state->stx_mode=a==9?S_IFDIR|0755:S_IFREG|0755;state->stx_mnt_id=3;state->stx_dev_major=8;state->stx_dev_minor=1;
  state->stx_ino=a==8||a==11?42:43;if(a==11&&mismatch)state->stx_ino=99;
 }
 call->result=result;atomic_store(&call->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=fd;lift->held_fd=fd;return fd;}
#include "../local_payload_linux.c"
#include "../local_installation_linux.c"
static void prepare(sl_linux_local_installation *installation,sl_linux_entry *entry,char path[64],char directory[64]){
 memset(installation,0,sizeof(*installation));installation->entry_original=entry;
 installation->executable_path=path;installation->executable_capacity=64;
 installation->directory_path=directory;installation->directory_capacity=64;calls=mismatch=0;public_error=0;
}
int main(void){
 sl_linux_entry entry={0};sl_linux_local_installation installation;char path[64],directory[64];unsigned before;
 entry.public_streams_prepared=1;prepare(&installation,&entry,path,directory);
 assert(sl_linux_acquire_local_installation(&installation)==0&&!strcmp(directory,"/installed"));
 assert(installation.proc_fd==7&&installation.executable_fd==8&&installation.payload.directory_fd==9&&installation.payload.payload_fd==10&&installation.public_fd==11);
 assert(installation.raw_link_count==(long)strlen(location)&&!memcmp(path,location,strlen(location)));
 assert(entry.birth.attempted==0&&entry.birth.payload_fd==0);
 before=calls;assert(sl_linux_acquire_local_installation(&installation)==-1&&calls==before);
 prepare(&installation,&entry,path,directory);mismatch=1;
 assert(sl_linux_acquire_local_installation(&installation)==-1&&installation.public_state.stx_ino==99&&installation.executable_state.stx_ino==42);
 prepare(&installation,&entry,path,directory);public_error=-EACCES;
 assert(sl_linux_acquire_local_installation(&installation)==-EACCES&&installation.payload.payload_fd==10&&installation.public_fd==-1);
 prepare(&installation,&entry,path,directory);installation.executable_capacity=8;
 assert(sl_linux_acquire_local_installation(&installation)==-1&&installation.raw_link_count==8&&installation.payload.attempted==0);
 return 0;
}
#endif