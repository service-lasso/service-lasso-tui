/* UNRUN controlled held namespace/query source cases. No genuine procfs,
 * namespace/capability/ptrace rights/profile/native admission is claimed. */
#if defined(__linux__) && defined(__x86_64__)
#include "../namespace_parent_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/nsfs.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <errno.h>
static unsigned calls,fail_at,waits,info_reads;
static const char *report;static unsigned char info_bytes[128];static sl_linux_call info_calls[2];static int next_fd,exit_after,mismatch;
static sl_linux_call *failed_original;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 c->source=s;c->number=n;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);calls++;
 long result=0;
 if(calls==fail_at){result=-EACCES;failed_original=c;}
 else if(n==SYS_waitid){waits++;if(exit_after&&waits==2)((siginfo_t *)d)->si_pid=12345;}
 else if(n==SYS_openat)result=next_fd++;
 else if(n==SYS_fstatfs)((struct statfs *)b)->f_type=a<=23?PROC_SUPER_MAGIC:NSFS_MAGIC;
 else if(n==SYS_ioctl&&b==NS_GET_USERNS)result=next_fd++;
 else if(n==SYS_ioctl&&b==NS_GET_NSTYPE)result=a==24?CLONE_NEWCGROUP:CLONE_NEWUSER;
 else if(n==SYS_read){info_reads++;result=info_reads==1?(long)strlen(report):0;if(result){assert(result<=d);memcpy((void *)b,report,result);}}
 else if(n==SYS_close)result=0;
 else if(n==SYS_statx){
  struct statx *out=(void *)f;out->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;out->stx_mode=S_IFREG;
  out->stx_ino=a==24?300:200+(mismatch&&a==25);out->stx_mnt_id=77;out->stx_dev_major=1;out->stx_dev_minor=2;
 }
 c->result=result;atomic_store(&c->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){assert(fd>=3&&!lift->attempted);lift->attempted=1;lift->original_fd=lift->held_fd=fd;return fd;}
#include "../controller_linux.c"
#include "../pidfd_proc_linux.c"
#include "../namespace_parent_linux.c"
static void setup(sl_linux_parent_namespace *o,sl_linux_birth *birth,sl_linux_bootstrap *b,sl_linux_authority_inspection *transition){
 memset(o,0,sizeof *o);memset(birth,0,sizeof *birth);memset(b,0,sizeof *b);memset(transition,0,sizeof *transition);
 calls=waits=info_reads=0;report="Pid:\t54321\nNSpid:\t54321 12345\n";memset(info_bytes,0,sizeof info_bytes);memset(info_calls,0,sizeof info_calls);next_fd=20;failed_original=0;exit_after=mismatch=0;
 birth->bootstrap=b;birth->attempted=1;b->pidfd=9;b->clone_parent.result=12345;atomic_init(&b->clone_parent.returned,1);
 transition->birth_original=birth;transition->authority_original=&b->authority;transition->returned=1;
 b->authority.namespaces_after[0]=(struct statx){.stx_ino=200,.stx_mnt_id=77,.stx_dev_major=1,.stx_dev_minor=2};
 b->authority.namespaces_after[1]=(struct statx){.stx_ino=300,.stx_mnt_id=77,.stx_dev_major=1,.stx_dev_minor=2};
 o->birth_original=birth;o->transition_original=transition;
 o->pidfd_info.bytes=info_bytes;o->pidfd_info.capacity=sizeof info_bytes;o->pidfd_info.reads=info_calls;o->pidfd_info.read_capacity=2;
}
int main(void){
 sl_linux_parent_namespace o;sl_linux_birth birth;sl_linux_bootstrap b;sl_linux_authority_inspection transition;
 fail_at=0;setup(&o,&birth,&b,&transition);assert(sl_linux_observe_parent_namespace(&o)==0&&waits==2);
 assert(!strcmp(o.pid_locator,"54321")&&b.clone_parent.result==12345&&o.views[4].raw_fd==25&&o.views[4].acquire.arguments[1]==NS_GET_USERNS);
 unsigned full=calls;assert(sl_linux_observe_parent_namespace(&o)<0&&calls==full);
 for(unsigned at=1;at<=full;at++){
  setup(&o,&birth,&b,&transition);fail_at=at;
  assert(sl_linux_observe_parent_namespace(&o)==-EACCES&&calls>=at&&calls<=at+1&&failed_original&&failed_original->result==-EACCES);
  unsigned retained=calls;if(calls==at+1)assert(o.pidfd_info.close.source&&o.pidfd_info.close.result==0);
  assert(sl_linux_observe_parent_namespace(&o)<0&&calls==retained);
 }
 fail_at=0;setup(&o,&birth,&b,&transition);exit_after=1;
 assert(sl_linux_observe_parent_namespace(&o)<0&&o.root_after.information.si_pid==12345&&o.views[5].fd==26);
 setup(&o,&birth,&b,&transition);mismatch=1;
 assert(sl_linux_observe_parent_namespace(&o)<0&&o.views[4].identity.stx_ino==201&&o.views[5].identity.stx_ino==200);
 setup(&o,&birth,&b,&transition);o.views[2].identity_query.number=SYS_statx;
 assert(sl_linux_observe_parent_namespace(&o)<0&&!calls&&o.views[2].identity_query.number==SYS_statx);
 const char *bad[]={"Pid: 0\n","Pid: -1\n","Pid: 42\nPid: 42\n","Pid: 42"};
 for(unsigned i=0;i<4;i++){
  setup(&o,&birth,&b,&transition);report=bad[i];
  assert(sl_linux_observe_parent_namespace(&o)<0&&o.pidfd_info.eof&&o.pidfd_info.close.result==0&&o.views[1].fd==-1);
 }
 setup(&o,&birth,&b,&transition);o.pidfd_info.capacity=strlen(report);
 assert(sl_linux_observe_parent_namespace(&o)<0&&!o.pidfd_info.eof&&info_reads==1&&o.views[1].fd==-1);
 return 0;
}
#endif