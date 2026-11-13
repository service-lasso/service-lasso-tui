#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "exec_label_linux.h"
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
static int separate(const void *a,size_t an,const void *b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(!a||!b||!an||!bn||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 0;return x+an<=y||y+bn<=x;}
long sl_linux_observe_exec_label(sl_linux_exec_label *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->fd=-1;
 o->actual_installation=o->installation_original;o->actual_payload=o->payload_original;o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;
 if((!o->actual_installation)==(!o->actual_payload)||!o->actual_bytes||o->actual_capacity<2||o->actual_capacity>LONG_MAX||!separate(o,sizeof *o,o->actual_bytes,o->actual_capacity))goto done;
 if(o->actual_installation){sl_linux_local_installation *p=o->actual_installation;if(!p->returned||p->result||p->public_fd<3||!separate(p,sizeof *p,o->actual_bytes,o->actual_capacity))goto done;o->fd=p->public_fd;}
 else{sl_linux_immutable_payload *p=o->actual_payload;if(!p->returned||p->result||!p->source_eof||!p->seals_observed||!p->readback_complete||p->fd<3||!separate(p,sizeof *p,o->actual_bytes,o->actual_capacity))goto done;o->fd=p->fd;}
 sl_linux_call *calls[3]={&o->before,&o->label,&o->after};for(unsigned i=0;i<3;i++)if(calls[i]->source||calls[i]->number)goto done;
 for(unsigned i=0;i<3;i++){atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);}
 const char name[]="security.selinux";for(unsigned i=0;i<sizeof name;i++)o->attribute[i]=name[i];
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 o->before_result=sl_linux_call6(&o->before,"exec.label.statx.before",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->before_state,0);
 if(o->before_result){o->result=o->before_result;goto done;}
 if((o->before_state.stx_mask&mask)!=mask||!S_ISREG(o->before_state.stx_mode)||!o->before_state.stx_ino)goto done;
 o->raw_count=sl_linux_call6(&o->label,"exec.label.security.selinux",SYS_fgetxattr,o->fd,(long)o->attribute,(long)o->actual_bytes,o->actual_capacity,0,0);
 /* Independent same-file observation also runs on returned xattr failure. */
 o->after_result=sl_linux_call6(&o->after,"exec.label.statx.after",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->after_state,0);
 if(o->raw_count<0){o->result=o->raw_count;goto done;}if(o->after_result){o->result=o->after_result;goto done;}
 if((o->after_state.stx_mask&mask)!=mask||!S_ISREG(o->after_state.stx_mode)||o->before_state.stx_ino!=o->after_state.stx_ino||o->before_state.stx_mnt_id!=o->after_state.stx_mnt_id||o->before_state.stx_dev_major!=o->after_state.stx_dev_major||o->before_state.stx_dev_minor!=o->after_state.stx_dev_minor)goto done;
 if(o->raw_count<2||(unsigned long)o->raw_count>o->actual_capacity)goto done;
 for(unsigned long i=0;i<(unsigned long)o->raw_count;i++){o->examined=i+1;if((i+1==(unsigned long)o->raw_count)!=(o->actual_bytes[i]==0))goto done;}
 o->extent=(unsigned long)o->raw_count-1;o->framing_complete=1;o->result=0;
 done:o->returned=1;return o->result;
 /* No FD disposition or label/loaded-policy authority follows this observation. */
}
#endif