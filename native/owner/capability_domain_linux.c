#if defined(__linux__) && defined(__x86_64__)
#include "capability_domain_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <linux/capability.h>
#include <fcntl.h>
_Static_assert(_LINUX_CAPABILITY_U32S_3==2,"selected native capability source requires v3 two-word representation");
static void domain_initialize(sl_linux_call *call){atomic_init(&call->entered,0);atomic_init(&call->returned,0);}
long sl_linux_observe_capability_domain(sl_linux_capability_domain *domain){
 unsigned i,value=0,digits=0;long result;
 if(!domain||domain->attempted)return -1;
 domain->attempted=1;domain->result=-1;domain->fd=-1;
 if(!domain->entry_original||!domain->entry_original->public_streams_prepared||domain->returned||domain->count||domain->eof)goto done;
 domain_initialize(&domain->open);domain_initialize(&domain->filesystem_query);
 for(i=0;i<4;i++)domain_initialize(&domain->reads[i]);
 result=sl_linux_call6(&domain->open,"native.cap-domain.open",SYS_openat,AT_FDCWD,(long)"/proc/sys/kernel/cap_last_cap",O_RDONLY|O_NONBLOCK|O_CLOEXEC,0,0,0);
 domain->result=result;if(result<0||result>2147483647L){if(result>=0)domain->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&domain->lift,(int)result);domain->result=result;if(result<0)goto done;domain->fd=(int)result;
 result=sl_linux_call6(&domain->filesystem_query,"native.cap-domain.fstatfs",SYS_fstatfs,domain->fd,(long)&domain->filesystem,0,0,0,0);
 domain->result=result;if(result||domain->filesystem.f_type!=PROC_SUPER_MAGIC){if(!result)domain->result=-1;goto done;}
 for(i=0;i<4;i++){
  result=sl_linux_call6(&domain->reads[i],"native.cap-domain.read-byte",SYS_read,domain->fd,(long)&domain->bytes[i],1,0,0,0);
  domain->raw_counts[i]=result;domain->count=i+1;domain->result=result;
  if(result<0||result>1){if(result>1)domain->result=-1;goto done;}
  if(!result){domain->eof=1;break;}
 }
 if(!domain->eof||domain->count<3){domain->result=-1;goto done;}
 /* Parse only completed originals; valid v3 domain is decimal0..63 followed
  * by one newline. Anything wider is unsupported, never truncated/masked. */
 for(i=0;i+1<domain->count;i++){
  unsigned char byte=domain->bytes[i];
  if(byte=='\n'){if(!digits||i+2!=domain->count){domain->result=-1;goto done;}break;}
  if(byte<'0'||byte>'9'||digits==2){domain->result=-1;goto done;}
  value=value*10+(byte-'0');digits++;
 }
 if(i+2!=domain->count||domain->bytes[i]!='\n'||value>=64){domain->result=-1;goto done;}
 domain->last_cap=value;domain->result=0;
done:
 domain->returned=1;return domain->result;
}
#endif