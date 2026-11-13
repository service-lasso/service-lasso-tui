#if defined(__linux__) && defined(__x86_64__)
#include "owner_linux.h"
#include "shared_range_linux.h"
#include <sys/syscall.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/magic.h>
#include <stddef.h>
static int reserve(unsigned long *extent,unsigned long count,unsigned long unit,unsigned long alignment,unsigned long *offset){
 if(!count||count>ULONG_MAX/unit)return -1;
 unsigned long bytes=count*unit,padding=(alignment-*extent%alignment)%alignment;
 if(padding>ULONG_MAX-*extent||bytes>ULONG_MAX-*extent-padding)return -1;
 *offset=*extent+padding;*extent=*offset+bytes;return 0;
}
static void initialize(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
long sl_linux_prepare_credential_backing(sl_linux_preexec_authority *a){
 unsigned long extent=0,bytes[2],reads[2],groups[2];long result;
 void *locations[2][3]={{0}};unsigned contained=0;
 if(!a||!a->status_bytes||!a->status_attempts||!a->group_capacity||a->group_capacity>INT_MAX||
    a->credential_mapping.source||a->credential_mapping.number||atomic_load(&a->credential_mapping.entered)||atomic_load(&a->credential_mapping.returned)||
    a->credential_mapping_bytes||a->credentials_before.status||a->credentials_after.status||a->credentials_before.attempted||a->credentials_after.attempted)return -1;
 for(unsigned i=0;i<2;i++){
  if(reserve(&extent,a->status_bytes,1,1,&bytes[i])||
     reserve(&extent,a->status_attempts,sizeof(sl_linux_call),_Alignof(sl_linux_call),&reads[i])||
     reserve(&extent,a->group_capacity,sizeof(unsigned),_Alignof(unsigned),&groups[i]))return -1;
 }
 if(extent>LONG_MAX)return -1;
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++)if(a->credential_ranges[i][j].mapping_original)contained=1;
 if(contained){
  /* All six regions belong to the SAME prebirth containing acquisition.
   * This also makes every per-region origin exclusion transitive. */
  const sl_linux_call *origin=a->credential_ranges[0][0].mapping_original;
  if(!origin)return -1;
  for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++)if(a->credential_ranges[i][j].mapping_original!=origin)return -1;
  unsigned long needed[3]={a->status_bytes,a->status_attempts*sizeof(sl_linux_call),a->group_capacity*sizeof(unsigned)};
  unsigned long alignments[3]={1,_Alignof(sl_linux_call),_Alignof(unsigned)};
  uintptr_t owner=(uintptr_t)a;if(sizeof(*a)>UINTPTR_MAX-owner)return -1;
  for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++){
   sl_linux_shared_range *r=&a->credential_ranges[i][j];
   locations[i][j]=sl_linux_shared_range_address(r,needed[j],alignments[j]);if(!locations[i][j])return -1;
   uintptr_t start=(uintptr_t)locations[i][j],end=start+r->bytes,original=(uintptr_t)r->mapping_original;
   if(sizeof(sl_linux_call)>UINTPTR_MAX-original||(owner<end&&start<owner+sizeof(*a))||
      (original<owner+sizeof(*a)&&owner<original+sizeof(sl_linux_call))||
      (original<end&&start<original+sizeof(sl_linux_call)))return -1;
   for(unsigned x=0;x<=i;x++)for(unsigned y=0;y<3;y++){
    if(x==i&&y>=j)break;
    uintptr_t prior=(uintptr_t)locations[x][y];unsigned long n=a->credential_ranges[x][y].bytes;
    if(prior<end&&start<prior+n)return -1;
   }
   const unsigned char *fresh=locations[i][j];for(unsigned long k=0;k<r->bytes;k++)if(fresh[k])return -1;
  }
 }
 a->credential_mapping_bytes=extent;initialize(&a->credential_mapping);
 if(!contained){
 result=sl_linux_call6(&a->credential_mapping,"bootstrap.credentials.shared-backing",SYS_mmap,0,extent,
                      PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
 a->credential_mapping_result=result;if(result<=0)return result<0?result:-1;
 }else result=0;
 for(unsigned i=0;i<2;i++){
  sl_linux_credential_snapshot *s=i?&a->credentials_after:&a->credentials_before;
  s->status=contained?locations[i][0]:(unsigned char *)result+bytes[i];s->reads=contained?locations[i][1]:(void *)((unsigned char *)result+reads[i]);
  s->groups=contained?locations[i][2]:(void *)((unsigned char *)result+groups[i]);s->fd=-1;
  s->status_capacity=a->status_bytes;s->read_capacity=a->status_attempts;s->group_capacity=a->group_capacity;
  initialize(&s->open);initialize(&s->filesystem_query);initialize(&s->groups_query);initialize(&s->close);
  for(unsigned long j=0;j<s->read_capacity;j++)initialize(&s->reads[j]);
 }
 return 0;
}
static int ids(const unsigned char *p,unsigned long n,unsigned values[4]){
 unsigned long at=0;
 for(unsigned i=0;i<4;i++){
  while(at<n&&(p[at]==' '||p[at]=='\t'))at++;
  if(at==n||p[at]<'0'||p[at]>'9')return -1;
  unsigned value=0;
  while(at<n&&p[at]>='0'&&p[at]<='9'){
   unsigned digit=p[at++]-'0';if(value>(UINT_MAX-digit)/10)return -1;value=value*10+digit;
  }
  values[i]=value;
 }
 while(at<n&&(p[at]==' '||p[at]=='\t'))at++;
 return at==n?0:-1;
}
static int parse(sl_linux_credential_snapshot *s){
 unsigned uid=0,gid=0;unsigned long at=0;
 while(at<s->length){
  unsigned long start=at;while(at<s->length&&s->status[at]!='\n')at++;
  if(at==s->length)return -1;
  unsigned long n=at-start;const unsigned char *p=s->status+start;at++;
  if(n>=4&&p[0]=='U'&&p[1]=='i'&&p[2]=='d'&&p[3]==':'){
   if(uid++||ids(p+4,n-4,s->uids))return -1;
  }else if(n>=4&&p[0]=='G'&&p[1]=='i'&&p[2]=='d'&&p[3]==':'){
   if(gid++||ids(p+4,n-4,s->gids))return -1;
  }
 }
 return uid==1&&gid==1?0:-1;
}
long sl_linux_observe_child_credentials(sl_linux_credential_snapshot *s){
 long result=-1;
 if(!s||s->attempted||!s->status||!s->reads||!s->groups||!s->status_capacity||!s->read_capacity||
    !s->group_capacity||s->group_capacity>INT_MAX)return -1;
 s->attempted=1;
 result=sl_linux_call6(&s->open,"bootstrap.credentials.status.open",SYS_openat,AT_FDCWD,
        (long)"/proc/thread-self/status",O_RDONLY|O_NONBLOCK|O_CLOEXEC,0,0,0);
 if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}s->fd=(int)result;
 result=sl_linux_call6(&s->filesystem_query,"bootstrap.credentials.status.filesystem",SYS_fstatfs,
        s->fd,(long)&s->filesystem,0,0,0,0);
 if(result<0)goto done;if(result||s->filesystem.f_type!=PROC_SUPER_MAGIC){result=-1;goto done;}
 while(!s->eof){
  /* Never issue a zero-length read and mistake it for genuine EOF. Full
   * capacity without an observed EOF is explicit exhaustion/retention. */
  if(s->read_count==s->read_capacity||s->length==s->status_capacity){result=-1;goto done;}
  unsigned long remaining=s->status_capacity-s->length;
  sl_linux_call *call=&s->reads[s->read_count++];
  result=sl_linux_call6(call,"bootstrap.credentials.status.read",SYS_read,s->fd,
                       (long)(s->status+s->length),remaining,0,0,0);
  if(result<0)goto done;if((unsigned long)result>remaining){result=-1;goto done;}
  if(!result)s->eof=1;else s->length+=(unsigned long)result;
 }
 if(parse(s)){result=-1;goto done;}
 result=sl_linux_call6(&s->groups_query,"bootstrap.credentials.getgroups",SYS_getgroups,
                      s->group_capacity,(long)s->groups,0,0,0,0);
 if(result<0)goto done;if((unsigned long)result>s->group_capacity){result=-1;goto done;}
 s->group_count=(unsigned)result;
 result=0;
done:
 /* Every issued native read/group call has actually returned before this
  * traversal. This independently owned status descriptor has no other source
  * borrower. Semantic refusal cannot skip its eligible single close; a failed
  * or uncertain close remains a separate original and is never retried. */
 if(s->fd>=0 && atomic_load_explicit(&s->open.returned,memory_order_acquire) && s->open.result>=0 && (long)s->fd==s->open.result &&
    !atomic_load_explicit(&s->close.entered,memory_order_acquire)){
  long closed=sl_linux_call6(&s->close,"bootstrap.credentials.status.close",SYS_close,s->fd,0,0,0,0,0);
  if(!result&&closed<0)result=closed;
 }
 s->result=result;s->returned=1;return result;
}
#endif
