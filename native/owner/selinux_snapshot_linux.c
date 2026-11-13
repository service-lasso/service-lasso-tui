#if defined(__linux__) && defined(__x86_64__)
#include "selinux_snapshot_linux.h"
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
static int policy_fresh(sl_linux_call *c){
 if(c->source||c->number||atomic_load(&c->entered)||atomic_load(&c->returned))return -1;
 atomic_init(&c->entered,0);atomic_init(&c->returned,0);return 0;
}
static int policy_overlap(const void *a,unsigned long n,const void *b,unsigned long m){
 uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
 if(!a||!b||!n||!m||n>UINTPTR_MAX-aa||m>UINTPTR_MAX-bb)return 1;
 return aa<bb+m&&bb<aa+n;
}
static int policy_mount_overlap(sl_linux_selinux_snapshot *o,const void *p,unsigned long n){
 sl_linux_local_mounts *m=o->actual_mounts;
 return policy_overlap(p,n,m,sizeof *m)||policy_overlap(p,n,m->actual_bytes,m->length)||
        policy_overlap(p,n,m->actual_selinux_rows,m->selinux_count*sizeof *m->actual_selinux_rows);
}
static int policy_path(sl_linux_selinux_snapshot *o){
 sl_linux_mount_field f=o->actual_row->mountpoint;sl_linux_local_mounts *m=o->actual_mounts;
 if(!f.length||f.offset>m->length||f.length>m->length-f.offset||m->actual_bytes[f.offset]!='/')return -1;
 for(unsigned long at=0;at<f.length;at++){
  unsigned char c=m->actual_bytes[f.offset+at];
  if(c=='\\'){
   if(f.length-at<4)return -1;unsigned char a=m->actual_bytes[f.offset+at+1],b=m->actual_bytes[f.offset+at+2],d=m->actual_bytes[f.offset+at+3];
   if(a=='0'&&b=='4'&&d=='0')c=' ';else if(a=='0'&&b=='1'&&d=='1')c='\t';else if(a=='0'&&b=='1'&&d=='2')c='\n';else if(a=='1'&&b=='3'&&d=='4')c='\\';else return -1;at+=3;
  }
  if(!c||o->path_length>=o->actual_path_capacity-1)return -1;o->actual_path[o->path_length++]=c;
 }
 o->actual_path[o->path_length]=0;return 0;
}
static long policy_observe(sl_linux_call *fs_call,sl_linux_call *id_call,int fd,struct statfs *fs,struct statx *state,int directory){
 long r=sl_linux_call6(fs_call,"startup.selinux-snapshot.filesystem",SYS_fstatfs,fd,(long)fs,0,0,0,0);
 if(r)return r<0?r:-1;if(fs->f_type!=SELINUX_MAGIC)return -1;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 r=sl_linux_call6(id_call,"startup.selinux-snapshot.identity",SYS_statx,fd,(long)"",AT_EMPTY_PATH,mask,(long)state,0);
 if(r)return r<0?r:-1;
 return (state->stx_mask&mask)==mask&&state->stx_ino&&(directory?S_ISDIR(state->stx_mode):S_ISREG(state->stx_mode))?0:-1;
}
long sl_linux_acquire_selinux_snapshot(sl_linux_selinux_snapshot *o){
 long result=-1;if(!o||o->attempted)return -1;o->attempted=1;o->root_fd=o->policy_fd=-1;
 o->actual_mounts=o->mounts_original;o->actual_path=o->path;o->actual_path_capacity=o->path_capacity;
 o->actual_body=o->body;o->actual_body_capacity=o->body_capacity;o->actual_frames=o->frames;o->actual_frame_capacity=o->frame_capacity;
 sl_linux_local_mounts *m=o->actual_mounts;
 if(!m||!m->returned||m->result||!m->eof||!m->actual_selinux_rows||o->row_index>=m->selinux_count||
    !o->actual_path||!o->actual_path_capacity||!o->actual_body||!o->actual_body_capacity||!o->actual_frames||!o->actual_frame_capacity||
    o->actual_frame_capacity>ULONG_MAX/sizeof *o->actual_frames||o->length||o->path_length||o->frame_count||o->eof||o->root_lift.attempted||o->policy_lift.attempted||o->status.attempted||o->status.snapshot_original||o->before_policy.attempted||o->after_policy.attempted)goto done;
 if(!m->actual_bytes||!m->length||m->selinux_count>ULONG_MAX/sizeof *m->actual_selinux_rows)goto done;
 o->actual_row=&m->actual_selinux_rows[o->row_index];
 if(policy_mount_overlap(o,o,sizeof *o)||policy_mount_overlap(o,o->actual_frames,o->actual_frame_capacity*sizeof *o->actual_frames)||
    policy_mount_overlap(o,o->actual_body,o->actual_body_capacity)||policy_mount_overlap(o,o->actual_path,o->actual_path_capacity))goto done;
 if(policy_overlap(o,sizeof *o,o->actual_frames,o->actual_frame_capacity*sizeof *o->actual_frames)||
    policy_overlap(o->actual_body,o->actual_body_capacity,o,sizeof *o)||policy_overlap(o->actual_path,o->actual_path_capacity,o,sizeof *o)||
    policy_overlap(o->actual_body,o->actual_body_capacity,o->actual_frames,o->actual_frame_capacity*sizeof *o->actual_frames)||
    policy_overlap(o->actual_path,o->actual_path_capacity,o->actual_frames,o->actual_frame_capacity*sizeof *o->actual_frames)||
    policy_overlap(o->actual_body,o->actual_body_capacity,o->actual_path,o->actual_path_capacity))goto done;
 sl_linux_call *calls[]={&o->root_open,&o->root_filesystem,&o->root_identity,&o->root_close,&o->policy_open,&o->policy_filesystem,&o->policy_identity,&o->policy_close};
 for(unsigned i=0;i<8;i++)if(policy_fresh(calls[i]))goto done;
 for(unsigned long i=0;i<o->actual_frame_capacity;i++){
  sl_linux_policy_frame *f=&o->actual_frames[i];
  if(f->attempted||f->returned||f->actual_bytes||f->length||!f->bytes||!f->capacity||f->capacity>INT_MAX||policy_fresh(&f->read))goto done;
  f->actual_bytes=f->bytes;f->actual_capacity=f->capacity;
  if(policy_mount_overlap(o,f->actual_bytes,f->actual_capacity))goto done;
  if(policy_overlap(f->actual_bytes,f->actual_capacity,o,sizeof *o)||policy_overlap(f->actual_bytes,f->actual_capacity,o->actual_frames,o->actual_frame_capacity*sizeof *o->actual_frames)||
     policy_overlap(f->actual_bytes,f->actual_capacity,o->actual_body,o->actual_body_capacity)||policy_overlap(f->actual_bytes,f->actual_capacity,o->actual_path,o->actual_path_capacity))goto done;
  for(unsigned long j=0;j<i;j++)if(policy_overlap(f->actual_bytes,f->actual_capacity,o->actual_frames[j].actual_bytes,o->actual_frames[j].actual_capacity))goto done;
 }
 if(policy_path(o))goto done;
 o->status.snapshot_original=o;o->before_policy.status_original=&o->status;o->after_policy.status_original=&o->status;
 o->root_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS};
 result=sl_linux_call6(&o->root_open,"startup.selinux-snapshot.root-open",SYS_openat2,AT_FDCWD,(long)o->actual_path,(long)&o->root_how,sizeof o->root_how,0,0);
 if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}result=sl_linux_lift_internal_fd(&o->root_lift,(int)result);if(result<0)goto done;o->root_fd=(int)result;
 result=policy_observe(&o->root_filesystem,&o->root_identity,o->root_fd,&o->root_fs,&o->root_state,1);if(result)goto done;
 if(o->root_state.stx_mnt_id!=o->actual_row->mount_id){result=-1;goto done;}
 result=sl_linux_acquire_selinux_status(&o->status);if(result)goto done;
 result=sl_linux_sample_selinux_status(&o->before_policy);if(result||o->before_policy.enforcing!=1){if(!result)result=-1;goto done;}
 o->policy_how=(struct open_how){.flags=O_RDONLY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};
 result=sl_linux_call6(&o->policy_open,"startup.selinux-snapshot.policy-open",SYS_openat2,o->root_fd,(long)"policy",(long)&o->policy_how,sizeof o->policy_how,0,0);
 if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}result=sl_linux_lift_internal_fd(&o->policy_lift,(int)result);if(result<0)goto done;o->policy_fd=(int)result;
 result=policy_observe(&o->policy_filesystem,&o->policy_identity,o->policy_fd,&o->policy_fs,&o->policy_state,0);if(result)goto done;
 if(o->policy_state.stx_mnt_id!=o->root_state.stx_mnt_id){result=-1;goto done;}
 while(o->frame_count<o->actual_frame_capacity){
  sl_linux_policy_frame *f=&o->actual_frames[o->frame_count++];f->attempted=1;
  result=sl_linux_call6(&f->read,"startup.selinux-snapshot.raw-read",SYS_read,o->policy_fd,(long)f->actual_bytes,f->actual_capacity,0,0,0);f->returned=1;
  if(result<0)goto done;if((unsigned long)result>f->actual_capacity){result=-1;goto done;}f->length=(unsigned long)result;
  if(!result){o->eof=1;break;}
  if(f->length>o->actual_body_capacity-o->length){result=-1;goto done;}
  memcpy(o->actual_body+o->length,f->actual_bytes,f->length);o->length+=f->length;
 }
 result=o->eof&&o->length?0:-1;
 if(!result){result=sl_linux_sample_selinux_status(&o->after_policy);if(!result&&!sl_linux_equal_selinux_status(&o->before_policy,&o->after_policy))result=-1;}
 done:o->result=result;o->returned=1;return result;
 /* Snapshot/held policy FD is not loaded-byte authority. Status/load epoch,
  * enforcing/context/label/kernel-policy and protected maintenance relations
  * remain mandatory, with actual consumer joins before independent close. */
}
#endif