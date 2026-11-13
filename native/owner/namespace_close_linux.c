#if defined(__linux__) && defined(__x86_64__)
#include "namespace_parent_linux.h"
#include <sys/syscall.h>
#include <limits.h>
static void namespace_close_one(sl_linux_parent_namespace *o,sl_linux_call *cell,unsigned *attempted,int fd,const char *source){
 *attempted=1;
 atomic_init(&cell->entered,0);atomic_init(&cell->returned,0);
 long result=sl_linux_call6(cell,source,SYS_close,fd,0,0,0,0,0);
 if(result&&!o->close_primary)o->close_primary=cell;
}
long sl_linux_close_namespace_views(sl_linux_parent_namespace *o){
 if(!o||!o->attempted||!o->returned||o->closes_attempted)return -1;
 /* This is not an eligibility decision. The connected final disposition
  * caller is still required to settle all future/profile borrowers first. */
 o->closes_attempted=1;o->close_result=-1;
 for(unsigned i=0;i<6;i++){
  sl_linux_namespace_view *v=&o->views[i];
  if(v->close_held_attempted||v->close_raw_attempted||v->close_held.source||v->close_held.number||
     v->close_raw.source||v->close_raw.number||atomic_load(&v->close_held.entered)||atomic_load(&v->close_raw.entered)||atomic_load(&v->close_held.returned)||atomic_load(&v->close_raw.returned))return -1;
 }
 for(unsigned i=0;i<6;i++){
  sl_linux_namespace_view *v=&o->views[i];
  if(!atomic_load_explicit(&v->acquire.returned,memory_order_acquire)||v->acquire.result<0||v->acquire.result>INT_MAX)continue;
  int raw=(int)v->acquire.result,held=-1;
  if(v->raw_fd!=raw){if(!o->close_unmapped_original)o->close_unmapped_original=v;continue;}
  if(v->lift.attempted&&v->lift.original_fd==raw&&v->lift.held_fd>=3){
   if(raw>=3&&v->lift.held_fd==raw)held=raw;
   else if(raw<3&&atomic_load_explicit(&v->lift.duplicate.returned,memory_order_acquire)&&
           v->lift.duplicate.result==v->lift.held_fd)held=v->lift.held_fd;
  }
  if(raw!=held&&atomic_load_explicit(&v->lift.close_original.entered,memory_order_acquire)){
   if(!atomic_load_explicit(&v->lift.close_original.returned,memory_order_acquire)){if(!o->close_unsettled_original)o->close_unsettled_original=&v->lift.close_original;}
   else if(v->lift.close_original.result&&!o->close_primary)o->close_primary=&v->lift.close_original;
  }
  if(held>=3)namespace_close_one(o,&v->close_held,&v->close_held_attempted,held,"controller.namespace.held-view.close-once");
  /* An entered original close is absorbing even if its result is uncertain.
   * A successful duplicate and failed original close still has held!=raw. */
  if(raw!=held&&!atomic_load_explicit(&v->lift.close_original.entered,memory_order_acquire))
   namespace_close_one(o,&v->close_raw,&v->close_raw_attempted,raw,"controller.namespace.raw-view.close-once");
 }
 o->closes_returned=1;
 if(o->close_primary)o->close_result=o->close_primary->result<0?o->close_primary->result:-1;
 else o->close_result=(o->close_unmapped_original||o->close_unsettled_original)?-1:0;
 return o->close_result;
}
#endif