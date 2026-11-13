#if defined(__linux__) && defined(__x86_64__)
#include "failed_archive_name_linux.h"
#include <errno.h>
#include <limits.h>
#include <sys/syscall.h>
static int excludes(const void *o,size_t n,const void *p,size_t m){uintptr_t a=(uintptr_t)o,b=(uintptr_t)p;return o&&p&&n<=UINTPTR_MAX-a&&m<=UINTPTR_MAX-b&&!(a<b+m&&b<a+n);}
static size_t decimal(unsigned char *p,uint64_t n){unsigned char reversed[20];size_t count=0;do{reversed[count++]=(unsigned char)('0'+n%10);n/=10;}while(n);for(size_t i=0;i<count;i++)p[i]=reversed[count-1-i];return count;}
long sl_linux_construct_failed_archive_name(sl_linux_failed_archive_name *o,
 const unsigned char *identity,size_t identity_extent,const sl_linux_call *process,
 const sl_linux_call *clock,const struct timespec *instant){
 if(!o||identity_extent!=32||!excludes(o,sizeof(*o),identity,32)||
    !excludes(o,sizeof(*o),process,sizeof(*process))||
    !excludes(o,sizeof(*o),clock,sizeof(*clock))||
    !excludes(o,sizeof(*o),instant,sizeof(*instant)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 o->attempted=1;o->identity_original=identity;o->process_original=process;o->clock_original=clock;o->instant_original=instant;o->result=-EINVAL;
 if(atomic_load_explicit(&process->entered,memory_order_acquire)!=1||atomic_load_explicit(&process->returned,memory_order_acquire)!=1||
    atomic_load_explicit(&clock->entered,memory_order_acquire)!=1||atomic_load_explicit(&clock->returned,memory_order_acquire)!=1||
    process->number!=SYS_getpid||process->result<=0||process->result>INT_MAX||clock->number!=SYS_clock_gettime||clock->result||
    clock->arguments[0]!=CLOCK_MONOTONIC||clock->arguments[1]!=(long)instant)goto done;
 for(unsigned i=0;i<6;i++)if(process->arguments[i])goto done;
 for(unsigned i=2;i<6;i++)if(clock->arguments[i])goto done;
 if(instant->tv_sec<0||instant->tv_nsec<0||instant->tv_nsec>=1000000000L)goto done;
 static const unsigned char prefix[]="service-lasso-tui.";static const unsigned char hex[]="0123456789abcdef";
 size_t at=0;for(size_t i=0;i<sizeof(prefix)-1;i++)o->name[at++]=prefix[i];
 for(unsigned i=0;i<32;i++){o->name[at++]=hex[identity[i]>>4];o->name[at++]=hex[identity[i]&15];}
 o->name[at++]='.';at+=decimal(o->name+at,(uint64_t)process->result);
 o->name[at++]='.';at+=decimal(o->name+at,(uint64_t)instant->tv_sec);
 o->name[at++]='.';at+=decimal(o->name+at,(uint64_t)instant->tv_nsec);
 static const unsigned char suffix[]=".failed";for(size_t i=0;i<sizeof(suffix);i++)o->name[at++]=suffix[i];
 o->extent=at-1;o->result=0;
 done:o->returned=1;return o->result;
}
static int match_decimal(const unsigned char *p,size_t extent,size_t *at,uint64_t n){
 uint64_t divisor=1;while(n/divisor>=10){if(divisor>UINT64_MAX/10)return 0;divisor*=10;}
 do{if(*at>=extent||p[(*at)++]!=(unsigned char)('0'+n/divisor))return 0;n%=divisor;divisor/=10;}while(divisor);return 1;
}
int sl_linux_failed_archive_name_matches_original(const sl_linux_failed_archive_name *o){
 if(!o||o->attempted!=1||o->returned!=1||o->result||!o->extent||o->extent>=sizeof(o->name)||
    !excludes(o,sizeof(*o),o->identity_original,32)||!excludes(o,sizeof(*o),o->process_original,sizeof(*o->process_original))||
    !excludes(o,sizeof(*o),o->clock_original,sizeof(*o->clock_original))||!excludes(o,sizeof(*o),o->instant_original,sizeof(*o->instant_original)))return 0;
 const sl_linux_call *process=o->process_original,*clock=o->clock_original;const struct timespec *instant=o->instant_original;
 if(atomic_load_explicit(&process->entered,memory_order_acquire)!=1||atomic_load_explicit(&process->returned,memory_order_acquire)!=1||
    atomic_load_explicit(&clock->entered,memory_order_acquire)!=1||atomic_load_explicit(&clock->returned,memory_order_acquire)!=1||
    process->number!=SYS_getpid||process->result<=0||process->result>INT_MAX||clock->number!=SYS_clock_gettime||clock->result||
    clock->arguments[0]!=CLOCK_MONOTONIC||clock->arguments[1]!=(long)instant||instant->tv_sec<0||instant->tv_nsec<0||instant->tv_nsec>=1000000000L)return 0;
 for(unsigned i=0;i<6;i++)if(process->arguments[i])return 0;for(unsigned i=2;i<6;i++)if(clock->arguments[i])return 0;
 static const unsigned char prefix[]="service-lasso-tui.",hex[]="0123456789abcdef",suffix[]=".failed";
 size_t at=0;for(size_t i=0;i<sizeof(prefix)-1;i++)if(at>=o->extent||o->name[at++]!=prefix[i])return 0;
 for(unsigned i=0;i<32;i++){
  if(at>=o->extent||o->name[at++]!=hex[o->identity_original[i]>>4])return 0;
  if(at>=o->extent||o->name[at++]!=hex[o->identity_original[i]&15])return 0;
 }
 if(at>=o->extent||o->name[at++]!='.'||!match_decimal(o->name,o->extent,&at,(uint64_t)process->result))return 0;
 if(at>=o->extent||o->name[at++]!='.'||!match_decimal(o->name,o->extent,&at,(uint64_t)instant->tv_sec))return 0;
 if(at>=o->extent||o->name[at++]!='.'||!match_decimal(o->name,o->extent,&at,(uint64_t)instant->tv_nsec))return 0;
 for(size_t i=0;i<sizeof(suffix)-1;i++)if(at>=o->extent||o->name[at++]!=suffix[i])return 0;
 return at==o->extent&&!o->name[at];
}
#endif
