/* UNRUN controlled snapshot acquisition: no policy parse or loaded authority. */
#if defined(__linux__) && defined(__x86_64__)
#include "../selinux_snapshot_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
static uint32_t status_words[5];static unsigned status_change;static unsigned calls,reads,wrong_mount,invalid_count,empty;static long read_failure;static sl_linux_selinux_snapshot *active;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 calls++;c->source=s;c->number=n;long args[]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);long result=0;
 if(n==SYS_openat2){struct open_how *how=(void*)d;assert(how->flags&O_NONBLOCK);
  if(a==AT_FDCWD){assert(!strcmp((char*)b,"/sys/fs/selinux"));result=8;active->frames=NULL;active->body=NULL;active->path=NULL;}
  else{assert(a==8);assert(how->resolve&RESOLVE_BENEATH);if(!strcmp((char*)b,"status"))result=10;else{assert(!strcmp((char*)b,"policy"));result=9;}}
 }else if(n==SYS_fstatfs)((struct statfs*)b)->f_type=SELINUX_MAGIC;
 else if(n==SYS_statx){struct statx *state=(void*)f;state->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;state->stx_mode=a==8?S_IFDIR:S_IFREG;state->stx_ino=a;state->stx_mnt_id=wrong_mount?45:44;}
 else if(n==SYS_mmap){assert(d==PROT_READ&&e==MAP_SHARED&&f==10&&b==20);result=(long)status_words;}
 else{assert(n==SYS_read&&a==9);unsigned at=reads++;if(status_change)status_words[1]=4;
  if(invalid_count)result=d+1;else if(empty)result=0;else if(at==1&&read_failure)result=read_failure;
  else if(at==0){assert(d>=4);memcpy((void*)b,"abcd",4);result=4;}else if(at==1){assert(d>=3);memcpy((void*)b,"efg",3);result=3;}else result=0;
 }
 c->result=result;atomic_store(&c->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){l->attempted=1;l->original_fd=l->held_fd=fd;return fd;}
#include "../selinux_status_linux.c"
#include "../selinux_snapshot_linux.c"
static unsigned char frame_bytes[3][8],body[16];static char path[64];static sl_linux_policy_frame frames[3];static sl_linux_mount_row row;
static void setup(sl_linux_selinux_snapshot *o,sl_linux_local_mounts *m){
 memset(o,0,sizeof *o);memset(m,0,sizeof *m);memset(frames,0,sizeof frames);memset(frame_bytes,0,sizeof frame_bytes);memset(body,0,sizeof body);memset(path,0,sizeof path);memset(&row,0,sizeof row);
 m->returned=m->eof=1;m->actual_bytes=(unsigned char*)"/sys/fs/selinux";m->length=strlen((char*)m->actual_bytes);m->actual_selinux_rows=&row;m->selinux_count=1;row.mount_id=44;row.mountpoint=(sl_linux_mount_field){0,m->length};
 o->mounts_original=m;o->path=path;o->path_capacity=sizeof path;o->body=body;o->body_capacity=sizeof body;o->frames=frames;o->frame_capacity=3;
 for(unsigned i=0;i<3;i++){frames[i].bytes=frame_bytes[i];frames[i].capacity=8;}
 calls=reads=wrong_mount=invalid_count=empty=status_change=0;read_failure=0;active=o;status_words[0]=1;status_words[1]=2;status_words[2]=1;status_words[3]=0;status_words[4]=1;
}
int main(void){
 sl_linux_selinux_snapshot o;sl_linux_local_mounts m;setup(&o,&m);
 assert(sl_linux_acquire_selinux_snapshot(&o)==0&&o.eof&&o.length==7&&o.frame_count==3&&!memcmp(body,"abcdefg",7));
 assert(o.actual_body==body&&o.actual_frames==frames&&frames[0].actual_bytes==frame_bytes[0]&&frames[1].actual_bytes==frame_bytes[1]);
 assert(!memcmp(frame_bytes[0],"abcd",4)&&!memcmp(frame_bytes[1],"efg",3)&&o.policy_fd==9&&o.root_fd==8);
 unsigned prior=calls;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&calls==prior);
 setup(&o,&m);status_change=1;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&o.eof&&o.length==7&&o.before_policy.sequence_after==2&&o.after_policy.sequence_after==4);
 setup(&o,&m);status_words[1]=3;assert(sl_linux_acquire_selinux_snapshot(&o)==1&&!reads&&!o.policy_open.source&&o.before_policy.sequence_before==3&&!o.before_policy.fields_read);
 setup(&o,&m);status_words[2]=0;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!reads&&!o.policy_open.source);
 setup(&o,&m);status_words[0]=2;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!reads&&!o.policy_open.source&&o.before_policy.version==2);
 setup(&o,&m);o.body_capacity=6;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&o.length==4&&frames[1].read.result==3&&!o.eof);
 setup(&o,&m);o.frame_capacity=2;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!o.eof&&o.length==7&&reads==2);
 setup(&o,&m);read_failure=-EIO;assert(sl_linux_acquire_selinux_snapshot(&o)==-EIO&&frames[1].read.result==-EIO&&o.length==4);
 setup(&o,&m);invalid_count=1;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&frames[0].read.result==9&&!o.length);
 setup(&o,&m);empty=1;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&o.eof&&!o.length);
 setup(&o,&m);wrong_mount=1;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&calls==3&&!reads&&o.root_fd==8&&o.policy_fd==-1);
 setup(&o,&m);o.body=frame_bytes[0];assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!calls);
 setup(&o,&m);o.body=m.actual_bytes;o.body_capacity=m.length;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!calls&&!reads);
 setup(&o,&m);frames[1].bytes=(unsigned char*)&row;frames[1].capacity=sizeof row;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!calls&&!reads);
 setup(&o,&m);o.path=(char*)&m;o.path_capacity=sizeof m;assert(sl_linux_acquire_selinux_snapshot(&o)<0&&!calls&&!reads);
 return 0;
}
#endif