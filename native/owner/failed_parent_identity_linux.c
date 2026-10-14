#if defined(__linux__) && defined(__x86_64__)
#include "failed_parent_identity_linux.h"
#include <errno.h>
#include <sys/syscall.h>
static int excludes(const void *a,size_t n,const void *b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static int prebirth(const sl_linux_outer_birth *b){return b&&b->prepared==1&&!b->attempted&&!atomic_load_explicit(&b->entered,memory_order_acquire)&&!atomic_load_explicit(&b->parent_captured,memory_order_acquire)&&!atomic_load_explicit(&b->child_captured,memory_order_acquire);}
long sl_linux_acquire_failed_parent_identity(sl_linux_failed_parent_identity *o,
 const unsigned char *identity,size_t identity_extent,const sl_linux_outer_birth *birth,
 sl_linux_deadline_sequence *sequence){
 if(!o||identity_extent!=32||!excludes(o,sizeof(*o),identity,32)||!excludes(o,sizeof(*o),birth,sizeof(*birth))||
    !sl_linux_deadline_sequence_excludes_output(sequence,o,sizeof(*o))||
    !sl_linux_deadline_sequence_excludes_output(sequence,identity,32)||
    !sl_linux_deadline_sequence_excludes_output(sequence,birth,sizeof(*birth)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 if(!prebirth(birth))return -EINVAL;
 o->attempted=1;o->identity_original=identity;o->birth_original=birth;o->sequence_original=sequence;o->result=-EINVAL;
 o->process_boundary_result=sl_linux_begin_deadline_boundary(&o->process_boundary,sequence);
 if(o->process_boundary_result){o->result=o->process_boundary_result<0?o->process_boundary_result:-ETIMEDOUT;goto done;}
 atomic_init(&o->process.entered,0);atomic_init(&o->process.returned,0);
 o->process_raw=sl_linux_call6(&o->process,"failed-parent.name.getpid",SYS_getpid,0,0,0,0,0,0);
 o->process_boundary_result=sl_linux_end_deadline_boundary(&o->process_boundary);
 if(o->process_boundary_result){o->result=o->process_boundary_result<0?o->process_boundary_result:-ETIMEDOUT;goto done;}
 if(!sl_linux_deadline_boundary_matches_original(&o->process_boundary)||!prebirth(birth)||o->process.result!=o->process_raw||o->process_raw<=0||o->process_raw>INT_MAX)goto done;
 o->clock_boundary_result=sl_linux_begin_deadline_boundary(&o->clock_boundary,sequence);
 if(o->clock_boundary_result){o->result=o->clock_boundary_result<0?o->clock_boundary_result:-ETIMEDOUT;goto done;}
 atomic_init(&o->clock.entered,0);atomic_init(&o->clock.returned,0);
 o->clock_raw=sl_linux_call6(&o->clock,"failed-parent.name.clock",SYS_clock_gettime,CLOCK_MONOTONIC,(long)&o->instant,0,0,0,0);
 o->clock_boundary_result=sl_linux_end_deadline_boundary(&o->clock_boundary);
 if(o->clock_boundary_result){o->result=o->clock_boundary_result<0?o->clock_boundary_result:-ETIMEDOUT;goto done;}
 if(!sl_linux_deadline_boundary_matches_original(&o->clock_boundary)||!prebirth(birth)||o->clock.result!=o->clock_raw||o->clock_raw)goto done;
 o->name_result=sl_linux_construct_failed_archive_name(&o->name,identity,identity_extent,&o->process,&o->clock,&o->instant);
 o->result=o->name_result;
 done:o->returned=1;return o->result;
}
#endif
