#if defined(__linux__) && defined(__x86_64__)
#include "selinux_snapshot_linux.h"
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <limits.h>
_Static_assert(sizeof(uint32_t)==4,"selected SELinux status UAPI uses five u32 fields");
long sl_linux_acquire_selinux_status(sl_linux_selinux_status *o){
 sl_linux_selinux_snapshot *s;long result=-1;
 if(!o||o->attempted)return -1;o->attempted=1;o->fd=-1;s=o->snapshot_original;
 if(!s||s->root_fd<3||!atomic_load_explicit(&s->root_filesystem.returned,memory_order_acquire)||s->root_filesystem.result||
    s->root_fs.f_type!=SELINUX_MAGIC||!atomic_load_explicit(&s->root_identity.returned,memory_order_acquire)||s->root_identity.result||
    !(s->root_state.stx_mask&STATX_MNT_ID)||o->returned||o->actual_words)goto done;
 sl_linux_call *calls[]={&o->open,&o->filesystem,&o->identity,&o->mapping};
 for(unsigned i=0;i<4;i++){
  if(calls[i]->source||calls[i]->number||atomic_load(&calls[i]->entered)||atomic_load(&calls[i]->returned))goto done;
  atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);
 }
 o->name[0]='s';o->name[1]='t';o->name[2]='a';o->name[3]='t';o->name[4]='u';o->name[5]='s';o->name[6]=0;
 o->how.flags=O_RDONLY|O_NONBLOCK|O_CLOEXEC;
 o->how.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV;
 o->raw_fd=sl_linux_call6(&o->open,"selinux.status.open",SYS_openat2,s->root_fd,(long)o->name,(long)&o->how,sizeof o->how,0,0);
 result=o->raw_fd;if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&o->lift,(int)o->raw_fd);if(result<0)goto done;o->fd=(int)result;
 result=sl_linux_call6(&o->filesystem,"selinux.status.filesystem",SYS_fstatfs,o->fd,(long)&o->fs,0,0,0,0);
 if(result<0)goto done;if(result||o->fs.f_type!=SELINUX_MAGIC){result=-1;goto done;}
 result=sl_linux_call6(&o->identity,"selinux.status.identity",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH,STATX_TYPE|STATX_MNT_ID,(long)&o->state,0);
 if(result<0)goto done;if(result||!(o->state.stx_mask&STATX_TYPE)||!(o->state.stx_mask&STATX_MNT_ID)||
    (o->state.stx_mode&S_IFMT)!=S_IFREG||o->state.stx_mnt_id!=s->root_state.stx_mnt_id){result=-1;goto done;}
 o->raw_mapping=sl_linux_call6(&o->mapping,"selinux.status.mmap",SYS_mmap,0,5*sizeof(uint32_t),PROT_READ,MAP_SHARED,o->fd,0);
 result=o->raw_mapping;if(result<0)goto done;if(!result){result=-1;goto done;}
 o->actual_words=(volatile uint32_t*)result;result=0;
done:o->result=result;o->returned=1;return result;
 /* Actual page-rounded charge and all status/raw/lift/mapping lifetimes remain
  * selected native profile/disposition obligations; no unmap or close inferred. */
}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *o){
 sl_linux_selinux_status *s;volatile uint32_t *words;
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;o->actual_status=o->status_original;s=o->actual_status;
 if(!s||!s->returned||s->result||!s->actual_words)goto done;words=s->actual_words;
 o->sequence_before=__atomic_load_n(&words[1],__ATOMIC_RELAXED);
 if(o->sequence_before&1){o->result=1;goto done;}
 __atomic_thread_fence(__ATOMIC_SEQ_CST);
 o->version=__atomic_load_n(&words[0],__ATOMIC_RELAXED);o->enforcing=__atomic_load_n(&words[2],__ATOMIC_RELAXED);
 o->policyload=__atomic_load_n(&words[3],__ATOMIC_RELAXED);o->deny_unknown=__atomic_load_n(&words[4],__ATOMIC_RELAXED);o->fields_read=1;
 __atomic_thread_fence(__ATOMIC_SEQ_CST);
 o->sequence_after=__atomic_load_n(&words[1],__ATOMIC_RELAXED);
 if(o->sequence_before!=o->sequence_after||(o->sequence_after&1)){o->result=1;goto done;}
 if(o->version!=1||o->enforcing>1||o->deny_unknown>1)goto done;o->result=0;
done:o->returned=1;return o->result;
}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){
 return a&&b&&a->returned&&b->returned&&!a->result&&!b->result&&a->actual_status==b->actual_status&&
 a->version==b->version&&a->sequence_after==b->sequence_after&&a->enforcing==b->enforcing&&a->policyload==b->policyload&&a->deny_unknown==b->deny_unknown;
}
#endif