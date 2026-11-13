/* UNRUN actual read-only observer with controlled native return cells. */
#include "../owner_linux.h"
#include <sys/syscall.h>
#include <sys/socket.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
static unsigned mode,calls;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 calls++;c->source=s;c->number=n;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);long r=0;assert(a==7);
 if(n==SYS_getsockname||n==SYS_getpeername){struct sockaddr_un *addr=(void*)b;addr->sun_family=mode==4?AF_INET:AF_UNIX;*(uint32_t*)d=offsetof(struct sockaddr_un,sun_path)+(mode==3?1:0);if(mode==1&&n==SYS_getsockname)r=-EIO;if(mode==2&&n==SYS_getpeername)r=-ENOTCONN;}
 else {assert(n==SYS_getsockopt&&b==SOL_SOCKET&&d==SO_ACCEPTCONN);*(int*)e=mode==5;*(uint32_t*)f=sizeof(int);if(mode==1)r=-EPERM;}
 c->result=r;atomic_store(&c->returned,1);return r;
}
#include "../socket_state_linux.c"
int main(void){sl_linux_entry originals[7]={0};for(mode=0;mode<7;mode++){
 sl_linux_entry *e=&originals[mode];e->attempted=1;e->custody_endpoints[0]=e->custody_raw_endpoints[0]=7;e->custody_lifts[0].attempted=1;e->custody_lifts[0].original_fd=e->custody_lifts[0].held_fd=7;
 sl_linux_socket_peer *p=&e->custody_peers[0];p->returned=p->framing_complete=1;p->descriptor=7;p->domain=AF_UNIX;p->type=SOCK_STREAM;
 atomic_init(&e->custody_pair.entered,1);atomic_init(&e->custody_pair.returned,1);e->custody_pair.number=SYS_socketpair;e->custody_pair.arguments[0]=AF_UNIX;e->custody_pair.arguments[1]=SOCK_STREAM|SOCK_CLOEXEC;e->custody_pair.arguments[3]=(long)e->custody_raw_endpoints;
 if(mode==6)e->custody_lifts[0].held_fd=8;unsigned before=calls;long r=sl_linux_observe_entry_socket_state(e,0);sl_linux_socket_state *o=&e->custody_states[0];assert(o->returned);
 if(!mode)assert(!r&&o->framing_complete);else if(mode==1)assert(r==-EIO&&o->accept_result==-EPERM&&o->local_result==-EIO);else if(mode==2)assert(r==-ENOTCONN);else assert(r==-EINVAL&&!o->framing_complete);
 assert(calls==before+(mode==6?0:3));before=calls;assert(sl_linux_observe_entry_socket_state(e,0)==-EINVAL&&calls==before);
 }return 0;}
