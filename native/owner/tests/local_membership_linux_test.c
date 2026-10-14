/* UNRUN substituted procfs/read fixture, not native namespace/exclusion proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../local_membership_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <errno.h>
static const char *text;static unsigned long offset;static unsigned calls,invalid_count;static long filesystem=PROC_SUPER_MAGIC;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 long result=0;unsigned long n;call->source=source;call->number=number;
 call->arguments[0]=a;call->arguments[1]=b;call->arguments[2]=c;call->arguments[3]=d;call->arguments[4]=e;call->arguments[5]=f;
 atomic_store(&call->entered,1);calls++;
 if(number==SYS_openat){assert(!strcmp((char *)b,"/proc/self/cgroup"));result=7;}
 else if(number==SYS_fstatfs)((struct statfs *)b)->f_type=filesystem;
 else if(number==SYS_read){
  if(invalid_count)result=c+1;
  else{n=strlen(text+offset);if(n>(unsigned long)c)n=c;memcpy((void *)b,text+offset,n);offset+=n;result=n;}
 }
 call->result=result;atomic_store(&call->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=fd;lift->held_fd=fd;return fd;}
#include "../local_membership_linux.c"
static void prepare(sl_linux_local_membership *observation,sl_linux_entry *entry,
 sl_linux_membership_frame *frames,unsigned char bytes[8][4],char path[32]){
 unsigned i;memset(observation,0,sizeof(*observation));memset(frames,0,8*sizeof(*frames));
 for(i=0;i<8;i++){frames[i].bytes=bytes[i];frames[i].capacity=4;}
 observation->entry_original=entry;observation->frames=frames;observation->capacity=8;
 observation->path=path;observation->path_capacity=32;offset=calls=0;invalid_count=0;filesystem=PROC_SUPER_MAGIC;
}
int main(void){
 sl_linux_entry entry={0};sl_linux_local_membership observation;sl_linux_membership_frame frames[8];unsigned char bytes[8][4];char path[32];unsigned before;
 entry.public_streams_prepared=1;
 prepare(&observation,&entry,frames,bytes,path);text="2:cpu:/old\n0::/owned\n";
 assert(sl_linux_observe_local_membership(&observation)==0&&observation.eof&&!strcmp(path,"/owned"));
 assert(frames[0].raw_count==4&&!memcmp(frames[0].bytes,"2:cp",4));
 assert(frames[observation.count-1].raw_count==0&&atomic_load(&frames[observation.count-1].read.returned));
 before=calls;assert(sl_linux_observe_local_membership(&observation)==-1&&calls==before);
 prepare(&observation,&entry,frames,bytes,path);text="0::/one\n0::/two\n";
 assert(sl_linux_observe_local_membership(&observation)==-1&&observation.eof);
 prepare(&observation,&entry,frames,bytes,path);text="0::/owned\n";observation.capacity=2;
 assert(sl_linux_observe_local_membership(&observation)==-1&&!observation.eof&&observation.count==2&&offset==8);
 prepare(&observation,&entry,frames,bytes,path);invalid_count=1;
 assert(sl_linux_observe_local_membership(&observation)==-1&&frames[0].raw_count==5&&frames[0].read.result==5);
 prepare(&observation,&entry,frames,bytes,path);filesystem=0;
 assert(sl_linux_observe_local_membership(&observation)==-1&&observation.fd==7&&observation.count==0);
 prepare(&observation,&entry,frames,bytes,path);frames[1].bytes=frames[0].bytes;
 assert(sl_linux_observe_local_membership(&observation)==-1&&calls==0);
 return 0;
}
#endif