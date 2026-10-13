#if defined(__linux__) && defined(__x86_64__)
#include "outer_supervisor_linux.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
static int supervisor_disjoint(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int supervisor_fresh(const void *p,size_t n){const unsigned char *b=p;for(size_t i=0;i<n;i++)if(b[i])return 0;return 1;}
long sl_linux_supervise_outer_parent(sl_linux_outer_supervisor *o){
 if(!o||!o->continuation_original||!o->sequence_original||!o->turns_original||!o->observations_original||!o->capacity||
    o->capacity>SIZE_MAX/sizeof(*o->turns_original)||o->capacity>SIZE_MAX/sizeof(*o->observations_original)||o->capacity>ULONG_MAX/4)return -EINVAL;
 const sl_linux_outer_continuation *c=o->continuation_original;
 const sl_linux_outer_birth *b=c->birth_original;sl_linux_deadline_sequence *s=o->sequence_original;
 if(!b||!atomic_load_explicit(&c->parent.published,memory_order_acquire)||!c->attempted||c->actual_birth!=b||
    !c->parent.captured||!c->parent.matched||c->parent.result||c->parent.raw<=0||c->parent.raw>INT_MAX||
    !atomic_load_explicit(&b->parent_captured,memory_order_acquire)||b->raw_parent!=c->parent.raw||b->pidfd<0)return -EINVAL;
 const void *outputs[3]={o,o->turns_original,o->observations_original};
 size_t sizes[3]={sizeof(*o),o->capacity*sizeof(*o->turns_original),o->capacity*sizeof(*o->observations_original)};
 for(unsigned i=0;i<3;i++){
  if(!supervisor_disjoint(outputs[i],sizes[i],c,sizeof(*c))||!supervisor_disjoint(outputs[i],sizes[i],b,sizeof(*b))||
     !sl_linux_deadline_sequence_excludes_output(s,outputs[i],sizes[i]))return -EINVAL;
  for(unsigned j=0;j<i;j++)if(!supervisor_disjoint(outputs[i],sizes[i],outputs[j],sizes[j]))return -EINVAL;
 }
 /* Symmetric exclusion also protects native clock writers from these held
  * source records. The actual owning entry supplies all other referents. */
 if(!sl_linux_deadline_sequence_excludes_output(s,c,sizeof(*c))||!sl_linux_deadline_sequence_excludes_output(s,b,sizeof(*b))||
    !supervisor_fresh((const unsigned char*)o+offsetof(sl_linux_outer_supervisor,actual_continuation),sizeof(*o)-offsetof(sl_linux_outer_supervisor,actual_continuation))||
    !supervisor_fresh(o->turns_original,sizes[1])||s->next>s->actual_capacity||s->actual_capacity-s->next<4*o->capacity)return -EINVAL;
 for(unsigned long i=0;i<o->capacity;i++){
  sl_linux_outer_terminal *ob=&o->observations_original[i];
  if(ob->birth_original!=b||!supervisor_fresh((const unsigned char*)ob+offsetof(sl_linux_outer_terminal,actual_birth),sizeof(*ob)-offsetof(sl_linux_outer_terminal,actual_birth)))return -EINVAL;
 }
 o->attempted=1;o->actual_continuation=c;o->actual_sequence=s;o->actual_turns=o->turns_original;
 o->actual_observations=o->observations_original;o->actual_capacity=o->capacity;o->result=1;
 for(;o->next<o->actual_capacity;o->next++){
  sl_linux_outer_turn *t=&o->actual_turns[o->next];sl_linux_outer_terminal *ob=&o->actual_observations[o->next];
  t->attempted=1;t->result=-EINVAL;t->input=(struct pollfd){b->pidfd,POLLIN,0};
  t->poll.sequence_original=s;t->poll.descriptors_original=&t->input;t->poll.outputs_original=&t->output;t->poll.count=1;
  long result=sl_linux_poll_original_deadline(&t->poll);
  if(result){t->result=result;goto stop_turn;}
  if(!t->poll.returned||!t->poll.timely||t->poll.raw_poll!=1||t->output.fd!=b->pidfd||t->output.events!=POLLIN||t->output.revents!=POLLIN)goto stop_turn;
  t->boundary_result=sl_linux_begin_deadline_boundary(&t->boundary,s);
  if(t->boundary_result){t->result=t->boundary_result;goto stop_turn;}
  t->observation_result=sl_linux_observe_outer_terminal(ob);t->observation_returned=1;
  t->boundary_result=sl_linux_end_deadline_boundary(&t->boundary);t->boundary_returned=1;
  if(t->boundary_result||!sl_linux_deadline_boundary_matches_original(&t->boundary)){
   if(t->boundary_result)t->result=t->boundary_result;goto stop_turn;
  }
  if(t->observation_result<0){t->result=t->observation_result;goto stop_turn;}
  if(!ob->returned||ob->actual_birth!=b||(!t->observation_result&&!ob->terminal)||(t->observation_result==1&&ob->terminal))goto stop_turn;
  t->result=t->observation_result;t->returned=1;
  if(!t->observation_result&&ob->terminal){o->terminal=1;o->result=0;o->next++;goto done;}
  if(t->observation_result!=1||ob->terminal)goto stop_turn;
  continue;
 stop_turn:t->returned=1;o->result=t->result;o->stopped=1;o->next++;goto done;
 }
 o->stopped=1; /* Finite schedule exhausted with creator still UNKNOWN. */
 done:o->returned=1;return o->result;
 /* All native IO/clock originals survive expiry or refusal. No kill/reap,
  * pidfd close, process return, graph retirement, FINAL or late-ACK credit. */
}
#endif