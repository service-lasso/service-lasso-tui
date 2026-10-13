#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "owner_linux.h"
#include <sys/syscall.h>
#include <sys/socket.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
static int peer_separate(const void *a,unsigned long an,const void *b,unsigned long bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!an||!bn||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 0;return x+an<=y||y+bn<=x;}
long sl_linux_observe_entry_peer(sl_linux_entry *entry,unsigned index){
 if(!entry||index>1)return -EINVAL;sl_linux_socket_peer *o=&entry->custody_peers[index];if(o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->endpoint_index=index;o->descriptor=entry->custody_endpoints[index];o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;
 if(!atomic_load_explicit(&entry->custody_pair.returned,memory_order_acquire)||!atomic_load(&entry->custody_pair.entered)||entry->custody_pair.number!=SYS_socketpair||entry->custody_pair.result||entry->custody_pair.arguments[0]!=AF_UNIX||entry->custody_pair.arguments[1]!=(SOCK_STREAM|SOCK_CLOEXEC)||entry->custody_pair.arguments[2]||entry->custody_pair.arguments[3]!=(long)entry->custody_raw_endpoints||!entry->custody_lifts[index].attempted||entry->custody_lifts[index].original_fd!=entry->custody_raw_endpoints[index]||entry->custody_lifts[index].held_fd!=o->descriptor)goto done;
 if(!entry->attempted||o->descriptor<3||!o->actual_bytes||!o->actual_capacity||o->actual_capacity>UINT_MAX||!peer_separate(entry,sizeof *entry,o->actual_bytes,o->actual_capacity)||!peer_separate(o->actual_bytes,o->actual_capacity,entry->custody_peers[1-index].bytes,entry->custody_peers[1-index].capacity))goto done;
 sl_linux_call *calls[4]={&o->domain_call,&o->type_call,&o->protocol_call,&o->security_call};for(unsigned i=0;i<4;i++){if(calls[i]->source||calls[i]->number)goto done;atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);}
 o->domain_length=sizeof o->domain;o->type_length=sizeof o->type;o->protocol_length=sizeof o->protocol;o->security_length=(uint32_t)o->actual_capacity;
 o->domain_result=sl_linux_call6(&o->domain_call,"custody.peer.domain",SYS_getsockopt,o->descriptor,SOL_SOCKET,SO_DOMAIN,(long)&o->domain,(long)&o->domain_length,0);
 o->type_result=sl_linux_call6(&o->type_call,"custody.peer.type",SYS_getsockopt,o->descriptor,SOL_SOCKET,SO_TYPE,(long)&o->type,(long)&o->type_length,0);
 o->protocol_result=sl_linux_call6(&o->protocol_call,"custody.peer.protocol",SYS_getsockopt,o->descriptor,SOL_SOCKET,SO_PROTOCOL,(long)&o->protocol,(long)&o->protocol_length,0);
 o->security_result=sl_linux_call6(&o->security_call,"custody.peer.security",SYS_getsockopt,o->descriptor,SOL_SOCKET,SO_PEERSEC,(long)o->actual_bytes,(long)&o->security_length,0);
 if(o->domain_result){o->result=o->domain_result;goto done;}if(o->type_result){o->result=o->type_result;goto done;}if(o->protocol_result){o->result=o->protocol_result;goto done;}if(o->security_result){o->result=o->security_result;goto done;}
 if(o->domain_length!=sizeof o->domain||o->type_length!=sizeof o->type||o->protocol_length!=sizeof o->protocol||o->domain!=AF_UNIX||o->type!=SOCK_STREAM||o->protocol||!o->security_length||o->security_length>o->actual_capacity)goto done;
 o->extent=o->security_length;if(!o->actual_bytes[o->extent-1])o->extent--;if(!o->extent)goto done;
 for(unsigned long i=0;i<o->extent;i++){o->examined=i+1;if(!o->actual_bytes[i])goto done;}
 o->framing_complete=1;o->result=0;
 done:o->returned=1;return o->result;
 /* Peer SOCKET labels only. Current process domain, creation policy, supplier
  * routes and independently admitted loaded-code/OS premise stay distinct. */
}
#endif