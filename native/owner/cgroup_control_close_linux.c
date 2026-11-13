#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <sys/syscall.h>
#include <limits.h>
static void control_close(sl_linux_cgroup_control_label *o,sl_linux_call *c,unsigned *attempted,int fd,const char *source){
 *attempted=1;atomic_init(&c->entered,0);atomic_init(&c->returned,0);
 long result=sl_linux_call6(c,source,SYS_close,fd,0,0,0,0,0);
 if(result&&!o->close_primary)o->close_primary=c;
}
long sl_linux_close_cgroup_controls(sl_linux_public_startup *s){
 if(!s||!s->attempted||!s->returned||!s->actual_controls||!s->actual_control_capacity||s->control_closes_attempted)return -1;
 /* Actual caller must first settle all control/policy/profile borrowers.
  * This full prepared roster includes skipped and partially entered slots. */
 s->control_closes_attempted=1;s->control_close_result=0;
 for(unsigned long i=0;i<s->actual_control_capacity;i++){
  sl_linux_cgroup_control_label *o=&s->actual_controls[i];
  if(!o->attempted)continue;
  if(!o->returned||o->closes_attempted||o->close_raw.source||o->close_held.source||o->close_raw.number||o->close_held.number){
   if(!s->control_close_primary){s->control_close_primary=o;s->control_close_result=-1;}continue;
  }
  o->closes_attempted=1;o->close_result=0;
  if(!o->open.source||o->open.number!=SYS_openat2)goto finished;
  if(!atomic_load_explicit(&o->open.returned,memory_order_acquire)||o->open.result>INT_MAX){o->close_unsettled=&o->open;o->close_result=-1;goto recorded;}
  if(o->open.result<0)goto finished;
  int raw=(int)o->open.result,held=-1;
  if(o->raw_fd!=raw){o->close_unmapped=1;o->close_result=-1;goto recorded;}
  if(o->lift.attempted&&o->lift.original_fd==raw&&o->lift.held_fd>=3){
   if(raw>=3&&o->lift.held_fd==raw)held=raw;
   else if(raw<3&&atomic_load_explicit(&o->lift.duplicate.returned,memory_order_acquire)&&o->lift.duplicate.result==o->lift.held_fd)held=o->lift.held_fd;
  }
  if(raw!=held&&o->lift.attempted&&atomic_load_explicit(&o->lift.close_original.entered,memory_order_acquire)){
   if(!atomic_load_explicit(&o->lift.close_original.returned,memory_order_acquire))o->close_unsettled=&o->lift.close_original;
   else if(o->lift.close_original.result)o->close_primary=&o->lift.close_original;
  }
  if(held>=3)control_close(o,&o->close_held,&o->close_held_attempted,held,"cgroup.control.held.close-once");
  if(raw!=held&&(!o->lift.attempted||!atomic_load_explicit(&o->lift.close_original.entered,memory_order_acquire)))control_close(o,&o->close_raw,&o->close_raw_attempted,raw,"cgroup.control.raw.close-once");
 finished:o->closes_returned=1;
  if(o->close_primary)o->close_result=o->close_primary->result<0?o->close_primary->result:-1;
  else if(o->close_unsettled)o->close_result=-1;
 recorded:if(o->close_result){if(!s->control_close_primary){s->control_close_primary=o;s->control_close_result=o->close_result;}}
 }
 s->control_closes_returned=1;return s->control_close_result;
}
#endif