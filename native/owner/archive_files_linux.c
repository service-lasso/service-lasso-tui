#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static void file_init(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
static unsigned long file_decimal(unsigned char *p,uint64_t n){unsigned char reversed[20];unsigned long count=0;do{reversed[count++]=(unsigned char)('0'+n%10);n/=10;}while(n);for(unsigned long i=0;i<count;i++)p[i]=reversed[count-1-i];return count;}
static long create_file(sl_linux_archive_files *owner,sl_linux_archive_file *f,const char suffix[7]){
 static const char prefix[]="service-lasso-tui.";static const char hex[]="0123456789abcdef";
 if(f->attempted)return -EINVAL;f->attempted=1;f->result=-EINVAL;f->raw_fd=f->fd=-1;f->directory_original=owner->directory_original;
 unsigned long at=0;for(unsigned i=0;i<sizeof(prefix)-1;i++)f->name[at++]=(unsigned char)prefix[i];
 for(unsigned i=0;i<32;i++){unsigned char byte=owner->source_identity_original.bytes[i];f->name[at++]=hex[byte>>4];f->name[at++]=hex[byte&15];}
 f->name[at++]='.';at+=file_decimal(f->name+at,(uint64_t)owner->process_result);
 f->name[at++]='.';at+=file_decimal(f->name+at,(uint64_t)owner->instant.tv_sec);
 f->name[at++]='.';at+=file_decimal(f->name+at,(uint64_t)owner->instant.tv_nsec);
 for(unsigned i=0;i<6;i++)f->name[at++]=(unsigned char)suffix[i];f->name[at]=0;f->name_extent=at;
 f->how.flags=O_RDWR|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC;f->how.mode=0600;
 f->how.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV;
 file_init(&f->open);long r=sl_linux_call6(&f->open,"archive.attempt.file.exclusive",SYS_openat2,f->directory_original->fd,(long)f->name,(long)&f->how,sizeof(f->how),0,0);
 if(r<0||r>INT_MAX){f->result=r<0?r:-EINVAL;goto done;}f->raw_fd=(int)r;
 r=sl_linux_lift_internal_fd(&f->lift,f->raw_fd);if(r<0||r>INT_MAX){f->result=r<0?r:-EINVAL;goto done;}f->fd=(int)r;
 file_init(&f->filesystem_query);r=sl_linux_call6(&f->filesystem_query,"archive.attempt.file.filesystem",SYS_fstatfs,f->fd,(long)&f->filesystem,0,0,0,0);if(r){f->result=r;goto done;}
 file_init(&f->identity_query);unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 r=sl_linux_call6(&f->identity_query,"archive.attempt.file.identity",SYS_statx,f->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&f->identity,0);if(r){f->result=r;goto done;}
 struct statx *directory=&f->directory_original->identity;
 if((f->identity.stx_mask&mask)!=mask||!S_ISREG(f->identity.stx_mode)||f->identity.stx_size||f->identity.stx_mnt_id!=directory->stx_mnt_id||f->identity.stx_dev_major!=directory->stx_dev_major||f->identity.stx_dev_minor!=directory->stx_dev_minor||f->identity.stx_ino==directory->stx_ino)goto done;
 f->result=0;
 done:f->returned=1;return f->result;
}
long sl_linux_create_archive_files(sl_linux_archive_files *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;o->actual_startup=o->startup_original;
 sl_linux_public_startup *s=o->actual_startup;
 if(!s||o!=&s->archive_files||!s->archive_directory.returned||s->archive_directory.result||s->archive_directory.fd<3||!s->archive_directory.candidate_original||!s->archive_directory.candidate_original->returned||s->archive_directory.candidate_original->result||o->local.attempted||o->final.attempted)goto done;
 if(!s->archive_precreation_attempted||!s->archive_precreation_returned||s->archive_precreation_result||!s->actual_parse_capacity||s->actual_parse_capacity>ULONG_MAX/SL_ARCHIVE_PURPOSE_COUNT||s->archive_access_capacity!=SL_ARCHIVE_PURPOSE_COUNT*s->actual_parse_capacity||!s->archive_accesses||!s->actual_archive_contexts||s->actual_archive_context_capacity!=3*s->actual_parse_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_parse_capacity)goto done;
 sl_linux_archive_label *directory_label=&s->archive_labels[SL_ARCHIVE_DIRECTORY_LABEL];
 if(!directory_label->attempted||!directory_label->returned||directory_label->result||!directory_label->framing_complete||directory_label->actual_startup!=s||directory_label->object_original!=&s->archive_directory||directory_label->identity_original!=&s->archive_directory.identity||directory_label->fd!=s->archive_directory.fd||directory_label->actual_bytes!=directory_label->bytes||directory_label->actual_capacity!=directory_label->capacity)goto done;
 for(unsigned long policy=0;policy<s->mounts.selinux_count;policy++){
  sl_linux_selinux_context *context=&s->actual_archive_contexts[2*s->actual_parse_capacity+policy];
  if(!context->archive_attempted||!context->archive_returned||context->archive_result||!context->attempted||!context->returned||context->result||context->status_result||context->archive_original!=directory_label)goto done;
  for(unsigned long purpose=12;purpose<SL_ARCHIVE_PURPOSE_COUNT;purpose++){
   sl_linux_archive_access *access=&s->archive_accesses[purpose*s->actual_parse_capacity+policy];
   if(!access->attempted||!access->returned||access->result||access->primary_result||access->status_result||access->startup_original!=s||access->purpose!=purpose||access->policy_index!=policy||access->context_original!=context)goto done;
   if(purpose>=SL_ARCHIVE_CREATION_PURPOSE_FIRST){
    if(!s->archive_creations||s->archive_creation_capacity!=s->actual_parse_capacity||access->creation_original!=&s->archive_creations[policy]||!access->creation_original->returned||access->creation_original->result||access->creation_original->status_result||access->inventory.actual_creation!=&access->creation_original->types||!access->inventory.returned||access->inventory.result)goto done;
   }
  }
 }
 o->actual_statuses=o->statuses;o->actual_status_capacity=o->status_capacity;
 if(!o->actual_statuses||!s->actual_candidate_policies||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_candidate_policy_capacity||o->actual_status_capacity!=s->actual_candidate_policy_capacity)goto done;
 for(unsigned long p=0;p<o->actual_status_capacity;p++)if(o->actual_statuses[p].attempted)goto done;
 for(unsigned long p=0;p<s->mounts.selinux_count;p++)if(!s->actual_candidate_policies[p].returned||s->actual_candidate_policies[p].result||!s->actual_candidate_policies[p].after_binding.status_original)goto done;
 o->directory_original=&s->archive_directory;o->source_identity_original=s->archive_directory.candidate_original->source_identity;
 if(!o->source_identity_original.bytes||o->source_identity_original.extent!=32)goto sampled;
 file_init(&o->process_query);o->process_result=sl_linux_call6(&o->process_query,"archive.attempt.getpid",SYS_getpid,0,0,0,0,0,0);
 if(o->process_result<=0||o->process_result>INT_MAX){o->result=o->process_result<0?o->process_result:-EINVAL;goto sampled;}
 file_init(&o->clock_query);o->clock_result=sl_linux_call6(&o->clock_query,"archive.attempt.clock.monotonic",SYS_clock_gettime,CLOCK_MONOTONIC,(long)&o->instant,0,0,0,0);
 if(o->clock_result){o->result=o->clock_result;goto sampled;}
 if(o->instant.tv_sec<0||o->instant.tv_nsec<0||o->instant.tv_nsec>=1000000000L)goto sampled;
 o->result=create_file(o,&o->local,".local");if(o->result)goto sampled;
 o->result=create_file(o,&o->final,".final");if(o->result)goto sampled;
 if(o->local.fd==o->final.fd||o->local.identity.stx_ino==o->final.identity.stx_ino)o->result=-EINVAL;
 sampled:o->primary_result=o->result;
 for(unsigned long p=0;p<s->mounts.selinux_count;p++){
  sl_linux_selinux_status_sample *sample=&o->actual_statuses[p];o->status_count=p+1;sample->status_original=s->actual_candidate_policies[p].after_binding.status_original;
  long sr=sl_linux_sample_selinux_status(sample);if(!sr&&!sl_linux_equal_selinux_status(&s->actual_candidate_policies[p].after_binding,sample))sr=-EINVAL;
  if(sr&&!o->status_failure_original){o->status_failure_original=sample;o->status_result=sr;}
 }
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
#endif
