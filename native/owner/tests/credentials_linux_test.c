/* UNRUN controlled read-only credentials source case, not genuine procfs,
 * task/userns correspondence, ACL/group exclusion or native profile proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../owner_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <errno.h>
static const char *body;static unsigned reads,closes;static int invalid_count,read_error,close_error;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 c->source=source;c->number=number;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);
 long result=-1;
 if(number==SYS_openat)result=7;
 if(number==SYS_fstatfs){((struct statfs *)b)->f_type=PROC_SUPER_MAGIC;result=0;}
 if(number==SYS_read){
  reads++;if(read_error)result=-EIO;
  else if(invalid_count)result=d+1;
  else if(reads==1){result=strlen(body);assert(result<=d);memcpy((void *)b,body,result);}else result=0;
 }
 if(number==SYS_getgroups){assert(a>=2);((unsigned *)b)[0]=44;((unsigned *)b)[1]=55;result=2;}
 if(number==SYS_close){closes++;result=close_error?-EINTR:0;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../credentials_linux.c"
static void setup(sl_linux_credential_snapshot *s,unsigned char *bytes,sl_linux_call *calls,unsigned *groups){
 memset(s,0,sizeof *s);memset(calls,0,2*sizeof *calls);reads=closes=invalid_count=read_error=close_error=0;
 s->status=bytes;s->status_capacity=256;s->reads=calls;s->read_capacity=2;s->groups=groups;s->group_capacity=2;
}
static void containing_backing_cases(void){
 /* Distinct synthetic acquisitions with identical values must still be
  * refused as different original occurrences. No real mmap is performed. */
 static struct {
  sl_linux_call original,other;sl_linux_preexec_authority authority;
  _Alignas(sl_linux_call) unsigned char backing[512];
 } cases[5];
 for(unsigned mode=0;mode<5;mode++){
  sl_linux_preexec_authority *a=&cases[mode].authority;sl_linux_call *m=&cases[mode].original;
  m->source="synthetic.containing.mmap";m->number=SYS_mmap;m->arguments[1]=512;
  m->arguments[2]=PROT_READ|PROT_WRITE;m->arguments[3]=MAP_SHARED|MAP_ANONYMOUS;m->arguments[4]=-1;
  m->result=(long)cases[mode].backing;atomic_store(&m->entered,1);atomic_store(&m->returned,1);
  memcpy(&cases[mode].other,m,sizeof(*m));a->status_bytes=64;a->status_attempts=1;a->group_capacity=2;
  for(unsigned i=0;i<2;i++){
   a->credential_ranges[i][0]=(sl_linux_shared_range){m,256*i,64};
   a->credential_ranges[i][1]=(sl_linux_shared_range){m,256*i+64,sizeof(sl_linux_call)};
   a->credential_ranges[i][2]=(sl_linux_shared_range){m,256*i+64+sizeof(sl_linux_call),2*sizeof(unsigned)};
  }
  if(mode==1)a->credential_ranges[1][2].mapping_original=&cases[mode].other;
  if(mode==2)a->credential_ranges[1][0].offset=0;
  if(mode==3)cases[mode].backing[511]=1; /* enlarge checked group backing to include dirty byte */
  if(mode==3)a->credential_ranges[1][2].bytes=512-a->credential_ranges[1][2].offset;
  if(mode==4)a->credential_ranges[0][1].offset++;
  unsigned char original[sizeof(*a)];memcpy(original,a,sizeof(*a));
  long result=sl_linux_prepare_credential_backing(a);
  if(mode)assert(result<0&&!memcmp(original,a,sizeof(*a)));
  else{
   assert(!result&&a->credentials_before.status==cases[mode].backing&&a->credentials_after.status==cases[mode].backing+256);
   assert(!a->credential_mapping.source&&!atomic_load(&a->credential_mapping.entered)&&!a->credential_mapping_result);
   assert(a->credentials_before.reads==(void *)(cases[mode].backing+64));
  }
 }
}
int main(void){
 containing_backing_cases();
 sl_linux_credential_snapshot s;unsigned char bytes[256];sl_linux_call calls[2];unsigned groups[2];
 setup(&s,bytes,calls,groups);body="Name:\ttui\nUid:\t1000 1000 1000 0\nGid:\t10 20 30 40\n";
 assert(sl_linux_observe_child_credentials(&s)==0&&s.eof&&s.uids[3]==0&&s.gids[3]==40&&s.group_count==2);
 /* fsuid0 is truthfully observed here; the preexec caller must deny it. */
 assert(groups[0]==44&&groups[1]==55&&closes==1);unsigned prior=reads;
 assert(sl_linux_observe_child_credentials(&s)<0&&reads==prior&&closes==1);
 const char *bad[]={"Uid: 1 2 3 4\n", "Uid: 1 2 3 4\nUid: 1 2 3 4\nGid: 1 2 3 4\n", "Uid: 1 2 3 4294967296\nGid: 1 2 3 4\n", "Uid: 1 2 3 4\nGid: 1 2 3 4"};
 for(unsigned i=0;i<4;i++){setup(&s,bytes,calls,groups);body=bad[i];assert(sl_linux_observe_child_credentials(&s)<0&&s.eof&&closes==1);}
 setup(&s,bytes,calls,groups);invalid_count=1;
 assert(sl_linux_observe_child_credentials(&s)<0&&calls[0].result==257&&s.length==0&&closes==1);
 setup(&s,bytes,calls,groups);read_error=1;
 assert(sl_linux_observe_child_credentials(&s)==-EIO&&calls[0].result==-EIO&&s.fd==7&&closes==1);
 setup(&s,bytes,calls,groups);body="Uid: 1 2 3 4\nGid: 1 2 3 4\n";s.status_capacity=strlen(body);
 assert(sl_linux_observe_child_credentials(&s)<0&&reads==1&&!s.eof&&closes==1);
 setup(&s,bytes,calls,groups);read_error=close_error=1;
 assert(sl_linux_observe_child_credentials(&s)==-EIO&&calls[0].result==-EIO&&s.close.result==-EINTR&&closes==1);
 assert(sl_linux_observe_child_credentials(&s)<0&&closes==1&&s.close.result==-EINTR);
 setup(&s,bytes,calls,groups);body="Uid: 1 2 3 4\nGid: 1 2 3 4\n";close_error=1;
 assert(sl_linux_observe_child_credentials(&s)==-EINTR&&s.eof&&s.close.result==-EINTR&&closes==1);
 return 0;
}
#endif
