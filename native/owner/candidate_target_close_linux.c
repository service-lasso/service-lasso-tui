#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <sys/syscall.h>
static void failed_target_close(sl_linux_target_close_view *v,sl_linux_call *c,unsigned *attempted,int fd,const char *source){
 *attempted=1;atomic_init(&c->entered,0);atomic_init(&c->returned,0);
 long r=sl_linux_call6(c,source,SYS_close,fd,0,0,0,0,0);if(r&&!v->failure_original)v->failure_original=c;
}
static void failed_target_view(sl_linux_target_close_view *v,sl_linux_call *open,sl_linux_fd_lift *lift,int recorded_raw){
 v->attempted=1;v->open_original=open;v->lift_original=lift;v->raw_fd=v->held_fd=-1;v->result=0;
 if(!open->source)goto done;
 if(open->number!=SYS_openat2||!atomic_load_explicit(&open->returned,memory_order_acquire)||open->result>INT_MAX){v->unsettled_original=open;goto recorded;}
 if(open->result<0)goto done;
 int raw=(int)open->result;v->raw_fd=raw;if(raw!=recorded_raw){v->unmapped=1;goto recorded;}
 if(lift->attempted){
  if(lift->original_fd!=raw){v->unmapped=1;goto recorded;}
  if(raw>=3){if(lift->held_fd!=raw){v->unmapped=1;goto recorded;}v->held_fd=raw;}
  else if(atomic_load_explicit(&lift->duplicate.entered,memory_order_acquire)){
   if(!atomic_load_explicit(&lift->duplicate.returned,memory_order_acquire)){v->unsettled_original=&lift->duplicate;goto recorded;}
   if(lift->duplicate.result>=3&&lift->duplicate.result<=INT_MAX&&lift->duplicate.result==lift->held_fd)v->held_fd=lift->held_fd;
   else if(lift->duplicate.result>=0){v->unmapped=1;goto recorded;}
  }
 }
 if(raw!=v->held_fd&&lift->attempted&&atomic_load_explicit(&lift->close_original.entered,memory_order_acquire)){
  if(!atomic_load_explicit(&lift->close_original.returned,memory_order_acquire))v->unsettled_original=&lift->close_original;
  else if(lift->close_original.result)v->failure_original=&lift->close_original;
 }
 if(v->held_fd>=3)failed_target_close(v,&v->close_held,&v->held_attempted,v->held_fd,"failed-prebirth.target.held.close-once");
 if(raw!=v->held_fd&&(!lift->attempted||!atomic_load_explicit(&lift->close_original.entered,memory_order_acquire)))failed_target_close(v,&v->close_raw,&v->raw_attempted,raw,"failed-prebirth.target.raw.close-once");
 done:v->returned=1;
 recorded:v->returned=1;if(v->failure_original)v->result=v->failure_original->result<0?v->failure_original->result:-EINVAL;else if(v->unsettled_original||v->unmapped)v->result=-EINVAL;
}
long sl_linux_close_failed_target_before_birth(sl_linux_public_controller *c){
 if(!c||!c->attempted||!c->construction_result||!c->startup_original||c->startup_original->controller_original!=c||c->entry.attempted||c->entry.birth.attempted||c->family_active)return -EINVAL;
 sl_linux_public_startup *s=c->startup_original;sl_linux_candidate_target *o=&s->candidate_target;
 if(!s->candidate_preexec_returned||!s->candidate_preexec_result||(o->attempted&&!o->returned)||(s->initial_family.attempted&&!s->initial_family.returned)||o->closes_attempted)return -EINVAL;
 for(unsigned i=0;i<2;i++)if(o->closes[i].attempted||o->closes[i].close_raw.source||o->closes[i].close_held.source)return -EINVAL;
 o->closes_attempted=1;o->close_result=0;
 if(o->attempted){
  failed_target_view(&o->closes[0],&o->open_target,&o->target_lift,o->raw_target_fd);
  failed_target_view(&o->closes[1],&o->open_procs,&o->procs_lift,o->raw_procs_fd);
  for(unsigned i=0;i<2;i++)if(o->closes[i].result&&!o->close_failure_original){o->close_failure_original=&o->closes[i];o->close_result=o->closes[i].result;}
 }
 o->closes_returned=1;return o->close_result;
 /* This caller cut cannot release a postbirth control or dispose any graph. */
}
#endif