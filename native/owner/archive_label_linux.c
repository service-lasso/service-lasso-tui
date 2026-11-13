#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static int label_separate(const void *a,unsigned long n,const void *b,unsigned long m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n&&m&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&(x+n<=y||y+m<=x);}
static int label_identity(const struct statx *a,const struct statx *b,unsigned kind){unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;return (a->stx_mask&mask)==mask&&(b->stx_mask&mask)==mask&&(kind==SL_ARCHIVE_DIRECTORY_LABEL?S_ISDIR(a->stx_mode):S_ISREG(a->stx_mode))&&a->stx_ino==b->stx_ino&&a->stx_mnt_id==b->stx_mnt_id&&a->stx_dev_major==b->stx_dev_major&&a->stx_dev_minor==b->stx_dev_minor;}
static int label_backing(sl_linux_public_startup *s,sl_linux_archive_label *o){
 if(!o->bytes||o->capacity<2||o->capacity>LONG_MAX||!label_separate(s,sizeof(*s),o->bytes,o->capacity))return 0;
 if(s->controller_original&&!label_separate(s->controller_original,sizeof(*s->controller_original),o->bytes,o->capacity))return 0;
 sl_candidate_profile *p=s->candidate_original;
 if(p&&(!label_separate(p,sizeof(*p),o->bytes,o->capacity)||(p->source_extent&&(!label_separate(p->source,p->source_extent,o->bytes,o->capacity)||!label_separate(p->source,p->source_extent,s,sizeof(*s))))))return 0;
 return 1;
}
long sl_linux_observe_archive_label(sl_linux_archive_label *o){
 if(!o||o->attempted)return -EINVAL;sl_linux_public_startup *s=o->startup_original;
 if(!s||o->kind>SL_ARCHIVE_DIRECTORY_LABEL||o->kind<SL_ARCHIVE_LOCAL_LABEL||o!=&s->archive_labels[o->kind]||!label_backing(s,o))return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->actual_startup=s;o->actual_kind=o->kind;o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;o->fd=-1;
 if(!s->returned||s->result||((o->actual_kind!=SL_ARCHIVE_DIRECTORY_LABEL)&&(!s->archive_files.returned||s->archive_files.result))||!s->archive_directory.returned||s->archive_directory.result)goto done;
 if(o->actual_kind==SL_ARCHIVE_DIRECTORY_LABEL){o->object_original=&s->archive_directory;o->identity_original=&s->archive_directory.identity;o->fd=s->archive_directory.fd;}
 else{sl_linux_archive_file *f=o->actual_kind==SL_ARCHIVE_LOCAL_LABEL?&s->archive_files.local:&s->archive_files.final;if(!f->returned||f->result)goto done;o->object_original=f;o->identity_original=&f->identity;o->fd=f->fd;o->creation_original=&f->open;o->creation_directory_original=f->directory_original;
  if(f->directory_original!=&s->archive_directory||!s->archive_precreation_returned||s->archive_precreation_result||!atomic_load_explicit(&f->open.entered,memory_order_acquire)||!atomic_load_explicit(&f->open.returned,memory_order_acquire)||f->open.number!=SYS_openat2||f->open.arguments[0]!=s->archive_directory.fd||f->open.arguments[1]!=(long)f->name||f->open.arguments[2]!=(long)&f->how||f->open.arguments[3]!=sizeof(f->how)||f->open.result!=f->raw_fd||f->raw_fd<0||!f->lift.attempted||f->lift.original_fd!=f->raw_fd||f->lift.held_fd!=f->fd||f->how.flags!=(O_RDWR|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC)||f->how.mode!=0600||f->how.resolve!=(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV))goto done;}
 if(o->fd<3)goto done;
 sl_linux_call *calls[3]={&o->before,&o->label,&o->after};for(unsigned i=0;i<3;i++)if(calls[i]->source||calls[i]->number)goto done;
 for(unsigned i=0;i<3;i++){atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);}
 const char attribute[]="security.selinux";for(unsigned i=0;i<sizeof(attribute);i++)o->attribute[i]=attribute[i];
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 o->before_result=sl_linux_call6(&o->before,"archive.label.before",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->before_state,0);
 if(o->before_result){o->result=o->before_result;goto done;}if(!label_identity(&o->before_state,o->identity_original,o->actual_kind))goto done;
 o->raw_count=sl_linux_call6(&o->label,"archive.label.security.selinux",SYS_fgetxattr,o->fd,(long)o->attribute,(long)o->actual_bytes,o->actual_capacity,0,0);
 o->after_result=sl_linux_call6(&o->after,"archive.label.after",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->after_state,0);
 if(!o->after_result&&!label_identity(&o->after_state,o->identity_original,o->actual_kind))o->after_result=-EINVAL;
 if(o->raw_count<0){o->result=o->raw_count;goto done;}if(o->after_result){o->result=o->after_result;goto done;}
 if(o->raw_count<2||(unsigned long)o->raw_count>o->actual_capacity)goto done;
 for(unsigned long i=0;i<(unsigned long)o->raw_count;i++){o->examined=i+1;if((i+1==(unsigned long)o->raw_count)!=(o->actual_bytes[i]==0))goto done;}
 o->extent=(unsigned long)o->raw_count-1;o->framing_complete=1;o->result=0;
 done:o->returned=1;return o->result;
}
long sl_linux_observe_archive_labels(sl_linux_public_startup *s){
 if(!s||s->archive_labels_attempted)return -EINVAL;
 for(unsigned i=0;i<3;i++){sl_linux_archive_label *o=&s->archive_labels[i];if((o->attempted&&!(i==SL_ARCHIVE_DIRECTORY_LABEL&&s->archive_precreation_returned&&!s->archive_precreation_result&&o->returned&&!o->result&&o->actual_startup==s&&o->actual_bytes==o->bytes&&o->actual_capacity==o->capacity))||o->startup_original!=s||o->kind!=(enum sl_linux_archive_label_kind)i||!label_backing(s,o))return -EINVAL;for(unsigned j=0;j<i;j++)if(!label_separate(o->bytes,o->capacity,s->archive_labels[j].bytes,s->archive_labels[j].capacity))return -EINVAL;}
 s->archive_labels_attempted=1;s->archive_labels_result=0;
 for(unsigned i=0;i<3;i++){long result=s->archive_labels[i].attempted?s->archive_labels[i].result:sl_linux_observe_archive_label(&s->archive_labels[i]);s->archive_labels_count=i+1;if(result&&!s->archive_labels_result){s->archive_labels_result=result;s->archive_label_failure=&s->archive_labels[i];}}
 s->archive_labels_returned=1;return s->archive_labels_result;
}
long sl_linux_observe_archive_directory_before_creation(sl_linux_public_startup *s){
 if(!s||s->archive_files.attempted)return -EINVAL;
 for(unsigned i=0;i<3;i++){
  sl_linux_archive_label *o=&s->archive_labels[i];
  if(o->attempted||o->startup_original!=s||o->kind!=(enum sl_linux_archive_label_kind)i||!label_backing(s,o))return -EINVAL;
  for(unsigned j=0;j<i;j++)if(!label_separate(o->bytes,o->capacity,s->archive_labels[j].bytes,s->archive_labels[j].capacity))return -EINVAL;
 }
 return sl_linux_observe_archive_label(&s->archive_labels[SL_ARCHIVE_DIRECTORY_LABEL]);
}
#endif
