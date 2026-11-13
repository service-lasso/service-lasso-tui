/* UNRUN controlled source regression. Link parent_views_linux.c and
 * stream_routes_linux.c with this fixture. No kernel/birth/native proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../public_controller_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned calls;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 assert(number==SYS_close);calls++;call->source=source;call->number=number;
 call->arguments[0]=a0;call->arguments[1]=a1;call->arguments[2]=a2;call->arguments[3]=a3;call->arguments[4]=a4;call->arguments[5]=a5;
 atomic_store(&call->entered,1);call->result=a0==50?-EINTR:a0==60?-EIO:0;atomic_store(&call->returned,1);return call->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){(void)lift;(void)fd;assert(0);return -1;}
int main(void){
 sl_linux_public_controller owner={0};sl_linux_bootstrap bootstrap={0};unsigned index;
 bootstrap.clone_parent.result=42;atomic_init(&bootstrap.clone_parent.returned,1);
 owner.entry.birth.bootstrap=&bootstrap;owner.birth_result=42;
 owner.entry.custody_endpoints[1]=50;owner.entry.entry_endpoints[0]=51;owner.pty_slave=52;owner.pty_master=53;
 for(index=0;index<3;index++){owner.routes[index].attempted=1;owner.routes[index].public_original=&owner.entry.public_streams[index];}
 owner.routes[0].child_fd=60;owner.routes[1].shares_controlling=1;owner.routes[1].child_fd=52;owner.routes[1].owner_fd=53;owner.routes[2].child_fd=61;
 assert(sl_linux_release_parent_child_views(&owner)==-EINTR);
 assert(calls==5&&owner.parent_close_primary==&owner.parent_custody_child_close);
 assert(owner.parent_gate_child_close.result==0&&atomic_load(&owner.parent_gate_child_close.returned));
 assert(owner.parent_slave_close.result==0&&atomic_load(&owner.parent_slave_close.returned));
 assert(owner.routes[0].close_child.result==-EIO&&owner.routes[0].close_attempted);
 assert(owner.routes[2].close_child.result==0&&owner.routes[2].close_attempted);
 assert(!owner.reader_start_result&&!owner.collector_create_entered);
 assert(sl_linux_release_parent_child_views(&owner)==-1&&calls==5);
 return 0;
}
#endif