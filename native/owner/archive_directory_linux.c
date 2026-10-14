#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static uint64_t archive_u64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static int archive_meaning(sl_candidate_span s){static const unsigned char text[]="service-lasso.archive-directory-object.linux.v1";if(!s.bytes||s.extent!=sizeof(text)-1)return 0;for(size_t i=0;i<s.extent;i++)if(s.bytes[i]!=text[i])return 0;return 1;}
static void archive_init(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
static int archive_path(sl_candidate_span p){
 if(!p.bytes||p.extent<2||p.bytes[0]!='/'||p.bytes[p.extent-1]=='/')return 0;
 size_t begin=1;
 for(size_t i=1;i<=p.extent;i++){
  if(i<p.extent&&!p.bytes[i])return 0;
  if(i==p.extent||p.bytes[i]=='/'){
   size_t n=i-begin;if(!n||(n==1&&p.bytes[begin]=='.')||(n==2&&p.bytes[begin]=='.'&&p.bytes[begin+1]=='.'))return 0;begin=i+1;
  }
 }return 1;
}
long sl_linux_acquire_archive_directory(sl_linux_archive_directory *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=o->primary_result=-EINVAL;o->raw_fd=o->fd=-1;
 o->actual_startup=o->startup_original;o->actual_path=o->path;o->actual_path_capacity=o->path_capacity;o->actual_declarations=o->declarations;o->actual_declaration_capacity=o->declaration_capacity;o->actual_statuses=o->statuses;o->actual_status_capacity=o->status_capacity;
 sl_linux_public_startup *s=o->actual_startup;
 if(!s||o!=&s->archive_directory||!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_file_entry_returned||s->candidate_file_entry_result||!s->actual_candidate_policies||!s->actual_candidate_policy_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_candidate_policy_capacity||!o->actual_path||!o->actual_path_capacity||!o->actual_declarations||!o->actual_declaration_capacity||!o->actual_statuses||o->actual_status_capacity!=s->actual_candidate_policy_capacity)goto done;
 o->candidate_original=s->actual_candidate_policies[0].actual_candidate;sl_candidate_profile *candidate=o->candidate_original;
 if(!candidate||!candidate->returned||candidate->result||!candidate->actual_rows[0]||!candidate->actual_rows[3])goto done;
 for(unsigned long p=0;p<s->mounts.selinux_count;p++)if(!s->actual_candidate_policies[p].returned||s->actual_candidate_policies[p].result||s->actual_candidate_policies[p].actual_candidate!=candidate)goto done;
 for(unsigned long i=0;i<o->actual_declaration_capacity;i++)if(o->actual_declarations[i].attempted||o->actual_declarations[i].declaration_original)goto done;
 for(unsigned long i=0;i<o->actual_status_capacity;i++)if(o->actual_statuses[i].attempted||o->actual_statuses[i].status_original)goto done;
 for(size_t i=0;i<candidate->counts[3];i++){
  const sl_candidate_record *d=&candidate->actual_rows[3][i];o->examined=i+1;
  if(d->field_count!=3||!archive_meaning(d->fields[1]))continue;
  if(o->declaration_count>=o->actual_declaration_capacity){o->primary_result=-EOVERFLOW;goto sampled;}
  sl_linux_archive_declaration *b=&o->actual_declarations[o->declaration_count++];b->attempted=1;b->declaration_original=d;b->result=-EINVAL;
  sl_candidate_span link=d->fields[0],value=d->fields[2];
  if(!link.bytes||link.extent!=24||!value.bytes||value.extent<40)goto row_failed;
  b->evidence_index=archive_u64(link.bytes);b->statement_offset=archive_u64(link.bytes+8);b->statement_extent=archive_u64(link.bytes+16);
  if(b->evidence_index>=candidate->counts[0]||!b->statement_extent)goto row_failed;
  b->evidence_original=&candidate->actual_rows[0][(size_t)b->evidence_index];sl_candidate_span body=b->evidence_original->fields[6];
  if(b->evidence_original->field_count!=7||!body.bytes||b->statement_offset>body.extent||b->statement_extent>body.extent-b->statement_offset)goto row_failed;
  b->statement_original=(sl_candidate_span){body.bytes+(size_t)b->statement_offset,(size_t)b->statement_extent};
  for(unsigned j=0;j<4;j++)b->tuple[j]=archive_u64(value.bytes+8*j);
  uint64_t extent=archive_u64(value.bytes+32);
  if(b->tuple[0]>UINT32_MAX||b->tuple[1]>UINT32_MAX||!b->tuple[2]||!b->tuple[3]||extent>SIZE_MAX||extent!=value.extent-40)goto row_failed;
  b->path_original=(sl_candidate_span){value.bytes+40,(size_t)extent};if(!archive_path(b->path_original))goto row_failed;
  if(o->declaration_count>1){sl_linux_archive_declaration *first=&o->actual_declarations[0];for(unsigned j=0;j<4;j++)if(b->tuple[j]!=first->tuple[j])goto row_failed;if(b->path_original.extent!=first->path_original.extent)goto row_failed;for(size_t j=0;j<b->path_original.extent;j++)if(b->path_original.bytes[j]!=first->path_original.bytes[j])goto row_failed;}
  b->result=0;b->returned=1;continue;
  row_failed:b->returned=1;goto sampled;
 }
 if(!o->declaration_count)goto sampled;
 sl_linux_archive_declaration *first=&o->actual_declarations[0];
 if(first->path_original.extent>=o->actual_path_capacity){o->primary_result=-EOVERFLOW;goto sampled;}
 o->path_extent=(unsigned long)first->path_original.extent;
 for(unsigned long i=0;i<o->path_extent;i++)o->actual_path[i]=first->path_original.bytes[i];o->actual_path[o->path_extent]=0;
 o->how.flags=O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC;o->how.resolve=RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS;
 archive_init(&o->open);long r=sl_linux_call6(&o->open,"archive.directory.openat2",SYS_openat2,AT_FDCWD,(long)o->actual_path,(long)&o->how,sizeof(o->how),0,0);
 if(r<0||r>INT_MAX){o->primary_result=r<0?r:-EINVAL;goto sampled;}o->raw_fd=(int)r;
 r=sl_linux_lift_internal_fd(&o->lift,o->raw_fd);if(r<0||r>INT_MAX){o->primary_result=r<0?r:-EINVAL;goto sampled;}o->fd=(int)r;
 archive_init(&o->filesystem_query);r=sl_linux_call6(&o->filesystem_query,"archive.directory.held-filesystem",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);if(r){o->primary_result=r;goto sampled;}
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;archive_init(&o->identity_query);
 r=sl_linux_call6(&o->identity_query,"archive.directory.held-identity",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0);if(r){o->primary_result=r;goto sampled;}
 if((o->identity.stx_mask&mask)!=mask||!S_ISDIR(o->identity.stx_mode)||first->tuple[0]!=o->identity.stx_dev_major||first->tuple[1]!=o->identity.stx_dev_minor||first->tuple[2]!=o->identity.stx_ino||first->tuple[3]!=o->identity.stx_mnt_id)goto sampled;
 o->primary_result=0;
 sampled:for(unsigned long p=0;p<s->mounts.selinux_count;p++){
  sl_linux_selinux_status_sample *sample=&o->actual_statuses[p];o->status_count=p+1;sample->status_original=s->actual_candidate_policies[p].after_binding.status_original;
  long sr=sl_linux_sample_selinux_status(sample);if(!sr&&!sl_linux_equal_selinux_status(&s->actual_candidate_policies[p].after_binding,sample))sr=-EINVAL;
  if(sr&&!o->status_failure_original){o->status_failure_original=sample;o->status_result=sr;}
 }
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
#endif