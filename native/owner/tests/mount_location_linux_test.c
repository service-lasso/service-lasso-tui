/* UNRUN controlled locator/held-object association cases, not namespace/profile qualification. */
#if defined(__linux__) && defined(__x86_64__)
#include "../mount_location_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
static unsigned calls,fail_at;static int next_fd,wrong_mount;static sl_linux_call *failure;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 calls++;c->source=s;c->number=n;long args[]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);
 long result=0;
 if(calls==fail_at){result=-EACCES;failure=c;}
 else if(n==SYS_openat2){struct open_how *how=(void*)d;assert(how->flags==(O_RDONLY|O_DIRECTORY|O_CLOEXEC));result=next_fd++;}
 else if(n==SYS_fstatfs)((struct statfs*)b)->f_type=CGROUP2_SUPER_MAGIC;
 else{assert(n==SYS_statx);struct statx *id=(void*)f;id->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;id->stx_mode=S_IFDIR;id->stx_ino=a;id->stx_mnt_id=wrong_mount?32:31;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){assert(fd>=3);l->attempted=1;l->original_fd=l->held_fd=fd;return fd;}
#include "../mount_location_linux.c"
static char roots[64],points[64],relative[64];static unsigned char body[256];static sl_linux_mount_row row;
static void setup(sl_linux_mount_location *o,sl_linux_local_mounts *m,sl_linux_local_membership *member,const char *encoded_root,const char *membership){
 memset(o,0,sizeof *o);memset(m,0,sizeof *m);memset(member,0,sizeof *member);memset(&row,0,sizeof row);
 memset(roots,0,sizeof roots);memset(points,0,sizeof points);memset(relative,0,sizeof relative);
 unsigned long root_size=strlen(encoded_root),point_size=strlen("/sys/fs/cgroup");memcpy(body,encoded_root,root_size);memcpy(body+root_size,"/sys/fs/cgroup",point_size);
 row.root=(sl_linux_mount_field){0,root_size};row.mountpoint=(sl_linux_mount_field){root_size,point_size};row.mount_id=31;
 member->returned=member->eof=1;member->prepared_path=(char*)membership;member->path_length=strlen(membership);member->prepared_path_capacity=member->path_length+1;
 m->returned=m->eof=1;m->actual_bytes=body;m->length=root_size+point_size;m->actual_rows=&row;m->row_count=1;m->membership_original=m->actual_membership=member;
 o->mounts_original=m;o->root=roots;o->mountpoint=points;o->relative=relative;o->capacity=sizeof roots;
 calls=0;fail_at=0;next_fd=8;wrong_mount=0;failure=0;
}
int main(void){
 sl_linux_mount_location o;sl_linux_local_mounts m;sl_linux_local_membership member;
 setup(&o,&m,&member,"/owner","/owner/child");assert(sl_linux_acquire_mount_location(&o)==0&&calls==6);
 assert(!strcmp(relative,"child")&&o.mount_fd==8&&o.member_fd==9&&o.actual_row==&row);
 assert(o.member_how.resolve==(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV));
 unsigned prior=calls;assert(sl_linux_acquire_mount_location(&o)<0&&calls==prior);
 for(unsigned at=1;at<=6;at++){setup(&o,&m,&member,"/owner","/owner/child");fail_at=at;assert(sl_linux_acquire_mount_location(&o)==-EACCES&&calls==at&&failure&&failure->result==-EACCES);}
 setup(&o,&m,&member,"/owner","/owner2/child");assert(sl_linux_acquire_mount_location(&o)==1&&o.outside_mount&&!calls);
 setup(&o,&m,&member,"/","/child");assert(sl_linux_acquire_mount_location(&o)==0&&!strcmp(relative,"child"));
 setup(&o,&m,&member,"/owner","/owner");assert(sl_linux_acquire_mount_location(&o)==0&&!strcmp(relative,"."));
 setup(&o,&m,&member,"/owner\\040one","/owner one/child");assert(sl_linux_acquire_mount_location(&o)==0&&!strcmp(roots,"/owner one")&&!strcmp(relative,"child"));
 setup(&o,&m,&member,"/owner","/owner/../escape");assert(sl_linux_acquire_mount_location(&o)<0&&!calls);
 setup(&o,&m,&member,"/owner","/owner/child");wrong_mount=1;assert(sl_linux_acquire_mount_location(&o)<0&&calls==3&&o.mount_state.stx_mnt_id==32&&o.mount_fd==8&&o.member_fd==-1);
 setup(&o,&m,&member,"/owner","/owner/child");o.capacity=4;assert(sl_linux_acquire_mount_location(&o)<0&&!calls);
 return 0;
}
#endif