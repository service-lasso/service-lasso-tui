#if defined(__linux__) && defined(__x86_64__)
#include "mount_location_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
static int location_fresh(sl_linux_call *c){
 if(c->source||c->number||atomic_load(&c->entered)||atomic_load(&c->returned))return -1;
 atomic_init(&c->entered,0);atomic_init(&c->returned,0);return 0;
}
static int location_decode(const sl_linux_local_mounts *m,sl_linux_mount_field f,char *out,unsigned long capacity,unsigned long *length){
 if(f.offset>m->length||f.length>m->length-f.offset||!f.length)return -1;
 unsigned long n=0;for(unsigned long i=0;i<f.length;i++){
  unsigned char value=m->actual_bytes[f.offset+i];
  if(value=='\\'){
   if(f.length-i<4)return -1;
   unsigned char a=m->actual_bytes[f.offset+i+1],b=m->actual_bytes[f.offset+i+2],c=m->actual_bytes[f.offset+i+3];
   if(!((a=='0'&&b=='4'&&c=='0')||(a=='0'&&b=='1'&&(c=='1'||c=='2'))||(a=='1'&&b=='3'&&c=='4')))return -1;
   value=(unsigned char)((a-'0')*64+(b-'0')*8+c-'0');i+=3;
  }
  if(!value||n+1>=capacity)return -1;out[n++]=(char)value;
 }
 if(!n||out[0]!='/')return -1;out[n]=0;*length=n;return 0;
}
static int location_relative(sl_linux_mount_location *o){
 sl_linux_local_membership *m=o->actual_membership;unsigned long prefix=o->root_length;
 if(!m->prepared_path||!m->path_length||m->path_length>=m->prepared_path_capacity||m->prepared_path[m->path_length])return -1;
 if(prefix==1&&o->actual_root[0]=='/')prefix=0;
 if(prefix>m->path_length){o->outside_mount=1;return 1;}
 for(unsigned long i=0;i<prefix;i++)if(o->actual_root[i]!=m->prepared_path[i]){o->outside_mount=1;return 1;}
 if(prefix<m->path_length&&m->prepared_path[prefix]!='/'){o->outside_mount=1;return 1;}
 unsigned long start=prefix;if(start<m->path_length&&m->prepared_path[start]=='/')start++;
 unsigned long length=m->path_length-start;
 if(!length){if(o->actual_capacity<2)return -1;o->actual_relative[0]='.';o->actual_relative[1]=0;o->relative_length=1;return 0;}
 if(length>=o->actual_capacity)return -1;
 unsigned long component=0;
 for(unsigned long i=0;i<length;i++){
  char value=m->prepared_path[start+i];if(!value)return -1;o->actual_relative[i]=value;
  if(value=='/'||i+1==length){unsigned long end=value=='/'?i:i+1,size=end-component;
   if(!size||(size==1&&o->actual_relative[component]=='.')||(size==2&&o->actual_relative[component]=='.'&&o->actual_relative[component+1]=='.'))return -1;component=i+1;}
 }
 o->actual_relative[length]=0;o->relative_length=length;return 0;
}
static long location_hold(sl_linux_call *call,sl_linux_fd_lift *lift,int *fd,const char *source,int directory,const char *path,struct open_how *how){
 long raw=sl_linux_call6(call,source,SYS_openat2,directory,(long)path,(long)how,sizeof *how,0,0);
 if(raw<0)return raw;if(raw>INT_MAX)return -1;
 long result=sl_linux_lift_internal_fd(lift,(int)raw);if(result<0)return result;*fd=(int)result;return 0;
}
static long location_observe(sl_linux_call *fs_call,sl_linux_call *id_call,int fd,struct statfs *fs,struct statx *id){
 long result=sl_linux_call6(fs_call,"startup.mount-location.cgroup2",SYS_fstatfs,fd,(long)fs,0,0,0,0);
 if(result)return result<0?result:-1;if(fs->f_type!=CGROUP2_SUPER_MAGIC)return -1;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 result=sl_linux_call6(id_call,"startup.mount-location.held-identity",SYS_statx,fd,(long)"",AT_EMPTY_PATH,mask,(long)id,0);
 if(result)return result<0?result:-1;
 return (id->stx_mask&mask)==mask&&S_ISDIR(id->stx_mode)&&id->stx_ino?0:-1;
}
long sl_linux_acquire_mount_location(sl_linux_mount_location *o){
 long result=-1;if(!o||o->attempted)return -1;o->attempted=1;o->mount_fd=o->member_fd=-1;
 o->actual_mounts=o->mounts_original;o->actual_root=o->root;o->actual_mountpoint=o->mountpoint;o->actual_relative=o->relative;o->actual_capacity=o->capacity;
 sl_linux_local_mounts *m=o->actual_mounts;
 if(!m||!m->returned||m->result||!m->eof||!m->actual_rows||!m->actual_bytes||o->row_index>=m->row_count||
    !o->actual_root||!o->actual_mountpoint||!o->actual_relative||!o->actual_capacity||o->mount_lift.attempted||o->member_lift.attempted)goto done;
 o->actual_row=&m->actual_rows[o->row_index];o->actual_membership=m->actual_membership;
 if(!o->actual_membership||!o->actual_membership->returned||o->actual_membership->result||!o->actual_membership->eof)goto done;
 if(location_fresh(&o->mount_open)||location_fresh(&o->member_open)||location_fresh(&o->mount_filesystem)||location_fresh(&o->member_filesystem)||
    location_fresh(&o->mount_identity)||location_fresh(&o->member_identity))goto done;
 if(location_decode(m,o->actual_row->root,o->actual_root,o->actual_capacity,&o->root_length)||
    location_decode(m,o->actual_row->mountpoint,o->actual_mountpoint,o->actual_capacity,&o->mountpoint_length))goto done;
 result=location_relative(o);if(result)goto done;
 o->mount_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC,.resolve=RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS};
 result=location_hold(&o->mount_open,&o->mount_lift,&o->mount_fd,"startup.mount-location.open-root",AT_FDCWD,o->actual_mountpoint,&o->mount_how);if(result)goto done;
 result=location_observe(&o->mount_filesystem,&o->mount_identity,o->mount_fd,&o->mount_fs,&o->mount_state);if(result)goto done;
 if(o->mount_state.stx_mnt_id!=o->actual_row->mount_id){result=-1;goto done;}
 o->member_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};
 result=location_hold(&o->member_open,&o->member_lift,&o->member_fd,"startup.mount-location.open-membership",o->mount_fd,o->actual_relative,&o->member_how);if(result)goto done;
 result=location_observe(&o->member_filesystem,&o->member_identity,o->member_fd,&o->member_fs,&o->member_state);if(result)goto done;
 if(o->member_state.stx_mnt_id!=o->mount_state.stx_mnt_id){result=-1;goto done;}
 result=0;
done:o->result=result;o->returned=1;return result;
 /* No close: these SAME held views are still input to profile/target selection.
  * Actual eventual borrowers/disposition must settle before independent once-close. */
}
#endif