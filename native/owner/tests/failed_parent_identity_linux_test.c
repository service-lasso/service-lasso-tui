/* Production acquisition and name bodies, synthetic syscalls/clocks, UNRUN. */
#include "../failed_parent_identity_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
static unsigned mode,calls,ends;
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *s,const void *p,size_t n){return s&&p&&n;}
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){b->sequence=s;b->attempted=b->ready=1;return 0;}
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *b){b->after_attempted=b->returned=1;ends++;return (mode==1&&ends==1)||(mode==2&&ends==2)?1:0;}
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *b){return b->returned;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 calls++;c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;
 c->result=number==SYS_getpid?123:mode==3?-EIO:0;
 if(number==SYS_clock_gettime&&!c->result){struct timespec *instant=(struct timespec *)b;instant->tv_sec=5;instant->tv_nsec=6;}
 atomic_store(&c->entered,1);atomic_store(&c->returned,1);return c->result;
}
int main(void){
 unsigned char identity[32]={0};sl_linux_outer_birth birth={.prepared=1};
 sl_linux_deadline_sequence sequence={0};
 for(mode=0;mode<4;mode++){
  calls=ends=0;sl_linux_failed_parent_identity o={0};
  long result=sl_linux_acquire_failed_parent_identity(&o,identity,32,&birth,&sequence);
  assert(o.returned&&o.process_raw==123&&o.birth_original==&birth);
  if(mode==0)assert(!result&&calls==2&&o.name.returned&&!o.name.result&&o.name.process_original==&o.process&&o.name.clock_original==&o.clock);
  if(mode==1)assert(result==-ETIMEDOUT&&calls==1&&!o.name.attempted&&!atomic_load(&o.clock.entered));
  if(mode==2)assert(result==-ETIMEDOUT&&calls==2&&o.instant.tv_sec==5&&!o.name.attempted);
  if(mode==3)assert(result<0&&calls==2&&o.clock_raw==-EIO&&!o.name.attempted);
  sl_linux_failed_parent_identity saved=o;
  assert(sl_linux_acquire_failed_parent_identity(&o,identity,32,&birth,&sequence)<0&&!memcmp(&o,&saved,sizeof(o)));
 }
 sl_linux_failed_parent_identity o={0};birth.attempted=1;calls=0;
 assert(sl_linux_acquire_failed_parent_identity(&o,identity,32,&birth,&sequence)<0&&!o.attempted&&!calls);
 birth.attempted=0;sl_linux_failed_parent_identity saved=o;
 assert(sl_linux_acquire_failed_parent_identity(&o,(unsigned char *)&o,32,&birth,&sequence)<0&&!memcmp(&o,&saved,sizeof(o)));
 return 0;
}
