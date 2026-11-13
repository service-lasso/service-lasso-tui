/* UNRUN actual supervisor body; helper substitutions model control flow only.
 * No genuine deadline, poll, process, provider, concurrency or fit evidence. */
#include "../outer_supervisor_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
static unsigned mode,polls,observations,afters;
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *s,const void *p,unsigned long n){return s&&p&&n;}
long sl_linux_poll_original_deadline(sl_linux_deadline_poll *p){
 polls++;assert(p->count==1&&p->descriptors_original->fd==42&&p->descriptors_original->events==POLLIN);
 p->actual_sequence=p->sequence_original;p->actual_sequence->next+=2;p->returned=1;
 p->raw_poll=mode==3?-EINTR:1;p->timely=mode!=3;
 *p->outputs_original=*p->descriptors_original;p->outputs_original->revents=mode==6?POLLERR:POLLIN;
 return p->result=mode==3?-EINTR:0;
}
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){b->sequence=s;s->next++;return 0;}
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *b){afters++;b->sequence->next++;return mode==4?-ETIME:0;}
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *b){return b&&mode!=4;}
long sl_linux_observe_outer_terminal(sl_linux_outer_terminal *o){
 observations++;o->actual_birth=o->birth_original;o->attempted=o->returned=1;
 o->terminal=mode==0||mode==4||(mode==1&&observations==2);
 return o->result=o->terminal?0:1;
}
#include "../outer_supervisor_linux.c"
int main(void){
 static struct fixture {sl_linux_outer_birth b;sl_linux_outer_continuation c;sl_linux_deadline_sequence s;sl_linux_outer_turn turns[2];sl_linux_outer_terminal obs[2];sl_linux_outer_supervisor owner;} cases[8];
 for(mode=0;mode<8;mode++){
  struct fixture *f=&cases[mode];polls=observations=afters=0;
  f->b.raw_parent=100;f->b.pidfd=42;atomic_store(&f->b.parent_captured,1);
  f->c.birth_original=f->c.actual_birth=&f->b;f->c.attempted=1;
  f->c.parent.raw=100;f->c.parent.captured=f->c.parent.matched=1;atomic_store(&f->c.parent.published,mode!=5);
  f->s.actual_capacity=8;
  f->obs[0].birth_original=f->obs[1].birth_original=&f->b;
  f->owner.continuation_original=&f->c;f->owner.sequence_original=&f->s;f->owner.turns_original=f->turns;f->owner.observations_original=f->obs;f->owner.capacity=2;
  if(mode==7)f->owner.turns_original=(void*)&f->c;
  sl_linux_outer_supervisor before=f->owner;sl_linux_outer_continuation held=f->c;
  long result=sl_linux_supervise_outer_parent(&f->owner);
  assert(!memcmp(&held,&f->c,sizeof(held)));
  if(mode>=5&&mode!=6){assert(result==-EINVAL&&!polls&&!observations&&!memcmp(&before,&f->owner,sizeof(before)));continue;}
  assert(f->owner.returned&&f->owner.next>0&&polls<=2);
  if(mode<2)assert(!result&&f->owner.terminal&&observations==(mode==0?1:2));
  else if(mode==2)assert(result==1&&!f->owner.terminal&&f->owner.stopped&&observations==2&&f->owner.next==2);
  else if(mode==3)assert(result==-EINTR&&!observations&&f->turns[0].poll.raw_poll==-EINTR);
  else if(mode==4)assert(result==-ETIME&&!f->owner.terminal&&observations==1&&afters==1&&f->obs[0].terminal);
  else assert(result==-EINVAL&&!observations&&f->turns[0].output.revents==POLLERR);
  unsigned old=polls;before=f->owner;
  assert(sl_linux_supervise_outer_parent(&f->owner)==-EINVAL&&polls==old&&!memcmp(&before,&f->owner,sizeof(before)));
 }
 return 0;
}