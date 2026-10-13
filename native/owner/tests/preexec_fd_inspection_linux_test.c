/* UNRUN actual authority consumer's FD subcut. Positive FD inspection still
 * fails deliberately absent credential/profile observations; no admission. */
#include "../owner_linux.h"
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include "../preexec_inspection_linux.c"
typedef struct {sl_linux_entry entry;sl_linux_capability_domain domain;sl_linux_bootstrap bootstrap;sl_linux_fd_action actions[3],inputs[3];sl_linux_authority_inspection observation;} episode;
static void call(sl_linux_call *c,long number,long result){atomic_init(&c->entered,1);atomic_init(&c->returned,1);c->source="controlled.original";c->number=number;c->result=result;}
int main(void){static episode cases[7];for(unsigned mode=0;mode<7;mode++){
 episode *e=&cases[mode];sl_linux_bootstrap *b=&e->bootstrap;e->entry.birth.bootstrap=b;e->entry.birth.attempted=1;e->observation.birth_original=&e->entry.birth;
 atomic_init(&b->failed,0);atomic_init(&b->fd_actions_prepared,1);atomic_init(&b->authority.returned,mode?1:0);
 b->fd_actions=e->actions;b->fd_action_count=3;b->controlling_terminal_fd=9;
 call(&b->create_session,SYS_setsid,123);call(&b->acquire_terminal,SYS_ioctl,0);b->acquire_terminal.arguments[0]=9;b->acquire_terminal.arguments[1]=TIOCSCTTY;
 for(unsigned i=0;i<3;i++){e->actions[i].outer_original=e;e->actions[i].source_fd=10+i;e->actions[i].call.arguments[0]=10+i;}
 e->actions[0].kind=SL_FD_CLOSE_VIEW;call(&e->actions[0].call,SYS_close,0);
 e->actions[1].kind=SL_FD_DUPLICATE_VIEW;e->actions[1].target_fd=1;call(&e->actions[1].call,SYS_dup3,1);e->actions[1].call.arguments[1]=1;
 e->actions[2].kind=SL_FD_CLEAR_EXEC;call(&e->actions[2].call,SYS_fcntl,0);e->actions[2].call.arguments[1]=F_SETFD;
 e->domain.entry_original=&e->entry;b->authority.domain_original=&e->domain;
 e->entry.fd_actions_attempted=1;e->entry.fd_action_inputs=e->inputs;e->entry.fd_action_input_count=3;e->entry.fd_action_mapping_bytes=sizeof(e->actions);e->entry.fd_action_mapping_result=(long)e->actions;
 call(&e->entry.fd_action_mapping,SYS_mmap,(long)e->actions);e->entry.fd_action_mapping.arguments[1]=sizeof(e->actions);e->entry.fd_action_mapping.arguments[2]=PROT_READ|PROT_WRITE;e->entry.fd_action_mapping.arguments[3]=MAP_SHARED|MAP_ANONYMOUS;e->entry.fd_action_mapping.arguments[4]=-1;
 for(unsigned i=0;i<3;i++){e->inputs[i].kind=e->actions[i].kind;e->inputs[i].source_fd=e->actions[i].source_fd;e->inputs[i].target_fd=e->actions[i].target_fd;e->inputs[i].outer_original=e->actions[i].outer_original;}
 if(mode==5)e->inputs[1].target_fd=2;
 if(mode==6)e->entry.fd_action_mapping.arguments[3]=MAP_PRIVATE|MAP_ANONYMOUS;
 if(mode==0)e->actions[0].call.result=-9;
 if(mode==2)e->actions[1].call.result=-4;
 if(mode==3)e->actions[1].call.number=SYS_close;
 if(mode==4)atomic_store(&e->actions[1].call.returned,0);
 long result=sl_linux_inspect_preexec_authority(&e->observation);
 assert(e->observation.returned&&result==(mode==0?1:-1));
 if(mode==0)assert(e->observation.not_ready&&!e->observation.checked_fd_actions&&!e->observation.fd_failure_original&&e->actions[0].call.result==-9);
 else if(mode==1)assert(e->observation.checked_fd_actions==3&&!e->observation.fd_failure_original);
 else if(mode!=6)assert(e->observation.checked_fd_actions==1&&e->observation.fd_failure_original==&e->actions[1]);
 if(mode==6)assert(!e->observation.checked_fd_actions&&!e->observation.fd_inputs_original&&!e->observation.fd_failure_original); assert(sl_linux_inspect_preexec_authority(&e->observation)==-1);
 }assert(cases[2].actions[1].call.result==-4&&cases[3].actions[1].call.number==SYS_close&&!atomic_load(&cases[4].actions[1].call.returned));return 0;}
