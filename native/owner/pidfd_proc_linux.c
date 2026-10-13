#if defined(__linux__) && defined(__x86_64__)
#include "namespace_parent_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <limits.h>
_Static_assert(sizeof(int)==4,"PID/fd decimal storage requires selected 32-bit UAPI integers");
static int pid_field(sl_linux_pidfd_proc *o){
 unsigned long at=0;
 while(at<o->length){
  unsigned long start=at;while(at<o->length&&o->actual_bytes[at]!='\n')at++;
  if(at==o->length)return -1;unsigned long end=at;at++;
  const unsigned char *p=o->actual_bytes;
  if(end-start<4||p[start]!='P'||p[start+1]!='i'||p[start+2]!='d'||p[start+3]!=':')continue;
  if(o->pid_seen++)return -1;o->pid_offset=start+4;o->pid_length=end-o->pid_offset;
  unsigned long i=o->pid_offset;while(i<end&&(p[i]==' '||p[i]=='\t'))i++;
  unsigned negative=0;if(i<end&&p[i]=='-'){negative=1;i++;}
  if(i==end||p[i]<'0'||p[i]>'9')return -1;
  long value=0;while(i<end&&p[i]>='0'&&p[i]<='9'){
   unsigned digit=p[i++]-'0';if(value>(INT_MAX-digit)/10)return -1;value=value*10+digit;
  }
  while(i<end&&(p[i]==' '||p[i]=='\t'))i++;
  if(i!=end)return -1;o->raw_pid=negative?-value:value;
 }
 return o->pid_seen==1&&o->raw_pid>0?0:-1;
}
long sl_linux_observe_pidfd_proc(sl_linux_pidfd_proc *o){
 long result=-1;int held=-1;sl_linux_bootstrap *b;sl_linux_namespace_view *proc;
 if(!o||o->attempted||!o->birth_original||!o->procfs_original)return -1;o->attempted=1;
 o->actual_bytes=o->bytes;o->actual_reads=o->reads;o->actual_capacity=o->capacity;o->actual_read_capacity=o->read_capacity;
 b=o->birth_original->bootstrap;proc=o->procfs_original;
 if(!b||b->pidfd<0||proc->fd<3||!atomic_load_explicit(&proc->filesystem_query.returned,memory_order_acquire)||
    proc->filesystem_query.result||proc->filesystem.f_type!=PROC_SUPER_MAGIC||!o->actual_bytes||!o->actual_reads||
    !o->actual_capacity||!o->actual_read_capacity||o->length||o->read_count||o->open.source||o->open.number||o->lift.attempted||
    o->filesystem_query.source||o->filesystem_query.number||o->close.source||o->close.number||
    atomic_load(&o->open.entered)||atomic_load(&o->open.returned)||atomic_load(&o->filesystem_query.entered)||
    atomic_load(&o->filesystem_query.returned)||atomic_load(&o->close.entered)||atomic_load(&o->close.returned))goto done;
 o->actual_pidfd=b->pidfd;
 for(unsigned long i=0;i<o->actual_read_capacity;i++){
  sl_linux_call *c=&o->actual_reads[i];if(c->source||c->number||atomic_load(&c->entered)||atomic_load(&c->returned))goto done;
  atomic_init(&c->entered,0);atomic_init(&c->returned,0);
 }
 atomic_init(&o->open.entered,0);atomic_init(&o->open.returned,0);
 atomic_init(&o->filesystem_query.entered,0);atomic_init(&o->filesystem_query.returned,0);
 atomic_init(&o->close.entered,0);atomic_init(&o->close.returned,0);o->fd=-1;
 unsigned long fd=(unsigned)o->actual_pidfd;unsigned at=sizeof o->fd_digits-1;o->fd_digits[at]=0;
 do{o->fd_digits[--at]=(char)('0'+fd%10);fd/=10;}while(fd);
 static const char prefix[]="thread-self/fdinfo/";unsigned out=0;
 for(unsigned i=0;i<sizeof(prefix)-1;i++)o->locator[out++]=prefix[i];
 while(o->fd_digits[at])o->locator[out++]=o->fd_digits[at++];o->locator[out]=0;
 o->raw_fd=sl_linux_call6(&o->open,"controller.pidfd.fdinfo.open",SYS_openat,proc->fd,(long)o->locator,
                         O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW,0,0,0);
 result=o->raw_fd;if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&o->lift,(int)o->raw_fd);if(result<0)goto done;o->fd=held=(int)result;
 result=sl_linux_call6(&o->filesystem_query,"controller.pidfd.fdinfo.filesystem",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);
 if(result<0)goto done;if(result||o->filesystem.f_type!=PROC_SUPER_MAGIC){result=-1;goto done;}
 while(!o->eof){
  if(o->read_count==o->actual_read_capacity||o->length==o->actual_capacity){result=-1;goto done;}
  unsigned long remaining=o->actual_capacity-o->length;sl_linux_call *c=&o->actual_reads[o->read_count++];
  result=sl_linux_call6(c,"controller.pidfd.fdinfo.read",SYS_read,o->fd,(long)(o->actual_bytes+o->length),remaining,0,0,0);
  if(result<0)goto done;if((unsigned long)result>remaining){result=-1;goto done;}
  if(!result)o->eof=1;else o->length+=(unsigned long)result;
 }
 result=pid_field(o);
done:
 if(held>=0&&o->fd==held&&o->lift.attempted&&o->lift.held_fd==o->fd&&!atomic_load_explicit(&o->close.entered,memory_order_acquire)){
  long closed=sl_linux_call6(&o->close,"controller.pidfd.fdinfo.close",SYS_close,o->fd,0,0,0,0,0);
  if(!result&&closed<0)result=closed;
 }
 o->result=result;o->returned=1;return result;
 /* Pid is the actual held pidfd report in this procfs instance's PID realm.
  * FD-table/procfs/source identity and original live brackets are still required;
  * neither numeric clone-PID equality nor a pathname grants qualification. */
}
#endif