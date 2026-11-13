#if defined(__linux__) && defined(__x86_64__)
#include "local_membership_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <stdint.h>
static void initialize(sl_linux_call *call){atomic_init(&call->entered,0);atomic_init(&call->returned,0);}
static int overlaps(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
long sl_linux_observe_local_membership(sl_linux_local_membership *observation){
 unsigned long i,j,k;long result;unsigned prefix=0,unified=0,record=0;unsigned char byte;
 if(!observation||observation->attempted)return -1;
 observation->attempted=1;observation->fd=-1;observation->result=-1;
 if(!observation->entry_original||!observation->entry_original->public_streams_prepared||
    !observation->frames||!observation->capacity||observation->capacity>UINTPTR_MAX/sizeof(*observation->frames)||
    !observation->path||observation->path_capacity<2||
    overlaps(observation->frames,observation->capacity*sizeof(*observation->frames),observation,sizeof(*observation))||
    overlaps(observation->path,observation->path_capacity,observation,sizeof(*observation))||
    overlaps(observation->path,observation->path_capacity,observation->frames,observation->capacity*sizeof(*observation->frames)))goto done;
 for(i=0;i<observation->capacity;i++){
  sl_linux_membership_frame *frame=&observation->frames[i];
  if(!frame->bytes||!frame->capacity||frame->capacity>2147483647UL||
          overlaps(frame->bytes,frame->capacity,observation->path,observation->path_capacity)||
     overlaps(frame->bytes,frame->capacity,observation,sizeof(*observation))||
     overlaps(frame->bytes,frame->capacity,observation->frames,observation->capacity*sizeof(*observation->frames)))goto done;
  for(j=0;j<i;j++)if(overlaps(frame->bytes,frame->capacity,observation->frames[j].bytes,observation->frames[j].capacity))goto done;
 }
 observation->prepared_frames=observation->frames;observation->prepared_capacity=observation->capacity;
 observation->prepared_path=observation->path;observation->prepared_path_capacity=observation->path_capacity;
 initialize(&observation->open);initialize(&observation->filesystem_query);
 /* self is the kernel's current-process relation. Procfs identity/exclusion
  * remains an actual qualified native premise, not pathname authority. */
 result=sl_linux_call6(&observation->open,"local.membership.open",SYS_openat,
    AT_FDCWD,(long)"/proc/self/cgroup",O_RDONLY|O_NONBLOCK|O_CLOEXEC,0,0,0);
 observation->result=result;if(result<0||result>2147483647L){if(result>2147483647L)observation->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&observation->lift,(int)result);observation->result=result;if(result<0)goto done;
 observation->fd=(int)result;
 result=sl_linux_call6(&observation->filesystem_query,"local.membership.fstatfs",SYS_fstatfs,
    observation->fd,(long)&observation->filesystem,0,0,0,0);
 observation->result=result;if(result||observation->filesystem.f_type!=PROC_SUPER_MAGIC){observation->result=result?result:-1;goto done;}
 for(i=0;i<observation->prepared_capacity;i++){
  sl_linux_membership_frame *frame=&observation->prepared_frames[i];initialize(&frame->read);
  result=sl_linux_call6(&frame->read,"local.membership.read",SYS_read,observation->fd,(long)frame->bytes,frame->capacity,0,0,0);
  frame->raw_count=result;observation->count=i+1;observation->result=result;
  if(result<0||result>(long)frame->capacity){if(result>(long)frame->capacity)observation->result=-1;goto done;}
  if(!result){observation->eof=1;break;}
 }
 if(!observation->eof){observation->result=-1;goto done;}
 /* Parse only genuine completed EOF frames, retaining all original bytes.
  * Other hierarchy records are observed but cannot supply the v2 locator. */
 for(i=0;i<observation->count;i++)for(k=0;k<(unsigned long)observation->prepared_frames[i].raw_count;k++){
  byte=observation->prepared_frames[i].bytes[k];if(!byte){observation->result=-1;goto done;}
  if(byte=='\n'){
   if(prefix==3){if(record||!observation->path_length||observation->prepared_path[0]!='/'){observation->result=-1;goto done;}record=1;}
   prefix=0;unified=0;continue;
  }
  if(prefix<3){
   static const unsigned char marker[3]={'0',':',':'};
   if(!unified&&byte==marker[prefix])prefix++;
   else unified=1;
   continue;
  }
  if(record||observation->path_length+1>=observation->prepared_path_capacity){observation->result=-1;goto done;}
  observation->prepared_path[observation->path_length++]=(char)byte;
 }
 if(!record||prefix||unified){observation->result=-1;goto done;}
 observation->prepared_path[observation->path_length]=0;observation->result=0;
done:
 observation->returned=1;return observation->result;
}
#endif