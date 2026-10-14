#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "deadline_linux.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <sys/syscall.h>
_Static_assert(sizeof(time_t)==8&&sizeof(long)==8&&sizeof(struct timespec)==16,"selected native clock ABI");
_Static_assert(offsetof(struct timespec,tv_nsec)==8,"selected native timespec offset");
static int deadline_disjoint(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int deadline_fresh(const void *p,size_t n){
 const unsigned char *b=p;for(size_t i=0;i<n;i++)if(b[i])return 0;return 1;
}
static int deadline_time(const struct timespec *t){return t->tv_sec>=0&&t->tv_nsec>=0&&t->tv_nsec<1000000000L;}
static int deadline_root(const sl_linux_deadline *d){
 if(!d||!d->returned||d->result||!d->attempted||!d->source_original||
    !d->operation_original||!d->operation_extent||!d->duration_ns||
    !deadline_disjoint(d,sizeof(*d),d->source_original,sizeof(*d->source_original))||
    !deadline_disjoint(d,sizeof(*d),d->operation_original,d->operation_extent)||
    d->source_original->operation_original!=d->operation_original||
    d->source_original->operation_extent!=d->operation_extent||
    d->source_original->duration_ns!=d->duration_ns||
    !deadline_time(&d->started)||!deadline_time(&d->end)||
    d->raw_clock||!atomic_load(&d->clock.entered)||!atomic_load(&d->clock.returned)||d->clock.result||
    d->clock.number!=SYS_clock_gettime||d->clock.arguments[0]!=CLOCK_MONOTONIC||
    d->clock.arguments[1]!=(long)&d->started||d->clock.arguments[2]||d->clock.arguments[3]||d->clock.arguments[4]||d->clock.arguments[5])return 0;
 unsigned long seconds=d->duration_ns/1000000000UL;
 unsigned long nanos=(unsigned long)d->started.tv_nsec+d->duration_ns%1000000000UL;
 unsigned long carry=nanos/1000000000UL;
 if(seconds>(unsigned long)LONG_MAX-(unsigned long)d->started.tv_sec||
    carry>(unsigned long)LONG_MAX-(unsigned long)d->started.tv_sec-seconds)return 0;
 return d->end.tv_sec==d->started.tv_sec+(long)seconds+(long)carry&&d->end.tv_nsec==(long)(nanos%1000000000UL);
}
long sl_linux_construct_deadline(sl_linux_deadline *o,const sl_linux_deadline_source *s){
 if(!o||!s||!deadline_disjoint(o,sizeof(*o),s,sizeof(*s))||
    !s->operation_original||!s->operation_extent||!s->duration_ns||
    !deadline_disjoint(o,sizeof(*o),s->operation_original,s->operation_extent)||!deadline_fresh(o,sizeof(*o)))return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->source_original=s;
 o->operation_original=s->operation_original;o->operation_extent=s->operation_extent;o->duration_ns=s->duration_ns;
 atomic_init(&o->clock.entered,0);atomic_init(&o->clock.returned,0);
 o->raw_clock=sl_linux_call6(&o->clock,"deadline.original.monotonic",SYS_clock_gettime,CLOCK_MONOTONIC,(long)&o->started,0,0,0,0);
 if(o->raw_clock){o->result=o->raw_clock<0?o->raw_clock:-EINVAL;goto done;}
 if(!atomic_load(&o->clock.entered)||!atomic_load(&o->clock.returned)||o->clock.result||!deadline_time(&o->started))goto done;
 unsigned long seconds=o->duration_ns/1000000000UL,nanos=o->duration_ns%1000000000UL;
 unsigned long end_nanos=(unsigned long)o->started.tv_nsec+nanos;
 unsigned long carry=end_nanos/1000000000UL;
 if(seconds>(unsigned long)LONG_MAX-(unsigned long)o->started.tv_sec||
    carry>(unsigned long)LONG_MAX-(unsigned long)o->started.tv_sec-seconds)goto done;
 o->end.tv_sec=o->started.tv_sec+(long)seconds+(long)carry;o->end.tv_nsec=(long)(end_nanos%1000000000UL);o->result=0;
done:o->returned=1;if(!o->result&&!deadline_root(o))o->result=-EINVAL;return o->result;
}
long sl_linux_observe_deadline(sl_linux_deadline_observation *o){
 if(!o||!o->deadline_original)return -EINVAL;
 const sl_linux_deadline *d=o->deadline_original;
 /* Preserve the enrolled input pointer; all other output bytes must be fresh.
  * Reject overlap before attempted/actual capture or the native output write. */
 size_t offset=offsetof(sl_linux_deadline_observation,actual_deadline);
 if(!deadline_disjoint(o,sizeof(*o),d,sizeof(*d))||
    !deadline_fresh((const unsigned char*)o+offset,sizeof(*o)-offset)||
    !deadline_root(d)||
    !deadline_disjoint(o,sizeof(*o),d->source_original,sizeof(*d->source_original))||
    !deadline_disjoint(o,sizeof(*o),d->operation_original,d->operation_extent))return -EINVAL;
 if(d->sequence_original){
  const sl_linux_deadline_sequence *s=d->sequence_original;
  if(!s->prepared||!s->attempted||s->actual_root!=d||s->root_original!=d||
     s->actual_observations!=s->observations_original||s->actual_capacity!=s->capacity||
     !s->actual_observations||!s->actual_capacity||s->actual_capacity>SIZE_MAX/sizeof(*o)||
     !s->next||s->next>s->actual_capacity||
     !deadline_disjoint(s,sizeof(*s),s->actual_observations,s->actual_capacity*sizeof(*o))||
     o!=&s->actual_observations[s->next-1])return -EINVAL;
 }
 o->attempted=1;o->result=-EINVAL;o->actual_deadline=d;
 atomic_init(&o->clock.entered,0);atomic_init(&o->clock.returned,0);
 o->raw_clock=sl_linux_call6(&o->clock,"deadline.observation.monotonic",SYS_clock_gettime,CLOCK_MONOTONIC,(long)&o->observed,0,0,0,0);
 if(o->raw_clock){o->result=o->raw_clock<0?o->raw_clock:-EINVAL;goto done;}
 if(!atomic_load(&o->clock.entered)||!atomic_load(&o->clock.returned)||o->clock.result||!deadline_time(&o->observed)||!deadline_root(d))goto done;
 if(o->observed.tv_sec<d->started.tv_sec||(o->observed.tv_sec==d->started.tv_sec&&o->observed.tv_nsec<d->started.tv_nsec))goto done;
 if(o->observed.tv_sec>d->end.tv_sec||(o->observed.tv_sec==d->end.tv_sec&&o->observed.tv_nsec>=d->end.tv_nsec)){
  o->expired=1;o->result=1;goto done;
 }
 o->remaining.tv_sec=d->end.tv_sec-o->observed.tv_sec;
 o->remaining.tv_nsec=d->end.tv_nsec-o->observed.tv_nsec;
 if(o->remaining.tv_nsec<0){o->remaining.tv_sec--;o->remaining.tv_nsec+=1000000000L;}
 o->result=0;
done:o->returned=1;return o->result;
}
long sl_linux_prepare_deadline_sequence(sl_linux_deadline_sequence *s){
 if(!s||!s->root_original||!s->observations_original||!s->capacity||
    s->capacity>SIZE_MAX/sizeof(*s->observations_original))return -EINVAL;
 size_t bytes=s->capacity*sizeof(*s->observations_original);
 size_t offset=offsetof(sl_linux_deadline_sequence,actual_root);
 sl_linux_deadline *d=s->root_original;
 if(!deadline_fresh((const unsigned char*)s+offset,sizeof(*s)-offset)||
    !deadline_disjoint(s,sizeof(*s),s->root_original,sizeof(*s->root_original))||
    !deadline_disjoint(s,sizeof(*s),s->observations_original,bytes)||
    !deadline_disjoint(s->root_original,sizeof(*s->root_original),s->observations_original,bytes)||
    !deadline_fresh(s->observations_original,bytes)||!deadline_root(d)||d->sequence_original||
    !deadline_disjoint(s,sizeof(*s),d->source_original,sizeof(*d->source_original))||
    !deadline_disjoint(s,sizeof(*s),d->operation_original,d->operation_extent)||
    !deadline_disjoint(s->observations_original,bytes,d->source_original,sizeof(*d->source_original))||
    !deadline_disjoint(s->observations_original,bytes,d->operation_original,d->operation_extent))return -EINVAL;
 s->attempted=1;s->actual_root=s->root_original;
 s->actual_observations=s->observations_original;s->actual_capacity=s->capacity;
 s->prepared=1;d->sequence_original=s;return 0;
 /* Containing storage precedes root construction. Sequence enrollment follows
  * that actual operation-boundary clock, once per root, and never restarts it. */
}
long sl_linux_observe_next_deadline(sl_linux_deadline_sequence *s){
 if(!s||!s->prepared||!s->attempted||s->failed||s->expired||s->exhausted||
    s->actual_root!=s->root_original||s->actual_observations!=s->observations_original||
    s->actual_capacity!=s->capacity||!s->actual_capacity||
    s->actual_capacity>SIZE_MAX/sizeof(*s->actual_observations)||s->next>s->actual_capacity)return -EINVAL;
 sl_linux_deadline *d=s->actual_root;
 size_t bytes=s->actual_capacity*sizeof(*s->actual_observations);
 if(!deadline_root(d)||d->sequence_original!=s||!deadline_disjoint(s,sizeof(*s),d,sizeof(*d))||
    !deadline_disjoint(s,sizeof(*s),s->actual_observations,bytes)||
    !deadline_disjoint(s->actual_observations,bytes,d,sizeof(*d))||
    !deadline_disjoint(s,sizeof(*s),d->source_original,sizeof(*d->source_original))||
    !deadline_disjoint(s,sizeof(*s),d->operation_original,d->operation_extent)||
    !deadline_disjoint(s->actual_observations,bytes,d->source_original,sizeof(*d->source_original))||
    !deadline_disjoint(s->actual_observations,bytes,d->operation_original,d->operation_extent))return -EINVAL;
 if(s->next==s->actual_capacity){s->exhausted=1;s->result=-ENOSPC;return s->result;}
 sl_linux_deadline_observation *previous=s->next?&s->actual_observations[s->next-1]:0;
 if(previous&&(!previous->attempted||!previous->returned||previous->result||previous->expired||
    previous->deadline_original!=d||previous->actual_deadline!=d||
    !atomic_load(&previous->clock.entered)||!atomic_load(&previous->clock.returned)||
    previous->raw_clock||previous->clock.result||!deadline_time(&previous->observed)||
    previous->clock.number!=SYS_clock_gettime||previous->clock.arguments[0]!=CLOCK_MONOTONIC||
    previous->clock.arguments[1]!=(long)&previous->observed||previous->clock.arguments[2]||
    previous->clock.arguments[3]||previous->clock.arguments[4]||previous->clock.arguments[5]))return -EINVAL;
 sl_linux_deadline_observation *o=&s->actual_observations[s->next];
 if(!deadline_fresh(o,sizeof(*o)))return -EINVAL;
 o->deadline_original=d;s->next++;
 s->result=sl_linux_observe_deadline(o);
 if(!s->result&&previous&&(o->observed.tv_sec<previous->observed.tv_sec||
    (o->observed.tv_sec==previous->observed.tv_sec&&o->observed.tv_nsec<previous->observed.tv_nsec))){
  o->result=-EINVAL;s->result=-EINVAL;
 }
 if(s->result<0){s->failed=1;s->first_failure=o;}
 else if(s->result==1)s->expired=1;
 return s->result;
 /* All prior rows remain immutable. next selects a source-owned occurrence,
  * never a supplied predecessor. No retry/renewal/retirement follows. */
}
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *s,const void *out,size_t extent){
 if(!out||!extent||!s||!s->prepared||!s->attempted||s->actual_root!=s->root_original||
    !s->actual_root||s->actual_observations!=s->observations_original||
    !s->actual_observations||s->actual_capacity!=s->capacity||!s->actual_capacity||
    s->actual_capacity>SIZE_MAX/sizeof(*s->actual_observations)||!deadline_root(s->actual_root))return 0;
 const sl_linux_deadline *d=s->actual_root;
 return d->sequence_original==s&&deadline_disjoint(out,extent,s,sizeof(*s))&&
 deadline_disjoint(out,extent,d,sizeof(*d))&&
 deadline_disjoint(out,extent,d->source_original,sizeof(*d->source_original))&&
 deadline_disjoint(out,extent,d->operation_original,d->operation_extent)&&
 deadline_disjoint(out,extent,s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations));
}
static int deadline_boundary_source(const sl_linux_deadline_boundary *b,const sl_linux_deadline_sequence *s){
 return b&&sl_linux_deadline_sequence_excludes_output(s,b,sizeof(*b));
}
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){
 if(!deadline_boundary_source(b,s)||!deadline_fresh(b,sizeof(*b))||
    s->failed||s->expired||s->exhausted||s->next>s->actual_capacity||s->actual_capacity-s->next<2)return -EINVAL;
 /* Both actual rows must be available before the caller issues a native effect. */
 if(!deadline_fresh(&s->actual_observations[s->next],2*sizeof(*s->actual_observations)))return -EINVAL;
 b->attempted=1;b->sequence=s;b->first=s->next;
 b->before=&s->actual_observations[b->first];b->after=&s->actual_observations[b->first+1];
 b->before_result=sl_linux_observe_next_deadline(s);
 if(b->before_result)return b->before_result;
 if(!deadline_boundary_source(b,s)||s->next!=b->first+1||!b->before->returned||b->before->result)return -EINVAL;
 b->ready=1;return 0;
}
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *b){
 if(!b||!b->attempted||!b->ready||b->after_attempted||b->returned||b->before_result||
    !deadline_boundary_source(b,b->sequence))return -EINVAL;
 sl_linux_deadline_sequence *s=b->sequence;
 if(b->first>=s->actual_capacity||s->actual_capacity-b->first<2||s->next!=b->first+1||
    b->before!=&s->actual_observations[b->first]||b->after!=&s->actual_observations[b->first+1]||
    !b->before->returned||b->before->result||!deadline_fresh(b->after,sizeof(*b->after)))return -EINVAL;
 b->after_attempted=1;b->after_result=sl_linux_observe_next_deadline(s);b->returned=1;
 return b->after_result;
 /* Native operation outputs are separately retained by the owning caller even
  * when this observation expires/fails. Neither result is terminal authority. */
}
static int deadline_boundary_observation(const sl_linux_deadline_observation *o,const sl_linux_deadline *d){
 if(!o->attempted||!o->returned||o->result||o->expired||o->deadline_original!=d||o->actual_deadline!=d||
    o->raw_clock||!atomic_load(&o->clock.entered)||!atomic_load(&o->clock.returned)||o->clock.result||
    o->clock.number!=SYS_clock_gettime||o->clock.arguments[0]!=CLOCK_MONOTONIC||
    o->clock.arguments[1]!=(long)&o->observed||o->clock.arguments[2]||o->clock.arguments[3]||
    o->clock.arguments[4]||o->clock.arguments[5]||!deadline_time(&o->observed)||
    o->observed.tv_sec<d->started.tv_sec||(o->observed.tv_sec==d->started.tv_sec&&o->observed.tv_nsec<d->started.tv_nsec)||
    o->observed.tv_sec>d->end.tv_sec||(o->observed.tv_sec==d->end.tv_sec&&o->observed.tv_nsec>=d->end.tv_nsec))return 0;
 struct timespec r={d->end.tv_sec-o->observed.tv_sec,d->end.tv_nsec-o->observed.tv_nsec};
 if(r.tv_nsec<0){r.tv_sec--;r.tv_nsec+=1000000000L;}
 return o->remaining.tv_sec==r.tv_sec&&o->remaining.tv_nsec==r.tv_nsec;
}
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *b){
 if(!b||!b->attempted||!b->ready||!b->after_attempted||!b->returned||b->before_result||b->after_result||
    !deadline_boundary_source(b,b->sequence))return 0;
 const sl_linux_deadline_sequence *s=b->sequence;
 if(b->first>=s->actual_capacity||s->actual_capacity-b->first<2||s->next<b->first+2||s->next>s->actual_capacity||
    b->before!=&s->actual_observations[b->first]||b->after!=&s->actual_observations[b->first+1]||
    !deadline_boundary_observation(b->before,s->actual_root)||!deadline_boundary_observation(b->after,s->actual_root))return 0;
 if(b->first){
  const sl_linux_deadline_observation *previous=&s->actual_observations[b->first-1];
  if(!deadline_boundary_observation(previous,s->actual_root)||
     b->before->observed.tv_sec<previous->observed.tv_sec||
     (b->before->observed.tv_sec==previous->observed.tv_sec&&b->before->observed.tv_nsec<previous->observed.tv_nsec))return 0;
 }
 return b->after->observed.tv_sec>b->before->observed.tv_sec||
 (b->after->observed.tv_sec==b->before->observed.tv_sec&&b->after->observed.tv_nsec>=b->before->observed.tv_nsec);
}
#endif
