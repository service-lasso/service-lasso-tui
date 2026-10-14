/* UNRUN real Linux filesystem case; no installed-member/native admission proof.
 * Execute only after complete source review and exact command admission. */
#if defined(__linux__) && defined(__x86_64__)
#define _GNU_SOURCE
#include "../local_payload_linux.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 long result;call->source=source;call->number=number;
 call->arguments[0]=a;call->arguments[1]=b;call->arguments[2]=c;call->arguments[3]=d;call->arguments[4]=e;call->arguments[5]=f;
 atomic_store(&call->entered,1);result=syscall(number,a,b,c,d,e,f);
 call->result=result<0?-errno:result;atomic_store(&call->returned,1);return call->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){
 int result;lift->attempted=1;lift->original_fd=fd;
 if(fd>=3){lift->held_fd=fd;return fd;}
 result=fcntl(fd,F_DUPFD_CLOEXEC,3);if(result<0)return -errno;
 lift->held_fd=result;if(close(fd)<0)return -errno;return result;
}
#include "../local_payload_linux.c"
int main(void){
 char directory[]="/tmp/service-lasso-payload-fifo-XXXXXX",private_directory[256],payload[320];
 sl_linux_entry entry={0};sl_linux_installed_payload acquisition={0};
 assert(mkdtemp(directory));
 assert(snprintf(private_directory,sizeof(private_directory),"%s/.service-lasso-tui",directory)>0);
 assert(snprintf(payload,sizeof(payload),"%s/service-lasso-tui-go",private_directory)>0);
 assert(mkdir(private_directory,0700)==0);assert(mkfifo(payload,0600)==0);
 /* No writer is ever opened: blocking O_RDONLY would not reach statx. */
 entry.public_streams_prepared=1;acquisition.entry_original=&entry;
 acquisition.directory_locator=directory;acquisition.directory_length=strlen(directory);acquisition.directory_capacity=sizeof(directory);
 assert(sl_linux_acquire_installed_payload(&acquisition)==-1);
 assert(acquisition.open_payload.result>=3&&atomic_load(&acquisition.open_payload.returned));
 assert(acquisition.payload_how.flags&O_NONBLOCK);
 assert(S_ISFIFO(acquisition.payload_state.stx_mode)&&atomic_load(&acquisition.payload_identity.returned));
 assert(entry.birth.attempted==0);
 /* Fixture cleanup only after all synchronous helper borrowers have returned. */
 assert(close(acquisition.payload_fd)==0);assert(close(acquisition.directory_fd)==0);
 assert(unlink(payload)==0);assert(rmdir(private_directory)==0);assert(rmdir(directory)==0);
 return 0;
}
#endif