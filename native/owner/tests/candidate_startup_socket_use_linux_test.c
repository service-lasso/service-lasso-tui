/* UNRUN actual fixed READ/WRITE caller, controlled classifier results. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned calls,fail_at;
long sl_linux_interpret_candidate_socket_use(sl_linux_candidate_socket_create *o){calls++;o->attempted=o->returned=1;o->result=calls==fail_at?-79:0;return o->result;}
#include "../candidate_startup_socket_use_linux.c"
typedef struct {sl_linux_public_startup s;sl_linux_candidate_roles role;sl_linux_selinux_context owner;sl_linux_candidate_payload_context payload;sl_linux_candidate_socket_context sockets[2];sl_linux_candidate_socket_create rows[4];} episode;
static void prepare(episode *e){
 sl_linux_public_startup *s=&e->s;s->candidate_socket_create_returned=1;s->actual_policy_capacity=s->actual_candidate_role_capacity=s->actual_context_capacity=s->actual_candidate_payload_capacity=s->mounts.selinux_count=1;
 s->actual_candidate_roles=&e->role;s->actual_contexts=&e->owner;s->actual_candidate_payloads=&e->payload;s->actual_candidate_socket_contexts=e->sockets;s->actual_candidate_socket_context_capacity=2;s->candidate_socket_uses=e->rows;s->candidate_socket_use_capacity=4;
 for(unsigned i=0;i<4;i++){e->rows[i].roles_original=&e->role;e->rows[i].owner_original=&e->owner;e->rows[i].payload_original=&e->payload.reachable;e->rows[i].socket_original=&e->sockets[i/2];e->rows[i].purpose=1+i%2;}
}
int main(void){episode cases[5]={0};for(unsigned i=0;i<5;i++){
 episode *e=&cases[i];prepare(e);unsigned before=calls;fail_at=i==1?before+3:0;
 if(i==2)e->rows[3].purpose=1;if(i==3)e->rows[2].socket_original=e->sockets;if(i==4)e->s.candidate_socket_create_result=-70;
 long result=sl_linux_interpret_startup_socket_uses(&e->s);
 if(!i)assert(!result&&calls==before+4&&e->s.candidate_socket_use_count==4);
 else if(i==1)assert(result==-79&&calls==before+3&&!e->rows[3].attempted&&e->rows[2].result==-79);
 else assert(result==-EINVAL&&calls==before&&!e->rows[0].attempted);
 before=calls;assert(sl_linux_interpret_startup_socket_uses(&e->s)==-EINVAL&&calls==before);
 }return 0;}
