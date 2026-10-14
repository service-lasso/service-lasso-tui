/* UNRUN controlled source regression. Link only stream_routes_linux.c with this
 * fixture: no kernel call, native admission or actual terminal proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../stream_routes_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
static long query_result;
static unsigned calls,pipe_calls;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 call->source=source;call->number=number;
 call->arguments[0]=a0;call->arguments[1]=a1;call->arguments[2]=a2;
 call->arguments[3]=a3;call->arguments[4]=a4;call->arguments[5]=a5;
 atomic_store(&call->entered,1);calls++;
 if(number==SYS_pipe2){pipe_calls++;call->result=-ENOSPC;}else call->result=query_result;
 atomic_store(&call->returned,1);return call->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){(void)lift;(void)fd;assert(0);return -1;}
static void check_unknown(long result){
 sl_linux_public_stream public_view={0};sl_linux_stream_route route={0};
 atomic_init(&public_view.original_flags.returned,1);atomic_init(&public_view.duplicate.returned,1);
 public_view.held_fd=99;public_view.duplicate.result=99;
 route.public_original=&public_view;route.stream_index=1;
 query_result=result;calls=0;pipe_calls=0;
 assert(sl_linux_construct_stream_route(&route,10,11)==result);
 assert(calls==1&&pipe_calls==0&&route.query.result==result&&route.owner_fd==-1&&route.child_fd==-1);
}
int main(void){
 sl_linux_public_stream public_view={0};sl_linux_stream_route route={0};
 check_unknown(-EBADF);check_unknown(-EIO);check_unknown(-EPERM);
 atomic_init(&public_view.original_flags.returned,1);atomic_init(&public_view.duplicate.returned,1);
 public_view.held_fd=99;public_view.duplicate.result=99;route.public_original=&public_view;route.stream_index=1;
 query_result=-ENOTTY;calls=0;pipe_calls=0;
 assert(sl_linux_construct_stream_route(&route,10,11)==-ENOSPC);
 assert(calls==2&&pipe_calls==1&&route.query.result==-ENOTTY&&route.pipe.result==-ENOSPC);
 memset(&route,0,sizeof(route));route.public_original=&public_view;route.stream_index=1;
 query_result=0;calls=0;pipe_calls=0;
 assert(sl_linux_construct_stream_route(&route,10,11)==0);
 assert(calls==1&&pipe_calls==0&&route.child_fd==11&&route.owner_fd==10&&route.shares_controlling);
 return 0;
}
#endif