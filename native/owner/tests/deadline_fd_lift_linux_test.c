/* Production lift body; native/clock substitutes are synthetic, UNRUN. */
#include "../deadline_fd_lift_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
static unsigned mode,calls,begins,ends;
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *s,const void *p,size_t n){return s&&p&&n;}
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){b->sequence=s;b->attempted=b->ready=1;begins++;return 0;}
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *b){b->after_attempted=b->returned=1;ends++;return (mode==2&&ends==1)?1:(mode==3&&ends==2)?-EIO:0;}
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *b){return b->returned;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 calls++;c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;
 c->result=number==SYS_fcntl?(mode==1?-EMFILE:7):(mode==4?-EINTR:0);
 atomic_store(&c->entered,1);atomic_store(&c->returned,1);return c->result;
}
int main(void){
 sl_linux_call producer={.result=0};atomic_init(&producer.entered,1);atomic_init(&producer.returned,1);
 sl_linux_deadline_sequence sequence={0};
 for(mode=0;mode<5;mode++){
  calls=begins=ends=0;sl_linux_deadline_fd_lift o={0};
  long result=sl_linux_lift_deadline_fd(&o,&producer,&sequence);
  assert(o.returned&&o.producer_original==&producer&&o.lift.original_fd==0);
  if(mode==0){assert(result==7&&calls==2&&begins==2&&ends==2);assert(sl_linux_deadline_fd_lift_matches_original(&o));o.lift.duplicate.arguments[2]=4;assert(!sl_linux_deadline_fd_lift_matches_original(&o));o.lift.duplicate.arguments[2]=3;}
  if(mode==1)assert(result==-EMFILE&&calls==1&&o.lift.held_fd==-1);
  if(mode==2)assert(result==-ETIMEDOUT&&calls==1&&o.lift.held_fd==7&&!atomic_load(&o.lift.close_original.entered));
  if(mode==3)assert(result==-EIO&&calls==2&&o.lift.held_fd==7&&o.lift.close_original.result==0);
  if(mode==4)assert(result==-EINTR&&calls==2&&o.lift.held_fd==7);
  sl_linux_deadline_fd_lift saved=o;
  assert(sl_linux_lift_deadline_fd(&o,&producer,&sequence)<0&&!memcmp(&o,&saved,sizeof(o)));
 }
 calls=0;producer.result=9;sl_linux_deadline_fd_lift o={0};
 assert(sl_linux_lift_deadline_fd(&o,&producer,&sequence)==9&&!calls&&o.lift.held_fd==9);
 assert(sl_linux_deadline_fd_lift_matches_original(&o));
 memset(&o,0,sizeof(o));sl_linux_deadline_fd_lift saved=o;
 assert(sl_linux_lift_deadline_fd(&o,(sl_linux_call *)&o,&sequence)<0&&!memcmp(&o,&saved,sizeof(o)));
 return 0;
}
