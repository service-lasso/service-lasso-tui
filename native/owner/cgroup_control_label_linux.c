#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "cgroup_control_label_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdint.h>
static int control_separate(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!an||!bn||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 0;return x+an<=y||y+bn<=x;
}
long sl_linux_acquire_cgroup_control_label(sl_linux_cgroup_control_label *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;o->raw_fd=o->fd=-1;
 o->actual_children=o->children_original;o->actual_view=o->view_original;o->actual_kind=o->kind;o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;
 sl_linux_cgroup_children *children=o->actual_children;sl_linux_child_view *v=o->actual_view;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 if(!children||!children->attempted||!children->returned||children->result||!children->eof||
    !children->actual_views||children->view_count>children->actual_view_capacity||!v||!o->actual_bytes||o->actual_capacity<2||o->actual_capacity>LONG_MAX||o->returned||o->framing_complete||
    !control_separate(o,sizeof *o,o->actual_bytes,o->actual_capacity)||!control_separate(v,sizeof *v,o->actual_bytes,o->actual_capacity))goto done;
 unsigned found=0;for(unsigned long i=0;i<children->view_count;i++)if(&children->actual_views[i]==v){found=1;break;}
 if(!found||v->dot||v->not_directory||!v->attempted||!v->returned||v->result||v->fd<3||
    v->filesystem.f_type!=CGROUP2_SUPER_MAGIC||v->filesystem_query.result||v->identity_query.result||
    (v->identity.stx_mask&mask)!=mask||!S_ISDIR(v->identity.stx_mode))goto done;
 sl_linux_call *calls[4]={&o->open,&o->filesystem_query,&o->identity_query,&o->label_query};
 for(unsigned i=0;i<4;i++)if(calls[i]->source||calls[i]->number)goto done;
 for(unsigned i=0;i<4;i++){atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);}
 if(o->actual_kind!=SL_CGROUP_PROCS&&o->actual_kind!=SL_CGROUP_THREADS)goto done;
 const char *name=o->actual_kind==SL_CGROUP_PROCS?"cgroup.procs":"cgroup.threads";
 for(unsigned i=0;i<sizeof o->name;i++){o->name[i]=name[i];if(!name[i])break;}
 const char attribute[]="security.selinux";for(unsigned i=0;i<sizeof attribute;i++)o->attribute[i]=attribute[i];
 o->how=(struct open_how){.flags=O_RDONLY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};
 long result=sl_linux_call6(&o->open,"cgroup.control.readonly.open",SYS_openat2,v->fd,(long)o->name,(long)&o->how,sizeof o->how,0,0);
 o->result=result;if(result<0||result>INT_MAX){if(result>INT_MAX)o->result=-1;goto done;}o->raw_fd=(int)result;
 result=sl_linux_lift_internal_fd(&o->lift,o->raw_fd);o->result=result;if(result<0)goto done;o->fd=(int)result;
 result=sl_linux_call6(&o->filesystem_query,"cgroup.control.fstatfs",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);
 o->result=result;if(result||o->filesystem.f_type!=CGROUP2_SUPER_MAGIC){if(!result)o->result=-1;goto done;}
 result=sl_linux_call6(&o->identity_query,"cgroup.control.statx",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0);
 o->result=result;if(result||(o->identity.stx_mask&mask)!=mask||!S_ISREG(o->identity.stx_mode)||o->identity.stx_mnt_id!=v->identity.stx_mnt_id){if(!result)o->result=-1;goto done;}
 o->raw_label_count=sl_linux_call6(&o->label_query,"cgroup.control.security.selinux",SYS_fgetxattr,o->fd,(long)o->attribute,(long)o->actual_bytes,o->actual_capacity,0,0);
 o->result=o->raw_label_count;if(o->raw_label_count<0)goto done;
 if(o->raw_label_count<2||(unsigned long)o->raw_label_count>o->actual_capacity){o->result=-1;goto done;}
 o->raw_extent=(unsigned long)o->raw_label_count;
 for(unsigned long i=0;i<o->raw_extent;i++){o->examined=i+1;if((i+1==o->raw_extent)!=(o->actual_bytes[i]==0)){o->result=-1;goto done;}}
 o->extent=o->raw_extent-1;o->framing_complete=1;o->result=0;
 done:o->returned=1;return o->result;
 /* fgetxattr security dispatch/epoch and future/control/actor authority need
  * the selected actual kernel/profile. No partial descriptor is discarded. */
}
#endif