/* Actual production naming body with synthetic native originals; UNRUN. */
#include "../failed_archive_name_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
int main(void){
 unsigned char identity[32]={0};struct timespec instant={0,0};
 sl_linux_call process={.number=SYS_getpid,.result=1},clock={.number=SYS_clock_gettime};
 atomic_init(&process.entered,1);atomic_init(&process.returned,1);
 atomic_init(&clock.entered,1);atomic_init(&clock.returned,1);
 clock.arguments[0]=CLOCK_MONOTONIC;clock.arguments[1]=(long)&instant;
 sl_linux_failed_archive_name o={0};
 assert(!sl_linux_construct_failed_archive_name(&o,identity,32,&process,&clock,&instant));
 assert(sl_linux_failed_archive_name_matches_original(&o));
 assert(o.extent==strlen((char *)o.name));
 assert(!strcmp((char *)o.name,"service-lasso-tui.0000000000000000000000000000000000000000000000000000000000000000.1.0.0.failed"));
 sl_linux_failed_archive_name saved=o;
 o.name[0]='x';assert(!sl_linux_failed_archive_name_matches_original(&o));o=saved;
 process.result=2;assert(!sl_linux_failed_archive_name_matches_original(&o));process.result=1;
 identity[0]=1;assert(!sl_linux_failed_archive_name_matches_original(&o));identity[0]=0;
 assert(sl_linux_failed_archive_name_matches_original(&o));
 assert(sl_linux_construct_failed_archive_name(&o,identity,32,&process,&clock,&instant)<0&&!memcmp(&o,&saved,sizeof(o)));
 memset(&o,0,sizeof(o));process.result=2147483647;instant.tv_sec=9223372036854775807L;instant.tv_nsec=999999999;
 assert(!sl_linux_construct_failed_archive_name(&o,identity,32,&process,&clock,&instant));
 assert(o.extent+1==SL_FAILED_ARCHIVE_NAME_BYTES);
 assert(sl_linux_failed_archive_name_matches_original(&o));
 for(unsigned mode=0;mode<4;mode++){
  memset(&o,0,sizeof(o));process.result=1;instant.tv_nsec=0;clock.arguments[1]=(long)&instant;
  atomic_store(&process.returned,1);
  if(mode==0)process.result=0;
  if(mode==1)instant.tv_nsec=1000000000;
  if(mode==2)clock.arguments[1]=0;
  if(mode==3)atomic_store(&process.returned,0);
  assert(sl_linux_construct_failed_archive_name(&o,identity,32,&process,&clock,&instant)<0&&o.returned);
 }
 memset(&o,0,sizeof(o));saved=o;
 assert(sl_linux_construct_failed_archive_name(&o,o.name,32,&process,&clock,&instant)<0&&!memcmp(&o,&saved,sizeof(o)));
 return 0;
}
