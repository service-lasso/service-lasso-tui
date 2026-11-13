#if defined(__linux__) && defined(__x86_64__)
#include "schedule_linux.h"
#include "family_events_linux.h"
#include "candidate_target_linux.h"
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
static void family_init(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
long sl_linux_observe_family_events(sl_linux_family_events *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->primary_result=o->result=-EINVAL;o->raw_fd=o->fd=-1;o->actual_target=o->target_original;
 sl_linux_candidate_target *target=o->actual_target;
 if(!target||!target->returned||target->result||!target->bound||target->target_fd<3||target->target_fs.f_type!=CGROUP2_SUPER_MAGIC||!S_ISDIR(target->target_state.stx_mode))goto done;
 o->how=(struct open_how){.flags=O_RDONLY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};family_init(&o->open);
 long r=sl_linux_call6(&o->open,"family.events.open-same-target",SYS_openat2,target->target_fd,(long)"cgroup.events",(long)&o->how,sizeof o->how,0,0);o->primary_result=r;
 if(r<0||r>INT_MAX){if(r>INT_MAX)o->primary_result=-EINVAL;goto done;}o->raw_fd=(int)r;
 r=sl_linux_lift_internal_fd(&o->lift,(int)r);o->primary_result=r;if(r<0){
  /* A duplicate may be actual even when closing its low original failed.
   * Close only that separately returned held duplicate, never retry original. */
  if(atomic_load_explicit(&o->lift.duplicate.returned,memory_order_acquire)&&o->lift.duplicate.result>=3&&o->lift.duplicate.result==o->lift.held_fd){o->fd=o->lift.held_fd;goto close;}goto done;
 }o->fd=(int)r;
 family_init(&o->filesystem_query);r=sl_linux_call6(&o->filesystem_query,"family.events.filesystem",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);o->primary_result=r;
 if(r||o->filesystem.f_type!=CGROUP2_SUPER_MAGIC){if(!r)o->primary_result=-EINVAL;goto close;}
 family_init(&o->identity_query);unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 r=sl_linux_call6(&o->identity_query,"family.events.identity",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0);o->primary_result=r;
 if(r||(o->identity.stx_mask&mask)!=mask||!S_ISREG(o->identity.stx_mode)||o->identity.stx_mnt_id!=target->target_state.stx_mnt_id||o->identity.stx_dev_major!=target->target_state.stx_dev_major||o->identity.stx_dev_minor!=target->target_state.stx_dev_minor){if(!r)o->primary_result=-EINVAL;goto close;}
 o->primary_result=-EINVAL;
 for(unsigned long i=0;i<22;i++){
  sl_linux_family_read *read=&o->reads[i];read->attempted=1;family_init(&read->call);o->read_count=i+1;
  r=sl_linux_call6(&read->call,"family.events.distinct-byte",SYS_read,o->fd,(long)&read->byte,1,0,0,0);read->count=r;read->returned=1;
  if(r<0){o->primary_result=r;goto close;}if(r>1)goto close;
  if(!r){o->eof=1;break;}o->extent++;if(i==21){o->excess=1;goto close;}
 }
 if(!o->eof||o->extent!=21)goto close;
 {static const unsigned char recipe[]="populated 0\nfrozen 0\n";
  for(unsigned long i=0;i<21;i++){
   unsigned char b=o->reads[i].byte;
   if(i==10||i==19){if(b!='0'&&b!='1')goto close;}else if(b!=recipe[i])goto close;
  }
 }
 o->populated=o->reads[10].byte-'0';o->frozen=o->reads[19].byte-'0';o->framing_complete=1;o->primary_result=0;
 close:o->close_attempted=1;family_init(&o->close);
 o->close_result=sl_linux_call6(&o->close,"family.events.independent-once-close",SYS_close,o->fd,0,0,0,0,0);o->close_returned=1;
 o->result=o->primary_result?o->primary_result:o->close_result;
 done:if(!o->close_attempted)o->result=o->primary_result;o->returned=1;return o->result;
}
#endif
#if defined(__linux__) && defined(__x86_64__)
#include "schedule_linux.h"
static void *family_worker_body(void *argument){
 sl_linux_family_worker *worker=argument;atomic_store_explicit(&worker->body_entered,1,memory_order_release);
 sl_linux_observe_family_events(&worker->observation);
 atomic_store_explicit(&worker->body_returned,1,memory_order_release);if(worker->terminal_wake)sl_linux_schedule_notify(worker->terminal_wake);return &worker->observation;
}
long sl_linux_start_family_worker(sl_linux_family_worker *w){
 if(!w||w->attempted)return -EINVAL;w->attempted=1;
 if(!w->observation.target_original||w->observation.attempted||w->joined_by)return -EINVAL;
 atomic_init(&w->body_entered,0);atomic_init(&w->body_returned,0);w->create_entered=1;
 w->create_result=pthread_create(&w->thread,0,family_worker_body,w);w->create_returned=1;return w->create_result;
}
long sl_linux_observe_family_join(sl_linux_family_join *o){
 if(!o||o->attempted||!o->worker_original)return -EINVAL;o->attempted=1;o->interpretation_result=-EINVAL;
 sl_linux_family_worker *w=o->worker_original;
 if(!w->create_returned||w->create_result||w->joined_by)return o->interpretation_result;
 o->native_result=pthread_tryjoin_np(w->thread,&o->joined_result);o->returned=1;
 if(o->native_result){o->interpretation_result=o->native_result;return o->interpretation_result;}
 w->joined_by=o;
 if(o->joined_result!=&w->observation||!atomic_load_explicit(&w->body_returned,memory_order_acquire)||!w->observation.returned)return o->interpretation_result;
 if(w->terminal_wake){long result=sl_linux_schedule_joined_notification(w->terminal_wake,w);if(result){o->interpretation_result=result;return result;}}
 o->interpretation_result=w->observation.result;return o->interpretation_result;
}
#endif