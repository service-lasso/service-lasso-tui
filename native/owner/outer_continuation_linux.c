#if defined(__linux__) && defined(__x86_64__)
#include "outer_birth_linux.h"
#include <stdint.h>
#include <errno.h>
long sl_linux_enter_outer_continuation(sl_linux_outer_continuation *o){
 if(!o||!o->birth_original)return -EINVAL;
 sl_linux_outer_birth *b=o->birth_original;
 uintptr_t x=(uintptr_t)o,y=(uintptr_t)b;
 if(sizeof(*o)>UINTPTR_MAX-x||sizeof(*b)>UINTPTR_MAX-y||(x<y+sizeof(*b)&&y<x+sizeof(*o)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char*)o;
 for(size_t i=offsetof(sl_linux_outer_continuation,actual_birth);i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 if(b->prepared!=1||b->attempted||atomic_load(&b->entered)||atomic_load(&b->parent_captured)||atomic_load(&b->child_captured))return -EINVAL;
 atomic_init(&o->parent.published,0);atomic_init(&o->child.published,0);
 o->actual_birth=b;o->attempted=1;
 long raw=sl_linux_enter_outer_birth(b);
 /* This native return selects the sole writer row. No common postbirth
  * returned/result cell is shared between the two racing continuations. */
 sl_linux_outer_return *r=raw==0?&o->child:&o->parent;
 r->raw=raw;r->result=-EINVAL;r->captured=1;
 if(o->actual_birth!=b||o->birth_original!=b||!b->attempted||!atomic_load_explicit(&b->entered,memory_order_acquire))goto publish;
 if(raw==0){
  if(atomic_load_explicit(&b->child_captured,memory_order_acquire)&&b->raw_child==raw){r->matched=1;r->result=0;}
 }else if(atomic_load_explicit(&b->parent_captured,memory_order_acquire)&&b->raw_parent==raw){
  r->matched=1;r->result=raw<0?raw:0;
 }
 publish:atomic_store_explicit(&r->published,1,memory_order_release);
 return raw;
 /* Matching is return correspondence only. Only the genuine parent branch
  * with matched successful parent return may later adopt its actual pidfd;
  * failure, child and unmatched paths provide no observation/exit authority. */
}
#endif
