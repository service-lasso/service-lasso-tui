/* UNRUN synthetic native substitutions exercising the SAME production bodies.
 * Each fixture owns independent roots/backing. No real kernel concurrency,
 * provider, descriptor lifetime, reservation or startup acceptance credit. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include "../deadline_poll_linux.h"
static unsigned clocks,polls,mode;
long sl_linux_call6(sl_linux_call *c,const char *source,long n,long a0,long a1,long a2,long a3,long a4,long a5){
 c->source=source;c->number=n;c->arguments[0]=a0;c->arguments[1]=a1;c->arguments[2]=a2;
 c->arguments[3]=a3;c->arguments[4]=a4;c->arguments[5]=a5;atomic_store(&c->entered,1);
 long result=0;
 if(n==SYS_clock_gettime){
  struct timespec *t=(void*)a1;assert(a0==CLOCK_MONOTONIC&&!a2&&!a3&&!a4&&!a5);
  t->tv_sec=0;t->tv_nsec=100+clocks;
  if((mode==3&&clocks==1)||(mode==4&&clocks==2))t->tv_nsec=110;
  if(mode==5&&clocks==2)result=-EIO;
  clocks++;
 }else{
  assert(n==SYS_ppoll&&a1==(mode==9?0:1)&&!a3&&!a4&&!a5);polls++;
  struct pollfd *f=(void*)a0;struct timespec *timeout=(void*)a2;
  assert(timeout->tv_sec==0&&timeout->tv_nsec==9);
  if(mode!=9){assert(f->fd==7&&f->events==POLLIN);f->revents=POLLIN;}
  timeout->tv_nsec=4; /* separate real in/out semantics model */
  result=mode==9?0:mode==2?-EINTR:mode==6?2:1;
  if(mode==7)f->fd=8;
  if(mode==8)f->revents=0;
 }
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../deadline_linux.c"
#include "../deadline_poll_linux.c"
typedef struct fixture {
 sl_linux_deadline_source source;sl_linux_deadline root;
 sl_linux_deadline_sequence sequence;sl_linux_deadline_observation observations[3];
 struct pollfd descriptors[1],outputs[1];sl_linux_deadline_poll poll;
} fixture;
static void prepare(fixture *f,unsigned selected_mode,unsigned capacity){
 memset(f,0,sizeof(*f));mode=selected_mode;clocks=polls=0;
 f->source=(sl_linux_deadline_source){&f->poll,sizeof(f->poll),10};
 assert(sl_linux_construct_deadline(&f->root,&f->source)==0);
 f->sequence.root_original=&f->root;f->sequence.observations_original=f->observations;f->sequence.capacity=capacity;
 assert(sl_linux_prepare_deadline_sequence(&f->sequence)==0);
 f->descriptors[0]=(struct pollfd){7,POLLIN,0};
 f->poll.sequence_original=&f->sequence;f->poll.descriptors_original=f->descriptors;f->poll.outputs_original=f->outputs;f->poll.count=1;
}
int main(void){
 fixture success;prepare(&success,0,3);
 assert(sl_linux_poll_original_deadline(&success.poll)==0&&success.poll.timely&&polls==1&&clocks==3);
 assert(success.poll.requested_timeout.tv_nsec==9&&success.poll.timeout.tv_nsec==4);
 assert(success.poll.before==&success.observations[0]&&success.poll.after==&success.observations[1]);
 assert(success.descriptors[0].revents==0&&success.outputs[0].revents==POLLIN);
 sl_linux_deadline_poll saved=success.poll;assert(sl_linux_poll_original_deadline(&success.poll)==-EINVAL);
 assert(!memcmp(&saved,&success.poll,sizeof(saved))&&polls==1&&clocks==3);
 fixture interrupted;prepare(&interrupted,2,3);
 assert(sl_linux_poll_original_deadline(&interrupted.poll)==-EINTR&&!interrupted.poll.timely&&polls==1&&clocks==3);
 assert(interrupted.outputs[0].revents==POLLIN&&interrupted.poll.timeout.tv_nsec==4&&interrupted.poll.after->returned);
 fixture before;prepare(&before,3,3);
 assert(sl_linux_poll_original_deadline(&before.poll)==1&&!polls&&clocks==2&&!before.poll.poll_returned&&!before.poll.after);
 fixture late;prepare(&late,4,3);
 assert(sl_linux_poll_original_deadline(&late.poll)==1&&polls==1&&!late.poll.timely&&late.poll.raw_poll==1&&late.outputs[0].revents==POLLIN);
 fixture failed;prepare(&failed,5,3);
 assert(sl_linux_poll_original_deadline(&failed.poll)==-EIO&&polls==1&&clocks==3&&failed.poll.raw_poll==1&&!failed.poll.timely);
 for(unsigned m=6;m<=8;m++){
  fixture malformed;prepare(&malformed,m,3);
  assert(sl_linux_poll_original_deadline(&malformed.poll)==-EINVAL&&polls==1&&clocks==3&&!malformed.poll.timely);
 }
 fixture exhausted;prepare(&exhausted,0,1);
 assert(sl_linux_poll_original_deadline(&exhausted.poll)==-ENOSPC&&!polls&&clocks==1&&!exhausted.sequence.next);
 fixture alias;prepare(&alias,0,3);alias.poll.outputs_original=(void*)&alias.observations[2];
 fixture alias_saved=alias;assert(sl_linux_poll_original_deadline(&alias.poll)==-EINVAL&&!polls&&clocks==1);
 assert(!memcmp(&alias_saved,&alias,sizeof(alias)));
 fixture zero;prepare(&zero,9,3);zero.poll.count=0;
 /* Nonnull prebound containing pointers are ignored, not dereferenced at
  * zero native count, and their future source/output backing is unchanged. */
 assert(sl_linux_poll_original_deadline(&zero.poll)==0&&zero.poll.timely&&polls==1&&clocks==3);
 assert(zero.descriptors[0].fd==7&&!zero.outputs[0].fd&&!zero.outputs[0].revents);
 return 0;
}
#endif
