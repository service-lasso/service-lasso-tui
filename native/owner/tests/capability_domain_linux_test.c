/* UNRUN controlled syscall cases; no actual procfs/kernel ABI evidence. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include "../capability_domain_linux.h"
static const char *input;static unsigned cursor,calls;static int wrong_fs,invalid_count,read_failure;
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=lift->held_fd=fd;return fd;}
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 long result;calls++;call->source=source;call->number=number;call->arguments[0]=a0;call->arguments[1]=a1;call->arguments[2]=a2;
 (void)a3;(void)a4;(void)a5;
 if(number==SYS_openat){assert((a2&(O_NONBLOCK|O_CLOEXEC))==(O_NONBLOCK|O_CLOEXEC));result=7;}
 else if(number==SYS_fstatfs){((struct statfs*)a1)->f_type=wrong_fs?0:PROC_SUPER_MAGIC;result=0;}
 else{assert(number==SYS_read&&a0==7&&a2==1);if(read_failure)result=-5;else if(invalid_count)result=2;
  else if(input[cursor]){*(unsigned char*)a1=(unsigned char)input[cursor++];result=1;}else result=0;}
 call->result=result;return result;
}
#include "../capability_domain_linux.c"
static void reset(sl_linux_capability_domain *d,sl_linux_entry *e,const char *bytes){
 memset(d,0,sizeof(*d));memset(e,0,sizeof(*e));d->entry_original=e;e->public_streams_prepared=1;
 input=bytes;cursor=calls=0;wrong_fs=invalid_count=read_failure=0;
}
int main(void){
 sl_linux_capability_domain d;sl_linux_entry e;unsigned prior;
 reset(&d,&e,"40\n");assert(sl_linux_observe_capability_domain(&d)==0&&d.last_cap==40&&d.eof&&d.count==4);
 assert(d.bytes[0]=='4'&&d.bytes[1]=='0'&&d.bytes[2]=='\n'&&d.raw_counts[3]==0);
 prior=calls;assert(sl_linux_observe_capability_domain(&d)==-1&&calls==prior&&d.last_cap==40);
 reset(&d,&e,"63\n");assert(sl_linux_observe_capability_domain(&d)==0&&d.last_cap==63);
 reset(&d,&e,"64\n");assert(sl_linux_observe_capability_domain(&d)==-1&&d.eof&&d.bytes[1]=='4');
 reset(&d,&e,"100\n");assert(sl_linux_observe_capability_domain(&d)==-1&&!d.eof&&d.count==4);
 reset(&d,&e,"40");assert(sl_linux_observe_capability_domain(&d)==-1&&d.eof);
 reset(&d,&e,"4\n");assert(sl_linux_observe_capability_domain(&d)==0&&d.last_cap==4&&d.count==3);
 reset(&d,&e,"40\n");invalid_count=1;assert(sl_linux_observe_capability_domain(&d)==-1&&d.raw_counts[0]==2&&d.reads[0].result==2);
 reset(&d,&e,"40\n");read_failure=1;assert(sl_linux_observe_capability_domain(&d)==-5&&d.raw_counts[0]==-5&&d.fd==7);
 reset(&d,&e,"40\n");wrong_fs=1;assert(sl_linux_observe_capability_domain(&d)==-1&&calls==2&&!d.count&&d.fd==7);
 reset(&d,&e,"40\n");e.public_streams_prepared=0;assert(sl_linux_observe_capability_domain(&d)==-1&&!calls);
 return 0;
}
#endif