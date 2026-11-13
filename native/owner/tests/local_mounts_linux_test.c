/* UNRUN controlled mountinfo framing/extent cases, not actual mount/profile proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../local_mounts_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <errno.h>
#include <fcntl.h>
static const char *body;static unsigned calls,read_count;static long read_error,close_error;static int invalid_count;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 (void)e;(void)f;(void)g;calls++;c->source=s;c->number=n;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;
 atomic_store(&c->entered,1);long result=0;
 if(n==SYS_openat){assert(d==(O_RDONLY|O_NONBLOCK|O_CLOEXEC));result=8;}
 else if(n==SYS_fstatfs)((struct statfs*)b)->f_type=PROC_SUPER_MAGIC;
 else if(n==SYS_read){read_count++;result=read_error?read_error:invalid_count?d+1:read_count==1?(long)strlen(body):0;if(result>0&&result<=d)memcpy((void*)b,body,result);}
 else{assert(n==SYS_close&&a==8);result=close_error;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){l->attempted=1;l->original_fd=l->held_fd=fd;return fd;}
#include "../local_mounts_linux.c"
static unsigned char bytes[512];static sl_linux_call reads[3];static sl_linux_mount_row rows[2];
static void setup(sl_linux_local_mounts *o,sl_linux_local_membership *m,const char *text){
 memset(o,0,sizeof *o);memset(m,0,sizeof *m);memset(reads,0,sizeof reads);memset(rows,0,sizeof rows);memset(bytes,0,sizeof bytes);
 m->returned=m->eof=1;o->membership_original=m;o->bytes=bytes;o->capacity=sizeof bytes;o->reads=reads;o->read_capacity=3;o->rows=rows;o->row_capacity=2;
 body=text;calls=read_count=0;read_error=close_error=invalid_count=0;
}
int main(void){
 sl_linux_local_mounts o;sl_linux_local_membership m;
 const char *valid="31 20 0:28 /owner /sys/fs/cgroup rw,nosuid - cgroup2 cgroup rw,nsdelegate\n";
 setup(&o,&m,valid);assert(sl_linux_observe_local_mounts(&o)==0&&o.eof&&o.row_count==1&&o.close.result==0);
 assert(o.actual_bytes==bytes&&o.actual_reads==reads&&o.actual_rows==rows&&rows[0].mount_id==31);
 assert(rows[0].root.length==6&&!memcmp(bytes+rows[0].root.offset,"/owner",6));
 unsigned prior=calls;assert(sl_linux_observe_local_mounts(&o)<0&&calls==prior);
 const char *bad[]={"31 20 0:28 /owner /sys rw - cgroup2 cgroup rw", "31 20 0:28 /owner /sys rw - cgroup2 cgroup\n", "31 20 0:28 /bad\\777 /sys rw - cgroup2 cgroup rw\n", "31 20 0:28 /owner /sys rw - tmpfs tmpfs rw\n"};
 for(unsigned i=0;i<4;i++){setup(&o,&m,bad[i]);assert(sl_linux_observe_local_mounts(&o)<0&&o.eof&&o.close.result==0);}
 setup(&o,&m,valid);o.capacity=strlen(valid);assert(sl_linux_observe_local_mounts(&o)<0&&!o.eof&&read_count==1&&o.close.result==0);
 setup(&o,&m,valid);read_error=-EIO;close_error=-EINTR;assert(sl_linux_observe_local_mounts(&o)==-EIO&&reads[0].result==-EIO&&o.close.result==-EINTR);
 prior=calls;assert(sl_linux_observe_local_mounts(&o)<0&&calls==prior);
 setup(&o,&m,valid);invalid_count=1;assert(sl_linux_observe_local_mounts(&o)<0&&o.length==0&&reads[0].result==(long)sizeof(bytes)+1);
 char twice[256];unsigned long extent=strlen(valid);memcpy(twice,valid,extent);memcpy(twice+extent,valid,extent+1);
 setup(&o,&m,twice);assert(sl_linux_observe_local_mounts(&o)<0&&o.row_count==1&&o.eof);
 setup(&o,&m,valid);o.read_capacity=1;assert(sl_linux_observe_local_mounts(&o)<0&&!o.eof&&o.read_count==1&&o.close.result==0);
 setup(&o,&m,valid);o.row_capacity=0;assert(sl_linux_observe_local_mounts(&o)<0&&!calls);
 sl_linux_mount_row selinux_rows[1]={0};
 const char *both="31 20 0:28 /owner /sys/fs/cgroup rw - cgroup2 cgroup rw,nsdelegate\n44 20 0:30 / /sys/fs/selinux rw - selinuxfs selinuxfs rw\n";
 setup(&o,&m,both);o.selinux_rows=selinux_rows;o.selinux_capacity=1;
 assert(sl_linux_observe_local_mounts(&o)==0&&o.eof&&o.row_count==1&&o.selinux_count==1&&o.actual_selinux_rows==selinux_rows&&selinux_rows[0].mount_id==44);
 setup(&o,&m,both);assert(sl_linux_observe_local_mounts(&o)<0&&o.eof&&o.row_count==1&&!o.selinux_count);
 return 0;
}
#endif