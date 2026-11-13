/* UNRUN actual startup roster; substituted context parser, no native authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned calls,fail_at;
long sl_linux_interpret_candidate_socket_context(sl_linux_candidate_socket_context *o){calls++;o->attempted=o->returned=1;o->result=calls==fail_at?-77:0;return o->result;}
#include "../candidate_startup_socket_context_linux.c"
typedef struct{sl_linux_public_startup s;sl_linux_public_controller c;sl_linux_candidate_policy policies[1];sl_linux_candidate_socket_context rows[2];} fixture;
static void prepare(fixture *f){f->s.controller_original=&f->c;f->c.startup_original=&f->s;f->s.returned=f->s.candidate_preexec_returned=1;f->s.stage=SL_STARTUP_ACQUIRED;f->s.actual_candidate_policies=f->policies;f->s.actual_policy_capacity=f->s.actual_candidate_policy_capacity=f->s.mounts.selinux_count=1;f->s.candidate_socket_contexts=f->rows;f->s.candidate_socket_context_capacity=2;for(unsigned i=0;i<2;i++){f->rows[i].policy_original=f->policies;f->rows[i].peer_original=&f->c.entry.custody_peers[i];}}
int main(void){fixture cases[5]={0};for(unsigned i=0;i<5;i++){fixture *f=&cases[i];prepare(f);unsigned before=calls;fail_at=i==1?before+1:i==2?before+2:0;if(i==3)f->rows[1].peer_original=&f->c.entry.custody_peers[0];if(i==4)f->s.candidate_preexec_result=-88;long result=sl_linux_interpret_startup_socket_contexts(&f->s);assert(f->s.candidate_socket_context_returned&&f->s.actual_candidate_socket_contexts==f->rows);if(i==0)assert(!result&&calls==before+2&&f->s.candidate_socket_context_count==2);else if(i==1)assert(result==-77&&calls==before+1&&!f->rows[1].attempted);else if(i==2)assert(result==-77&&calls==before+2&&!f->rows[0].result);else assert(result==-EINVAL&&calls==before&&!f->rows[0].attempted);before=calls;assert(sl_linux_interpret_startup_socket_contexts(&f->s)==-EINVAL&&calls==before);}return 0;}