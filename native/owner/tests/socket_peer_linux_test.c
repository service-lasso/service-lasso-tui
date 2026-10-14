/* UNRUN actual peer observation body, controlled syscall returns only. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../owner_linux.h"
#include <sys/syscall.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
static unsigned calls,mode;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){assert(n==SYS_getsockopt&&a==7&&b==SOL_SOCKET&&!g);calls++;c->source=s;c->number=n;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);long result=0;
 if(d==SO_DOMAIN)*(int *)e=AF_UNIX;else if(d==SO_TYPE)*(int *)e=mode==2?SOCK_DGRAM:SOCK_STREAM;else if(d==SO_PROTOCOL)*(int *)e=0;else{assert(d==SO_PEERSEC);unsigned char *bytes=(unsigned char *)e;uint32_t *length=(uint32_t *)f;if(mode==3){*length=4097;result=-ERANGE;}else{memcpy(bytes,"u:r:s:s0",8);*length=mode==1?9:8;if(mode==1)bytes[8]=0;if(mode==4)bytes[2]=0;}}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../socket_peer_linux.c"
typedef struct{sl_linux_entry entry;unsigned char bytes[2][32];} fixture;
int main(void){fixture cases[6]={0};for(mode=0;mode<6;mode++){fixture *f=&cases[mode];sl_linux_entry *e=&f->entry;e->attempted=1;for(unsigned i=0;i<2;i++){e->custody_peers[i].bytes=f->bytes[i];e->custody_peers[i].capacity=32;e->custody_endpoints[i]=7+i;e->custody_raw_endpoints[i]=7+i;e->custody_lifts[i].attempted=1;e->custody_lifts[i].original_fd=e->custody_lifts[i].held_fd=7+i;}
 atomic_init(&e->custody_pair.entered,1);atomic_init(&e->custody_pair.returned,1);e->custody_pair.number=SYS_socketpair;e->custody_pair.arguments[0]=AF_UNIX;e->custody_pair.arguments[1]=SOCK_STREAM|SOCK_CLOEXEC;e->custody_pair.arguments[3]=(long)e->custody_raw_endpoints;if(mode==5)e->custody_lifts[0].held_fd++;
 unsigned before=calls;long result=sl_linux_observe_entry_peer(e,0);sl_linux_socket_peer *o=&e->custody_peers[0];assert(o->returned&&o->actual_bytes==f->bytes[0]&&o->descriptor==7);
 if(mode<2)assert(!result&&o->framing_complete&&o->extent==8&&o->security_length==(mode==1?9:8));else assert(result&&!o->framing_complete);
 assert(calls==before+(mode==5?0:4));if(mode==3)assert(result==-ERANGE&&o->security_result==-ERANGE&&o->security_length==4097);
 before=calls;assert(sl_linux_observe_entry_peer(e,0)==-EINVAL&&calls==before);
 }return 0;}