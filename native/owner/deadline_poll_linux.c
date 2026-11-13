#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "deadline_poll_linux.h"
#include <stdint.h>
#include <errno.h>
#include <sys/syscall.h>
static int deadline_poll_disjoint(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int deadline_poll_fresh(const void *p,size_t n){
 const unsigned char *b=p;for(size_t i=0;i<n;i++)if(b[i])return 0;return 1;
}
long sl_linux_poll_original_deadline(sl_linux_deadline_poll *o){
 if(!o||!o->sequence_original||o->count>SIZE_MAX/sizeof(struct pollfd))return -EINVAL;
 sl_linux_deadline_sequence *s=o->sequence_original;
 sl_linux_deadline *d=s->root_original;
 if(!d||!d->source_original||!s->prepared||!s->attempted||s->failed||s->expired||s->exhausted||
    s->actual_root!=d||d->sequence_original!=s||s->actual_observations!=s->observations_original||
    !s->actual_observations||s->actual_capacity!=s->capacity||!s->actual_capacity||
    s->actual_capacity>SIZE_MAX/sizeof(*s->actual_observations)||s->next>s->actual_capacity)return -EINVAL;
 size_t roster=s->actual_capacity*sizeof(*s->actual_observations),bytes=o->count*sizeof(struct pollfd);
 if(!deadline_poll_fresh((unsigned char*)o+offsetof(sl_linux_deadline_poll,actual_sequence),sizeof(*o)-offsetof(sl_linux_deadline_poll,actual_sequence))||
    !deadline_poll_disjoint(o,sizeof(*o),s,sizeof(*s))||
    !deadline_poll_disjoint(o,sizeof(*o),d,sizeof(*d))||
    !deadline_poll_disjoint(o,sizeof(*o),d->source_original,sizeof(*d->source_original))||
    !deadline_poll_disjoint(o,sizeof(*o),s->actual_observations,roster))return -EINVAL;
 if(bytes){
  if(!deadline_poll_disjoint(o->outputs_original,bytes,o,sizeof(*o))||
     !deadline_poll_disjoint(o->outputs_original,bytes,o->descriptors_original,bytes)||
     !deadline_poll_disjoint(o->outputs_original,bytes,s,sizeof(*s))||
     !deadline_poll_disjoint(o->outputs_original,bytes,d,sizeof(*d))||
     !deadline_poll_disjoint(o->outputs_original,bytes,d->source_original,sizeof(*d->source_original))||
     !deadline_poll_disjoint(o->outputs_original,bytes,s->actual_observations,roster)||
     !deadline_poll_disjoint(o->descriptors_original,bytes,o,sizeof(*o))||
     !deadline_poll_disjoint(o->descriptors_original,bytes,s,sizeof(*s))||
     !deadline_poll_disjoint(o->descriptors_original,bytes,d,sizeof(*d))||
     !deadline_poll_disjoint(o->descriptors_original,bytes,d->source_original,sizeof(*d->source_original))||
     !deadline_poll_disjoint(o->descriptors_original,bytes,s->actual_observations,roster)||
     !deadline_poll_fresh(o->outputs_original,bytes))return -EINVAL;
 } /* With count0 the native kernel ignores both descriptor pointers; no
    * dereference/copy is permitted here and source backing remains retained. */
 o->attempted=1;o->result=-EINVAL;o->actual_sequence=s;
 o->actual_descriptors=o->descriptors_original;o->actual_outputs=o->outputs_original;o->actual_count=o->count;
 /* Two distinct clock rows must already fit; do not start a native wait if
  * its mandatory after observation has no retained output occurrence. */
 if(s->actual_capacity-s->next<2){o->result=-ENOSPC;goto done;}
 o->before=&s->actual_observations[s->next];
 o->before_result=sl_linux_observe_next_deadline(s);
 if(o->before_result){o->result=o->before_result;goto done;}
 if(!o->before->returned||o->before->result||o->before->actual_deadline!=d)goto done;
 o->requested_timeout=o->before->remaining;o->timeout=o->requested_timeout;
 o->after=&s->actual_observations[s->next];
 for(unsigned long i=0;i<o->actual_count;i++)o->actual_outputs[i]=o->actual_descriptors[i];
 atomic_init(&o->poll.entered,0);atomic_init(&o->poll.returned,0);
 o->raw_poll=sl_linux_call6(&o->poll,"deadline.operation.ppoll",SYS_ppoll,
  (long)o->actual_outputs,o->actual_count,(long)&o->timeout,0,0,0);
 o->poll_returned=1;
 o->after_result=sl_linux_observe_next_deadline(s);
 if(o->after_result){o->result=o->after_result;goto done;}
 if(!o->after->returned||o->after->result||o->after->actual_deadline!=d||
    o->sequence_original!=s||o->actual_sequence!=s||
    o->actual_descriptors!=o->descriptors_original||o->actual_outputs!=o->outputs_original||o->actual_count!=o->count||
    s->next<2||s->next>s->actual_capacity||o->after!=&s->actual_observations[s->next-1]||o->before!=o->after-1||
    o->requested_timeout.tv_sec!=o->before->remaining.tv_sec||o->requested_timeout.tv_nsec!=o->before->remaining.tv_nsec||
    !atomic_load(&o->poll.entered)||!atomic_load(&o->poll.returned)||o->poll.result!=o->raw_poll||
    o->poll.number!=SYS_ppoll||o->poll.arguments[0]!=(long)o->actual_outputs||
    o->poll.arguments[1]!=(long)o->actual_count||o->poll.arguments[2]!=(long)&o->timeout||
    o->poll.arguments[3]||o->poll.arguments[4]||o->poll.arguments[5])goto done;
 if(o->raw_poll<0){o->result=o->raw_poll;goto done;}
 if((unsigned long)o->raw_poll>o->actual_count)goto done;
 unsigned long ready=0;
 for(unsigned long i=0;i<o->actual_count;i++){
  if(o->actual_outputs[i].fd!=o->actual_descriptors[i].fd||o->actual_outputs[i].events!=o->actual_descriptors[i].events)goto done;
  if(o->actual_outputs[i].revents)ready++;
 }
 if(ready!=(unsigned long)o->raw_poll)goto done;
 o->result=0;o->timely=1;
done:o->returned=1;return o->result;
 /* No retry, FD close, deadline renewal, natural-terminal inference or public
  * success. In particular EINTR preserves this actual operation's failure. */
}
#endif
