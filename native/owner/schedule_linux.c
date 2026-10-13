#if defined(__linux__) && defined(__x86_64__)
#include "schedule_linux.h"
#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <sys/syscall.h>
_Static_assert(sizeof(uint32_t)==4&&sizeof(unsigned)==4,"futex exact32-bit words");
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"source scheduler lockfree publication");
_Static_assert(_Alignof(_Atomic uint32_t)>=4,"futex word alignment");
static int schedule_row(const void *p,const void *base,unsigned long count,unsigned long width){uintptr_t a=(uintptr_t)p,b=(uintptr_t)base;if(count>UINTPTR_MAX/width||count*width>UINTPTR_MAX-b)return 0;return a>=b&&a-b<count*width&&(a-b)%width==0;}
long sl_linux_prepare_schedule(sl_linux_schedule *s){
 if(!s||s->prepare_attempted)return -EINVAL;s->prepare_attempted=1;
 s->actual_wakes=s->wakes;s->actual_wake_capacity=s->wake_capacity;s->actual_waits=s->waits;s->actual_wait_capacity=s->wait_capacity;s->actual_interval_ns=s->interval_ns;
 if(!s->controller_original||!s->actual_wakes||!s->actual_waits||!s->actual_wake_capacity||s->actual_wake_capacity>UINT32_MAX||!s->actual_wait_capacity||!s->actual_interval_ns||(uintptr_t)&s->generation%4)return -EINVAL;
 if(s->actual_wake_capacity>UINTPTR_MAX/sizeof(*s->actual_wakes)||s->actual_wait_capacity>UINTPTR_MAX/sizeof(*s->actual_waits))return -EINVAL;
 uintptr_t w=(uintptr_t)s->actual_wakes,v=(uintptr_t)s->actual_waits;unsigned long wb=s->actual_wake_capacity*sizeof(*s->actual_wakes),vb=s->actual_wait_capacity*sizeof(*s->actual_waits);
 if(wb>UINTPTR_MAX-w||vb>UINTPTR_MAX-v||sizeof(*s)>UINTPTR_MAX-(uintptr_t)s||(w<v+vb&&v<w+wb)||(w<(uintptr_t)s+sizeof(*s)&&(uintptr_t)s<w+wb)||(v<(uintptr_t)s+sizeof(*s)&&(uintptr_t)s<v+vb))return -EINVAL;
 for(unsigned long i=0;i<s->actual_wake_capacity;i++)if(s->actual_wakes[i].schedule_original!=s||!s->actual_wakes[i].source_original||s->actual_wakes[i].call.source)return -EINVAL;
 for(unsigned long i=0;i<s->actual_wait_capacity;i++)if(s->actual_waits[i].schedule_original!=s||s->actual_waits[i].call.source||s->actual_waits[i].observed||s->actual_waits[i].returned)return -EINVAL;
 atomic_init(&s->generation,0);
 for(unsigned long i=0;i<s->actual_wake_capacity;i++){sl_linux_schedule_wake *o=&s->actual_wakes[i];atomic_init(&o->claimed,0);atomic_init(&o->returned,0);atomic_init(&o->call.entered,0);atomic_init(&o->call.returned,0);}
 for(unsigned long i=0;i<s->actual_wait_capacity;i++){sl_linux_schedule_wait *o=&s->actual_waits[i];atomic_init(&o->claimed,0);atomic_init(&o->call.entered,0);atomic_init(&o->call.returned,0);}
 s->interval.tv_sec=(time_t)(s->actual_interval_ns/1000000000UL);s->interval.tv_nsec=(long)(s->actual_interval_ns%1000000000UL);s->prepared=1;return 0;
}
long sl_linux_schedule_notify(sl_linux_schedule_wake *o){
 if(!o||!o->schedule_original)return -EINVAL;sl_linux_schedule *s=o->schedule_original;
 if(!s->prepared||!schedule_row(o,s->actual_wakes,s->actual_wake_capacity,sizeof(*o)))return -EINVAL;
 unsigned expected=0;if(!atomic_compare_exchange_strong(&o->claimed,&expected,1))return -EINVAL;
 /* Fixed once-claimed roster <=UINT32_MAX is the actual nonwrap bound.
  * Publication notification is not completion of this native wake call. */
 o->generation_before=atomic_fetch_add_explicit(&s->generation,1,memory_order_release);o->generation_after=o->generation_before+1;
 o->result=sl_linux_call6(&o->call,"controller.schedule.private-wake",SYS_futex,(long)&s->generation,FUTEX_WAKE_PRIVATE,INT_MAX,0,0,0);
 atomic_store_explicit(&o->returned,1,memory_order_release);return o->result;
}
/* Only actual successful native-join callers consume this occurrence. This
 * validates the SAME notification original; it grants no task/native custody. */
long sl_linux_schedule_joined_notification(sl_linux_schedule_wake *o,const void *source){
 if(!o||!o->schedule_original||o->source_original!=source)return -EINVAL;
 sl_linux_schedule *s=o->schedule_original;
 if(!s->prepared||!schedule_row(o,s->actual_wakes,s->actual_wake_capacity,sizeof(*o))||
    !atomic_load_explicit(&o->returned,memory_order_acquire)||!atomic_load(&o->claimed)||
    !atomic_load_explicit(&o->call.returned,memory_order_acquire))return -EINVAL;
 return o->result<0?o->result:0;
}
long sl_linux_schedule_observe(sl_linux_schedule_wait *o){
 if(!o||!o->schedule_original)return -EINVAL;sl_linux_schedule *s=o->schedule_original;
 if(!s->prepared||!schedule_row(o,s->actual_waits,s->actual_wait_capacity,sizeof(*o)))return -EINVAL;
 unsigned expected=0;if(!atomic_compare_exchange_strong(&o->claimed,&expected,1))return -EINVAL;
 o->generation_before=atomic_load_explicit(&s->generation,memory_order_acquire);
 o->observed=1;return 0;
}
long sl_linux_schedule_block(sl_linux_schedule_wait *o){
 if(!o||!o->schedule_original)return -EINVAL;sl_linux_schedule *s=o->schedule_original;
 if(!s->prepared||!schedule_row(o,s->actual_waits,s->actual_wait_capacity,sizeof(*o))||!o->observed||o->returned||atomic_load(&o->call.entered))return -EINVAL;
 o->result=sl_linux_call6(&o->call,"controller.schedule.private-wait",SYS_futex,(long)&s->generation,FUTEX_WAIT_PRIVATE,o->generation_before,(long)&s->interval,0,0);
 o->generation_after=atomic_load_explicit(&s->generation,memory_order_acquire);o->returned=1;return o->result;
 /* Actual owner rescan/worker join is required regardless of this outcome.
  * No disposition, retry of an uncertain source or terminal hold is granted. */
}
#endif