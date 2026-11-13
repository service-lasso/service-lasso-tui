/* UNRUN substituted native acquisition cases; no real cgroup/profile proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../cgroup_children_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <errno.h>
#include <fcntl.h>
static unsigned calls,reads,candidate_opens,unknown,wrong_mount;static unsigned char raw[128];static unsigned long raw_length;
static sl_linux_cgroup_children *active;
static unsigned long entry(unsigned long at,const char *name){
 unsigned short extent=(unsigned short)(20+strlen(name));memset(raw+at,0,extent);
 memcpy(raw+at+16,&extent,2);raw[at+18]=4;memcpy(raw+at+19,name,strlen(name)+1);return at+extent;
}
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 calls++;c->source=s;c->number=n;long args[]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);long result=0;
 if(n==SYS_openat2){
  struct open_how *how=(void*)d;assert(how->flags==(O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC));
  if(!strcmp((char*)b,".")){assert(a==8);result=20;active->frames=NULL;active->views=NULL;active->frame_capacity=0;}
  else{candidate_opens++;assert(active->eof&&a==20);
   result=!strcmp((char*)b,"cgroup.procs")?-ENOTDIR:unknown?-ENOENT:30;}
 }else if(n==SYS_fstatfs)((struct statfs*)b)->f_type=CGROUP2_SUPER_MAGIC;
 else if(n==SYS_statx){struct statx *id=(void*)f;id->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;id->stx_mode=S_IFDIR;id->stx_ino=a==20?11:99;id->stx_mnt_id=wrong_mount?32:31;}
 else{assert(n==SYS_getdents64);if(!reads++){assert(raw_length<=(unsigned long)d);memcpy((void*)b,raw,raw_length);result=raw_length;}else result=0;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){assert(fd>=3);l->attempted=1;l->original_fd=l->held_fd=fd;return fd;}
#include "../cgroup_children_linux.c"
static sl_linux_child_frame frames[2];static sl_linux_child_view views[8];static unsigned char buffers[2][128];
static void setup(sl_linux_cgroup_children *o,sl_linux_mount_location *l){
 memset(o,0,sizeof *o);memset(l,0,sizeof *l);memset(frames,0,sizeof frames);memset(views,0,sizeof views);memset(buffers,0,sizeof buffers);memset(raw,0,sizeof raw);
 l->returned=1;l->member_fd=8;l->member_state.stx_ino=11;l->member_state.stx_mnt_id=31;
 o->location_original=l;o->frames=frames;o->frame_capacity=2;o->views=views;o->view_capacity=8;
 for(unsigned i=0;i<2;i++){frames[i].bytes=buffers[i];frames[i].capacity=128;}
 calls=reads=candidate_opens=unknown=wrong_mount=0;active=o;
 raw_length=entry(0,".");raw_length=entry(raw_length,"..");raw_length=entry(raw_length,"cgroup.procs");raw_length=entry(raw_length,"target");
}
int main(void){
 sl_linux_cgroup_children o;sl_linux_mount_location l;setup(&o,&l);
 assert(sl_linux_acquire_cgroup_children(&o)==0&&o.eof&&o.view_count==4&&o.candidate_count==1&&candidate_opens==2);
 assert(o.actual_frames==frames&&o.actual_views==views&&views[2].not_directory&&views[2].open.result==-ENOTDIR);
 assert(views[3].fd==30&&views[3].frame_original==&frames[0]&&!strcmp((char*)frames[0].actual_bytes+views[3].name_offset,"target"));
 unsigned prior=calls;assert(sl_linux_acquire_cgroup_children(&o)<0&&calls==prior);
 setup(&o,&l);o.frame_capacity=1;assert(sl_linux_acquire_cgroup_children(&o)<0&&!o.eof&&!candidate_opens&&frames[0].read.result==(long)raw_length);
 setup(&o,&l);unknown=1;assert(sl_linux_acquire_cgroup_children(&o)==-ENOENT&&views[3].open.result==-ENOENT&&o.eof);
 setup(&o,&l);wrong_mount=1;assert(sl_linux_acquire_cgroup_children(&o)<0&&!reads&&!candidate_opens&&o.fd==20);
 setup(&o,&l);raw_length=entry(0,"target");raw_length=entry(raw_length,"target");assert(sl_linux_acquire_cgroup_children(&o)<0&&!candidate_opens&&o.view_count==1);
 setup(&o,&l);raw[16]=1;raw[17]=0;assert(sl_linux_acquire_cgroup_children(&o)<0&&!candidate_opens&&frames[0].returned);
 setup(&o,&l);o.view_capacity=1;assert(sl_linux_acquire_cgroup_children(&o)<0&&!candidate_opens&&o.view_count==1);
 return 0;
}
#endif