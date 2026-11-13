/* UNRUN failed-before-birth caller cut; substituted close is not OS proof. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned calls;static int mode;static int closed[4];
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;assert(number==SYS_close);closed[calls++]=(int)a0;c->source=source;c->number=number;atomic_store(&c->entered,1);c->result=mode==0?(calls==1?-EINTR:-EIO):0;atomic_store(&c->returned,1);return c->result;}
#include "../candidate_target_close_linux.c"
typedef struct {sl_linux_public_controller c;sl_linux_public_startup s;} fixture;
static void opened(sl_linux_call *c,int fd){c->source="fixture.actual-open";c->number=SYS_openat2;c->result=fd;atomic_init(&c->entered,1);atomic_init(&c->returned,1);}
static void prepare(fixture *f){f->c.attempted=1;f->c.construction_result=-77;f->c.startup_original=&f->s;f->s.controller_original=&f->c;f->s.candidate_preexec_returned=1;f->s.candidate_preexec_result=-77;sl_linux_candidate_target *o=&f->s.candidate_target;o->attempted=o->returned=1;o->result=-88;o->raw_target_fd=o->target_fd=40;o->raw_procs_fd=o->procs_fd=41;opened(&o->open_target,40);opened(&o->open_procs,41);o->target_lift.attempted=o->procs_lift.attempted=1;o->target_lift.original_fd=o->target_lift.held_fd=40;o->procs_lift.original_fd=o->procs_lift.held_fd=41;}
int main(void){fixture cases[5]={0};for(mode=0;mode<5;mode++){
 fixture *f=&cases[mode];prepare(f);sl_linux_candidate_target *o=&f->s.candidate_target;calls=0;
 if(mode==1){opened(&o->open_target,0);o->raw_target_fd=0;o->target_lift.original_fd=0;opened(&o->target_lift.duplicate,40);o->target_lift.duplicate.number=SYS_fcntl;o->target_lift.close_original.result=-EINTR;atomic_init(&o->target_lift.close_original.entered,1);atomic_init(&o->target_lift.close_original.returned,1);}
 if(mode==2)atomic_store(&o->open_target.returned,0);
 if(mode==3)f->c.entry.birth.attempted=1;
 if(mode==4){f->s.initial_family.attempted=1;f->s.initial_family.returned=0;}
 long r=sl_linux_close_failed_target_before_birth(&f->c);
 if(mode<3){assert(o->closes_attempted&&o->closes_returned);if(!mode)assert(r==-EINTR&&calls==2&&closed[0]==40&&closed[1]==41&&o->closes[1].close_held.result==-EIO&&o->close_failure_original==&o->closes[0]);if(mode==1)assert(r==-EINTR&&calls==2&&closed[0]==40&&closed[1]==41&&!o->closes[0].raw_attempted&&o->closes[0].failure_original==&o->target_lift.close_original);if(mode==2)assert(r==-EINVAL&&calls==1&&closed[0]==41&&o->closes[0].unsettled_original==&o->open_target&&o->closes[0].returned);unsigned before=calls;assert(sl_linux_close_failed_target_before_birth(&f->c)==-EINVAL&&calls==before);}
 else assert(r==-EINVAL&&!calls&&!o->closes_attempted);
 assert(f->c.construction_result==-77&&f->s.candidate_preexec_result==-77&&o->result==-88);
 }return 0;}