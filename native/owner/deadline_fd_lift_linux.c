#if defined(__linux__) && defined(__x86_64__)
#include "deadline_fd_lift_linux.h"
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/syscall.h>
static int separate(const void *o,size_t n,const void *p,size_t m){uintptr_t a=(uintptr_t)o,b=(uintptr_t)p;return o&&p&&n<=UINTPTR_MAX-a&&m<=UINTPTR_MAX-b&&!(a<b+m&&b<a+n);}
static int original(const sl_linux_call *c,long number,long fd,long a1,long a2,long raw){
 if(atomic_load_explicit(&c->entered,memory_order_acquire)!=1||atomic_load_explicit(&c->returned,memory_order_acquire)!=1||c->number!=number||c->arguments[0]!=fd||c->arguments[1]!=a1||c->arguments[2]!=a2||c->result!=raw)return 0;
 for(unsigned i=3;i<6;i++)if(c->arguments[i])return 0;return 1;
}
long sl_linux_lift_deadline_fd(sl_linux_deadline_fd_lift *o,
 const sl_linux_call *producer,sl_linux_deadline_sequence *sequence){
 if(!o||!separate(o,sizeof(*o),producer,sizeof(*producer))||
    !sl_linux_deadline_sequence_excludes_output(sequence,o,sizeof(*o))||
    !sl_linux_deadline_sequence_excludes_output(sequence,producer,sizeof(*producer)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 if(atomic_load_explicit(&producer->entered,memory_order_acquire)!=1||
    atomic_load_explicit(&producer->returned,memory_order_acquire)!=1||producer->result<0||producer->result>INT_MAX)return -EINVAL;
 o->attempted=1;o->producer_original=producer;o->sequence_original=sequence;
 o->result=-EINVAL;o->lift.attempted=1;o->lift.original_fd=(int)producer->result;o->lift.held_fd=-1;
 if(o->lift.original_fd>=3){o->lift.held_fd=o->lift.original_fd;o->result=o->lift.held_fd;goto done;}
 o->duplicate_clock_result=sl_linux_begin_deadline_boundary(&o->duplicate_boundary,sequence);
 if(o->duplicate_clock_result){o->result=o->duplicate_clock_result<0?o->duplicate_clock_result:-ETIMEDOUT;goto done;}
 atomic_init(&o->lift.duplicate.entered,0);atomic_init(&o->lift.duplicate.returned,0);
 long raw=sl_linux_call6(&o->lift.duplicate,"failed-parent.low-fd.duplicate",SYS_fcntl,o->lift.original_fd,F_DUPFD_CLOEXEC,3,0,0,0);
 /* Retain an actually produced entry even when the afterclock fails. */
 if(raw>=3&&raw<=INT_MAX)o->lift.held_fd=(int)raw;
 o->duplicate_clock_result=sl_linux_end_deadline_boundary(&o->duplicate_boundary);
 if(o->duplicate_clock_result){o->result=o->duplicate_clock_result<0?o->duplicate_clock_result:-ETIMEDOUT;goto done;}
 if(!sl_linux_deadline_boundary_matches_original(&o->duplicate_boundary)||producer->result!=o->lift.original_fd||
    !original(&o->lift.duplicate,SYS_fcntl,o->lift.original_fd,F_DUPFD_CLOEXEC,3,raw)){o->result=-EINVAL;goto done;}
 if(raw<3||raw>INT_MAX){o->result=raw<0?raw:-EINVAL;goto done;}
 o->close_clock_result=sl_linux_begin_deadline_boundary(&o->close_boundary,sequence);
 if(o->close_clock_result){o->result=o->close_clock_result<0?o->close_clock_result:-ETIMEDOUT;goto done;}
 atomic_init(&o->lift.close_original.entered,0);atomic_init(&o->lift.close_original.returned,0);
 raw=sl_linux_call6(&o->lift.close_original,"failed-parent.low-fd.close",SYS_close,o->lift.original_fd,0,0,0,0,0);
 o->close_clock_result=sl_linux_end_deadline_boundary(&o->close_boundary);
 if(o->close_clock_result){o->result=o->close_clock_result<0?o->close_clock_result:-ETIMEDOUT;goto done;}
 if(!sl_linux_deadline_boundary_matches_original(&o->close_boundary)||producer->result!=o->lift.original_fd||
    !original(&o->lift.close_original,SYS_close,o->lift.original_fd,0,0,raw)){o->result=-EINVAL;goto done;}
 o->result=raw<0?raw:raw?-EINVAL:o->lift.held_fd;
 done:o->returned=1;return o->result;
}
int sl_linux_deadline_fd_lift_matches_original(const sl_linux_deadline_fd_lift *o){
 if(!o||o->attempted!=1||o->returned!=1||o->lift.attempted!=1||o->result<3||o->result>INT_MAX||
    !separate(o,sizeof(*o),o->producer_original,sizeof(*o->producer_original))||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o,sizeof(*o))||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o->producer_original,sizeof(*o->producer_original)))return 0;
 const sl_linux_call *p=o->producer_original;
 if(atomic_load_explicit(&p->entered,memory_order_acquire)!=1||atomic_load_explicit(&p->returned,memory_order_acquire)!=1||
    p->result<0||p->result>INT_MAX||o->lift.original_fd!=p->result||o->lift.held_fd!=o->result)return 0;
 if(p->result>=3)return o->result==p->result&&!o->duplicate_boundary.attempted&&!o->close_boundary.attempted&&
    !atomic_load_explicit(&o->lift.duplicate.entered,memory_order_acquire)&&!atomic_load_explicit(&o->lift.close_original.entered,memory_order_acquire);
 return !o->duplicate_clock_result&&!o->close_clock_result&&
    o->duplicate_boundary.sequence==o->sequence_original&&o->close_boundary.sequence==o->sequence_original&&
    sl_linux_deadline_boundary_matches_original(&o->duplicate_boundary)&&sl_linux_deadline_boundary_matches_original(&o->close_boundary)&&
    original(&o->lift.duplicate,SYS_fcntl,p->result,F_DUPFD_CLOEXEC,3,o->result)&&
    original(&o->lift.close_original,SYS_close,p->result,0,0,0);
}
#endif
