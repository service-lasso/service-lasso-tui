#if defined(__linux__) && defined(__x86_64__)
#include "cgroup_children_linux.h"
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
static int child_fresh(sl_linux_call *c){
 if(c->source||c->number||atomic_load(&c->entered)||atomic_load(&c->returned))return -1;
 atomic_init(&c->entered,0);atomic_init(&c->returned,0);return 0;
}
static long child_identity(sl_linux_call *fs_call,sl_linux_call *id_call,int fd,struct statfs *fs,struct statx *id){
 long r=sl_linux_call6(fs_call,"startup.children.filesystem",SYS_fstatfs,fd,(long)fs,0,0,0,0);
 if(r)return r<0?r:-1;if(fs->f_type!=CGROUP2_SUPER_MAGIC)return -1;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 r=sl_linux_call6(id_call,"startup.children.identity",SYS_statx,fd,(long)"",AT_EMPTY_PATH,mask,(long)id,0);
 if(r)return r<0?r:-1;return (id->stx_mask&mask)==mask&&S_ISDIR(id->stx_mode)&&id->stx_ino?0:-1;
}
static int child_parse(sl_linux_cgroup_children *o,sl_linux_child_frame *f){
 unsigned long at=0;
 while(at<f->length){
  unsigned short record;unsigned long remaining=f->length-at;
  /* Linux getdents64 header: ino64/off64/reclen16/type8/name. Raw original
   * bytes stay in the nonreused frame; no alignment-cast replaces them. */
  if(remaining<20)return -1;memcpy(&record,f->actual_bytes+at+16,sizeof record);
  if(record<20||record>remaining)return -1;
  unsigned long name=at+19,end=name;
  while(end<at+record&&f->actual_bytes[end]){if(f->actual_bytes[end]=='/')return -1;end++;}
  if(end==at+record||end==name)return -1;
  if(o->view_count==o->actual_view_capacity)return -1;
  for(unsigned long i=0;i<o->view_count;i++){
   sl_linux_child_view *v=&o->actual_views[i];
   if(v->name_length==end-name&&!memcmp(v->frame_original->actual_bytes+v->name_offset,f->actual_bytes+name,end-name))return -1;
  }
  sl_linux_child_view *v=&o->actual_views[o->view_count++];
  v->frame_original=f;v->offset=at;v->name_offset=name;v->name_length=end-name;v->raw_type=f->actual_bytes[at+18];
  v->dot=(v->name_length==1&&f->actual_bytes[name]=='.')||(v->name_length==2&&f->actual_bytes[name]=='.'&&f->actual_bytes[name+1]=='.');
  at+=record;
 }
 return 0;
}
long sl_linux_acquire_cgroup_children(sl_linux_cgroup_children *o){
 long result=-1;if(!o||o->attempted)return -1;o->attempted=1;o->fd=-1;
 o->actual_location=o->location_original;o->actual_frames=o->frames;o->actual_views=o->views;
 o->actual_frame_capacity=o->frame_capacity;o->actual_view_capacity=o->view_capacity;
 sl_linux_mount_location *location=o->actual_location;
 if(!location||!location->returned||location->result||location->member_fd<3||!o->actual_frames||!o->actual_views||
    !o->actual_frame_capacity||!o->actual_view_capacity||o->frame_count||o->view_count||o->eof||o->lift.attempted||
    child_fresh(&o->open)||child_fresh(&o->filesystem_query)||child_fresh(&o->identity_query)||child_fresh(&o->close))goto done;
 for(unsigned long i=0;i<o->actual_frame_capacity;i++){
  sl_linux_child_frame *f=&o->actual_frames[i];
  if(f->attempted||f->returned||f->actual_bytes||f->length||!f->bytes||f->capacity<20||f->capacity>INT_MAX||child_fresh(&f->read))goto done;
  f->actual_bytes=f->bytes;f->actual_capacity=f->capacity;
 }
 for(unsigned long i=0;i<o->actual_view_capacity;i++){
  sl_linux_child_view *v=&o->actual_views[i];
  if(v->frame_original||v->attempted||v->returned||v->lift.attempted||child_fresh(&v->open)||child_fresh(&v->filesystem_query)||child_fresh(&v->identity_query)||child_fresh(&v->close))goto done;
  v->fd=-1;
 }
 o->how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};
 result=sl_linux_call6(&o->open,"startup.children.independent-directory",SYS_openat2,location->member_fd,(long)".",(long)&o->how,sizeof o->how,0,0);
 if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&o->lift,(int)result);if(result<0)goto done;o->fd=(int)result;
 result=child_identity(&o->filesystem_query,&o->identity_query,o->fd,&o->filesystem,&o->identity);if(result)goto done;
 if(o->identity.stx_ino!=location->member_state.stx_ino||o->identity.stx_mnt_id!=location->member_state.stx_mnt_id||
    o->identity.stx_dev_major!=location->member_state.stx_dev_major||o->identity.stx_dev_minor!=location->member_state.stx_dev_minor){result=-1;goto done;}
 while(o->frame_count<o->actual_frame_capacity){
  sl_linux_child_frame *f=&o->actual_frames[o->frame_count++];f->attempted=1;
  result=sl_linux_call6(&f->read,"startup.children.getdents64",SYS_getdents64,o->fd,(long)f->actual_bytes,f->actual_capacity,0,0,0);
  f->returned=1;if(result<0)goto done;if((unsigned long)result>f->actual_capacity){result=-1;goto done;}
  f->length=(unsigned long)result;if(!result){o->eof=1;break;}
  if(child_parse(o,f)){result=-1;goto done;}
 }
 if(!o->eof){result=-1;goto done;}
 /* No candidate acquisition until the actual bounded directory EOF. d_type
  * never decides authority or skips an actual candidate-directory observation. */
 for(unsigned long i=0;i<o->view_count;i++){
  sl_linux_child_view *v=&o->actual_views[i];if(v->dot)continue;v->attempted=1;v->how=o->how;
  result=sl_linux_call6(&v->open,"startup.children.direct-candidate",SYS_openat2,o->fd,(long)(v->frame_original->actual_bytes+v->name_offset),(long)&v->how,sizeof v->how,0,0);
  if(result==-ENOTDIR){v->not_directory=1;v->result=result;v->returned=1;continue;}
  if(result<0){v->result=result;v->returned=1;goto done;}if(result>INT_MAX){result=-1;v->result=result;v->returned=1;goto done;}
  result=sl_linux_lift_internal_fd(&v->lift,(int)result);
  if(result>=0){v->fd=(int)result;result=child_identity(&v->filesystem_query,&v->identity_query,v->fd,&v->filesystem,&v->identity);}
  if(!result&&v->identity.stx_mnt_id!=o->identity.stx_mnt_id)result=-1;
  v->result=result;v->returned=1;if(result)goto done;o->candidate_count++;
 }
 result=0;
 done:o->result=result;o->returned=1;return result;
 /* Every partial lift/raw view remains retained independently of semantic
  * failure. No cleanup retry, membership mutation or native gate is implied. */
}
#endif