/* UNRUN substituted raw calls; not installed-image/native/rights proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../local_payload_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
static unsigned calls,opens;static long payload_result;static int wrong_type,cross_mount;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 long result=0;calls++;
 call->source=source;call->number=number;call->arguments[0]=a;call->arguments[1]=b;call->arguments[2]=c;
 call->arguments[3]=d;call->arguments[4]=e;call->arguments[5]=f;atomic_store(&call->entered,1);
 if(number==SYS_openat2){opens++;if(opens==2){assert(a==7);assert(((struct open_how *)c)->flags==(O_RDONLY|O_NONBLOCK|O_CLOEXEC));assert(!strcmp((char *)b,".service-lasso-tui/service-lasso-tui-go"));}
  result=opens==1?7:(payload_result?payload_result:8);}
 else if(number==SYS_statx){struct statx *state=(struct statx *)e;state->stx_mask=d;
  state->stx_mode=a==7?S_IFDIR|0755:(wrong_type?S_IFDIR|0755:S_IFREG|0755);
  state->stx_size=128;state->stx_mnt_id=a==8&&cross_mount?12:11;state->stx_ino=a;}
 call->result=result;atomic_store(&call->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=fd;lift->held_fd=fd;return fd;}
#include "../local_payload_linux.c"
static sl_linux_installed_payload input(sl_linux_entry *entry){
 return (sl_linux_installed_payload){.entry_original=entry,.directory_locator="/installed",.directory_length=10,.directory_capacity=11};
}
int main(void){
 sl_linux_entry entry={0};sl_linux_installed_payload acquisition;unsigned before;
 acquisition=input(&entry);assert(sl_linux_acquire_installed_payload(&acquisition)==-1&&calls==0);
 entry.public_streams_prepared=1;payload_result=-EACCES;acquisition=input(&entry);
 assert(sl_linux_acquire_installed_payload(&acquisition)==-EACCES);
 assert(acquisition.directory_fd==7&&acquisition.payload_fd==-1&&acquisition.open_payload.result==-EACCES);
 before=calls;assert(sl_linux_acquire_installed_payload(&acquisition)==-1&&calls==before);
 calls=opens=0;payload_result=0;wrong_type=1;acquisition=input(&entry);
 assert(sl_linux_acquire_installed_payload(&acquisition)==-1&&acquisition.payload_fd==8);
 assert(S_ISDIR(acquisition.payload_state.stx_mode)&&atomic_load(&acquisition.payload_identity.returned));
 calls=opens=0;wrong_type=0;cross_mount=1;acquisition=input(&entry);
 assert(sl_linux_acquire_installed_payload(&acquisition)==-1&&acquisition.payload_state.stx_mnt_id==12);
 calls=opens=0;cross_mount=0;acquisition=input(&entry);
 assert(sl_linux_acquire_installed_payload(&acquisition)==0&&acquisition.directory_fd==7&&acquisition.payload_fd==8);
 assert(entry.birth.payload_fd==0&&entry.birth.attempted==0); /* no admission by acquisition */
 return 0;
}
#endif