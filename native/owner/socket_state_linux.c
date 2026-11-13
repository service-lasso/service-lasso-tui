#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "owner_linux.h"
#include <sys/syscall.h>
#include <sys/socket.h>
#include <stddef.h>
#include <errno.h>
long sl_linux_observe_entry_socket_state(sl_linux_entry *e,unsigned index){
 if(!e||index>1)return -EINVAL;sl_linux_socket_state *o=&e->custody_states[index];if(o->attempted)return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->endpoint_index=index;o->descriptor=e->custody_endpoints[index];
 sl_linux_socket_peer *peer=&e->custody_peers[index];
 if(!e->attempted||!peer->returned||peer->result||!peer->framing_complete||peer->descriptor!=o->descriptor||peer->endpoint_index!=index||peer->domain!=AF_UNIX||peer->type!=SOCK_STREAM||peer->protocol||o->descriptor<3||
    !atomic_load_explicit(&e->custody_pair.returned,memory_order_acquire)||e->custody_pair.number!=SYS_socketpair||e->custody_pair.result||e->custody_pair.arguments[0]!=AF_UNIX||e->custody_pair.arguments[1]!=(SOCK_STREAM|SOCK_CLOEXEC)||e->custody_pair.arguments[2]||e->custody_pair.arguments[3]!=(long)e->custody_raw_endpoints||
    !e->custody_lifts[index].attempted||e->custody_lifts[index].original_fd!=e->custody_raw_endpoints[index]||e->custody_lifts[index].held_fd!=o->descriptor)goto done;
 sl_linux_call *calls[3]={&o->local_query,&o->peer_query,&o->accept_query};
 for(unsigned i=0;i<3;i++)if(calls[i]->source||calls[i]->number)goto done;
 for(unsigned i=0;i<3;i++){atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);}
 o->local_length=sizeof(o->local_address);o->peer_length=sizeof(o->peer_address);o->accept_length=sizeof(o->accepting);
 o->local_result=sl_linux_call6(&o->local_query,"custody.socket.local-name",SYS_getsockname,o->descriptor,(long)&o->local_address,(long)&o->local_length,0,0,0);
 o->peer_result=sl_linux_call6(&o->peer_query,"custody.socket.peer-name",SYS_getpeername,o->descriptor,(long)&o->peer_address,(long)&o->peer_length,0,0,0);
 o->accept_result=sl_linux_call6(&o->accept_query,"custody.socket.accept-state",SYS_getsockopt,o->descriptor,SOL_SOCKET,SO_ACCEPTCONN,(long)&o->accepting,(long)&o->accept_length,0);
 if(o->local_result){o->result=o->local_result;goto done;}if(o->peer_result){o->result=o->peer_result;goto done;}if(o->accept_result){o->result=o->accept_result;goto done;}
 if(o->local_length!=offsetof(struct sockaddr_un,sun_path)||o->peer_length!=offsetof(struct sockaddr_un,sun_path)||o->local_address.sun_family!=AF_UNIX||o->peer_address.sun_family!=AF_UNIX||o->accept_length!=sizeof(o->accepting)||o->accepting)goto done;
 o->framing_complete=1;o->result=0;
 done:o->returned=1;return o->result;
 /* No enduring exclusion or caller identity follows from these observations. */
}
#endif
