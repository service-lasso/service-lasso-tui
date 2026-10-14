#if defined(__linux__) && defined(__x86_64__)
#include "public_controller_linux.h"
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static int readback_range(const void *a,unsigned long n,const void *b,unsigned long m){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!a||!b||n>UINTPTR_MAX-x||m>UINTPTR_MAX-y)return 1;
 return n&&m&&x<y+m&&y<x+n;
}
long sl_linux_prepare_archive_readback(sl_linux_archive_readback *o){
 if(!o||o->prepare_attempted)return -EINVAL;
 sl_linux_public_controller *c=o->controller_original;
 if(!c||o!=&c->final_reader_task||!c->startup_original||
    o->view_original!=&c->startup_original->final_reader||o->capture_original!=&c->final_capture||
    !o->bytes||!o->capacity||!o->reads||!o->read_capacity||
    o->read_capacity>ULONG_MAX/sizeof(*o->reads))return -EINVAL;
 unsigned long calls=o->read_capacity*sizeof(*o->reads);
 if(readback_range(o,sizeof(*o),o->bytes,o->capacity)||readback_range(o,sizeof(*o),o->reads,calls)||
    readback_range(o->bytes,o->capacity,o->reads,calls)||
    readback_range(c,sizeof(*c),o->bytes,o->capacity)||readback_range(c,sizeof(*c),o->reads,calls)||
    readback_range(c->startup_original,sizeof(*c->startup_original),o->bytes,o->capacity)||
    readback_range(c->startup_original,sizeof(*c->startup_original),o->reads,calls))return -EINVAL;
 sl_candidate_span source=c->final_capture.candidate_body;
 if(source.extent&&(readback_range(source.bytes,source.extent,o,sizeof(*o))||readback_range(source.bytes,source.extent,o->bytes,o->capacity)||readback_range(source.bytes,source.extent,o->reads,calls)))return -EINVAL;
 /* Fresh arena storage is a constructor obligation. No active call cell is
  * reset; all supplied cells are inspected before the first initialization. */
 for(unsigned long i=0;i<o->read_capacity;i++)if(atomic_load(&o->reads[i].entered)||atomic_load(&o->reads[i].returned))return -EINVAL;
 o->prepare_attempted=1;o->actual_view=o->view_original;o->actual_capture=o->capture_original;
 o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;o->actual_reads=o->reads;o->actual_read_capacity=o->read_capacity;
 atomic_init(&o->body_entered,0);atomic_init(&o->body_returned,0);
 atomic_init(&o->before.entered,0);atomic_init(&o->before.returned,0);atomic_init(&o->after.entered,0);atomic_init(&o->after.returned,0);
 for(unsigned long i=0;i<o->actual_read_capacity;i++){atomic_init(&o->actual_reads[i].entered,0);atomic_init(&o->actual_reads[i].returned,0);}
 o->prepared=1;return 0;
}
static long readback_body(sl_linux_archive_readback *o){
 while(o->extent<o->expected_extent){
  if(o->read_count>=o->actual_read_capacity)return -ENOSPC;
  sl_linux_call *call=&o->actual_reads[o->read_count++];
  unsigned long remain=o->expected_extent-o->extent;
  long n=sl_linux_call6(call,"archive.independent.read",SYS_pread64,o->actual_view->fd,(long)(o->actual_bytes+o->extent),remain,o->extent,0,0);
  if(n<0)return n;if(!n||((unsigned long)n)>remain)return -EIO;o->extent+=(unsigned long)n;
 }
 if(o->read_count>=o->actual_read_capacity)return -ENOSPC;
 long n=sl_linux_call6(&o->actual_reads[o->read_count++],"archive.independent.eof",SYS_pread64,o->actual_view->fd,(long)&o->eof_byte,1,o->extent,0,0);
 if(n)return n<0?n:-EIO;
 unsigned long offset=0;
 for(unsigned long i=0;i<o->record_count;i++){
  sl_linux_final_record *r=&o->records_original[i];
  if(memcmp(o->actual_bytes+offset,r->header,32))return -EIO;offset+=32;
  if(r->length&&memcmp(o->actual_bytes+offset,r->body,r->length))return -EIO;offset+=r->length;
 }
 if(offset!=o->expected_extent)return -EIO;
 long interpreted=sl_linux_interpret_shared_range_records(&o->shared_ranges,o->actual_bytes,o->expected_extent,o->records_original,o->record_count);
 if(interpreted)return interpreted;
 o->readback_equal=1;return 0;
}
static void *readback_worker(void *arg){
 sl_linux_archive_readback *o=arg;atomic_store_explicit(&o->body_entered,1,memory_order_release);
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 o->primary_result=sl_linux_call6(&o->before,"archive.independent.identity.before",SYS_statx,o->actual_view->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->before_state,0);
 struct statx *observed=&o->before_state,*held=&o->actual_view->identity;
 if(!o->primary_result&&((observed->stx_mask&mask)!=mask||!S_ISREG(observed->stx_mode)||observed->stx_ino!=held->stx_ino||observed->stx_mnt_id!=held->stx_mnt_id||observed->stx_dev_major!=held->stx_dev_major||observed->stx_dev_minor!=held->stx_dev_minor||observed->stx_size!=o->expected_extent))o->primary_result=-EINVAL;
 if(!o->primary_result)o->primary_result=readback_body(o);
 /* This independent returned observation is retained even after an actual
  * primary read/EOF/comparison failure. It cannot prove an immutable interval. */
 o->after_result=sl_linux_call6(&o->after,"archive.independent.identity.after",SYS_statx,o->actual_view->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->after_state,0);
 observed=&o->after_state;
 if(!o->after_result&&((observed->stx_mask&mask)!=mask||!S_ISREG(observed->stx_mode)||observed->stx_ino!=held->stx_ino||observed->stx_mnt_id!=held->stx_mnt_id||observed->stx_dev_major!=held->stx_dev_major||observed->stx_dev_minor!=held->stx_dev_minor||observed->stx_size!=o->expected_extent))o->after_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->after_result;atomic_store_explicit(&o->body_returned,1,memory_order_release);
 if(o->terminal_wake)sl_linux_schedule_notify(o->terminal_wake);
 return o;
}
long sl_linux_start_archive_readback(sl_linux_archive_readback *o){
 if(!o||!o->prepared||o->start_attempted)return -EINVAL;
 sl_linux_final_capture *f=o->actual_capture;sl_linux_archive_read_view *v=o->actual_view;
 if(!f||!v||!f->returned||f->result||!f->readback_equal||!v->returned||v->result||v->fd<3||
    !f->count||f->count>f->actual_capacity||f->extent>o->actual_capacity||
    f->count>ULONG_MAX/sizeof(*f->actual_records)||!o->terminal_wake||o->terminal_wake->source_original!=o)return -EINVAL;
 unsigned long records=f->count*sizeof(*f->actual_records),calls=o->actual_read_capacity*sizeof(*o->actual_reads),extent=0;
 /* All immutable source spans are checked before any dispatch mutation. */
 if(readback_range(f->actual_records,records,o,sizeof(*o))||readback_range(f->actual_records,records,o->actual_bytes,o->actual_capacity)||readback_range(f->actual_records,records,o->actual_reads,calls))return -EINVAL;
 for(unsigned long i=0;i<f->count;i++){
  sl_linux_final_record *r=&f->actual_records[i];
  if(r->offset!=extent)return -EINVAL;
  if(extent>ULONG_MAX-32||r->length>ULONG_MAX-extent-32)return -EOVERFLOW;extent+=32+r->length;
  if(r->length&&(readback_range(r->body,r->length,o,sizeof(*o))||readback_range(r->body,r->length,o->actual_bytes,o->actual_capacity)||readback_range(r->body,r->length,o->actual_reads,calls)))return -EINVAL;
 }
 if(extent!=f->extent)return -EINVAL;
 if(o->shared_ranges.attempted||o->shared_ranges.returned||o->shared_ranges.expected||o->shared_ranges.seen)return -EINVAL;
 uint64_t expected=0;sl_linux_entry *entry=&o->controller_original->entry;
 if(entry->attempted){
  if(entry->bootstrap_range.mapping_original)expected|=1;
  if(entry->fd_actions_attempted&&entry->fd_action_range.mapping_original)expected|=2;
 }
 if(entry->birth.attempted){
  sl_linux_bootstrap *bootstrap=entry->birth.bootstrap;if(!bootstrap)return -EINVAL;
  for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++)if(bootstrap->authority.credential_ranges[i][j].mapping_original)expected|=1ULL<<(2+3*i+j);
  if(bootstrap->candidate_preexec){
   sl_linux_candidate_preexec *p=o->controller_original->candidate_preexec_original;
   if(!p||p->shared!=bootstrap->candidate_preexec||p->actual_bootstrap!=bootstrap)return -EINVAL;
   if(p->containing_range.mapping_original)expected|=1ULL<<8;
  }
 }
 o->start_attempted=1;o->records_original=f->actual_records;o->record_count=f->count;o->expected_extent=extent;
 o->shared_ranges.expected=expected;
 o->create_entered=1;o->create_result=pthread_create(&o->thread,0,readback_worker,o);o->create_returned=1;
 return o->create_result?-(long)o->create_result:0;
}
long sl_linux_join_archive_readback(sl_linux_archive_reader_join *j){
 if(!j||j->attempted||!j->reader_original)return -EINVAL;
 sl_linux_archive_readback *o=j->reader_original;sl_linux_public_controller *c=o->controller_original;
 if(!c||o!=&c->final_reader_task||!c->final_reader_joins||!c->final_reader_join_capacity||c->final_reader_join_capacity>ULONG_MAX/sizeof(*j))return -EINVAL;
 uintptr_t base=(uintptr_t)c->final_reader_joins,at=(uintptr_t)j;unsigned long bytes=c->final_reader_join_capacity*sizeof(*j);
 if(bytes>UINTPTR_MAX-base||at<base||at>=base+bytes||(at-base)%sizeof(*j))return -EINVAL;
 j->attempted=1;long result=-EINVAL;
 if(!o->create_returned||o->create_result||o->joined_by)goto done;
 j->native_result=pthread_tryjoin_np(o->thread,&j->output);j->returned=1;
 if(j->native_result){result=j->native_result;goto done;}
 o->joined_by=j;
 if(j->output!=o||!atomic_load_explicit(&o->body_returned,memory_order_acquire))goto done;
 result=sl_linux_schedule_joined_notification(o->terminal_wake,o);if(result)goto done;
 result=o->result;if(!result&&!o->readback_equal)result=-EIO;
 done:j->interpretation_result=result;j->interpretation_returned=1;return result;
}
#endif
