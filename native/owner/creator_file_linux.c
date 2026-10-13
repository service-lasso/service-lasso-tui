#if defined(__linux__) && defined(__x86_64__)
#include "creator_file_linux.h"
#include "shared_range_linux.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
static const char creator_empty[]="";
static int creator_disjoint(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int creator_view(const void *out,unsigned long bytes,const void *p,unsigned long n){
 return !n||creator_disjoint(out,bytes,p,n);
}
static int creator_entry_views(const sl_linux_entry *e,const void *out,unsigned long bytes){
 if(!e||!creator_disjoint(out,bytes,e,sizeof(*e)))return 0;
 const sl_linux_shared_range *ranges[8]={&e->bootstrap_range,&e->fd_action_range,
 &e->credential_ranges[0][0],&e->credential_ranges[0][1],&e->credential_ranges[0][2],
 &e->credential_ranges[1][0],&e->credential_ranges[1][1],&e->credential_ranges[1][2]};
 for(unsigned i=0;i<8;i++){
  const sl_linux_shared_range *r=ranges[i];
  if(r->mapping_original){
   if(!creator_disjoint(out,bytes,r->mapping_original,sizeof(*r->mapping_original)))return 0;
   void *p=sl_linux_shared_range_address(r,r->bytes,1);
   if(!p||!creator_disjoint(out,bytes,p,r->bytes))return 0;
  }else if(r->bytes||r->offset)return 0;
 }
 if(e->fd_action_input_count>ULONG_MAX/sizeof(*e->fd_action_inputs)||
    !creator_view(out,bytes,e->fd_action_inputs,e->fd_action_input_count*sizeof(*e->fd_action_inputs)))return 0;
 for(unsigned i=0;i<2;i++){
  const sl_linux_socket_peer *p=&e->custody_peers[i];
  if(!creator_view(out,bytes,p->bytes,p->capacity)||!creator_view(out,bytes,p->actual_bytes,p->actual_capacity))return 0;
 }
 const sl_linux_bootstrap *b=e->birth.bootstrap;
 if(!b)return 1;
 if(!creator_disjoint(out,bytes,b,sizeof(*b))||b->fd_action_count>ULONG_MAX/sizeof(*b->fd_actions)||
    !creator_view(out,bytes,b->fd_actions,b->fd_action_count*sizeof(*b->fd_actions)))return 0;
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++){
  const sl_linux_shared_range *r=&b->authority.credential_ranges[i][j];
  if(r->mapping_original){
   if(!creator_disjoint(out,bytes,r->mapping_original,sizeof(*r->mapping_original)))return 0;
   void *p=sl_linux_shared_range_address(r,r->bytes,1);
   if(!p||!creator_disjoint(out,bytes,p,r->bytes))return 0;
  }else if(r->bytes||r->offset)return 0;
 }
 const sl_linux_credential_snapshot *credentials[2]={&b->authority.credentials_before,&b->authority.credentials_after};
 for(unsigned i=0;i<2;i++){
  const sl_linux_credential_snapshot *c=credentials[i];
  if(c->read_capacity>ULONG_MAX/sizeof(*c->reads)||c->group_capacity>ULONG_MAX/sizeof(*c->groups)||
     !creator_view(out,bytes,c->status,c->status_capacity)||
     !creator_view(out,bytes,c->reads,c->read_capacity*sizeof(*c->reads))||
     !creator_view(out,bytes,c->groups,c->group_capacity*sizeof(*c->groups)))return 0;
 }
 return 1;
 /* argv/environment, candidate_preexec and their actual independent owning
  * source bindings remain separate full-caller obligations, not absent aliases. */
}
static int creator_fresh(const void *p,unsigned long n){const unsigned char *b=p;for(unsigned long i=0;i<n;i++)if(b[i])return 0;return 1;}
static void creator_initialize(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
static int creator_lift(const sl_linux_local_installation *s){
 const sl_linux_fd_lift *l=&s->executable_lift;long raw=s->open_executable.result;
 if(!l->attempted||raw<0||raw>INT_MAX||l->original_fd!=raw||l->held_fd!=s->executable_fd||l->held_fd<3)return 0;
 if(raw>=3)return l->held_fd==raw;
 const sl_linux_call *d=&l->duplicate,*c=&l->close_original;
 return atomic_load(&d->entered)&&atomic_load(&d->returned)&&d->number==SYS_fcntl&&
 d->arguments[0]==raw&&d->arguments[1]==F_DUPFD_CLOEXEC&&d->arguments[2]==3&&
 !d->arguments[3]&&!d->arguments[4]&&!d->arguments[5]&&d->result==l->held_fd&&
 atomic_load(&c->entered)&&atomic_load(&c->returned)&&c->number==SYS_close&&c->arguments[0]==raw&&
 !c->arguments[1]&&!c->arguments[2]&&!c->arguments[3]&&!c->arguments[4]&&!c->arguments[5]&&!c->result;
}
static int creator_same(const struct statx *a,const struct statx *b){
 unsigned mask=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE|STATX_MTIME|STATX_CTIME;
 return (a->stx_mask&mask)==mask&&(b->stx_mask&mask)==mask&&S_ISREG(a->stx_mode)&&
 a->stx_mode==b->stx_mode&&a->stx_uid==b->stx_uid&&a->stx_gid==b->stx_gid&&
 a->stx_ino==b->stx_ino&&a->stx_mnt_id==b->stx_mnt_id&&a->stx_size==b->stx_size&&
 a->stx_dev_major==b->stx_dev_major&&a->stx_dev_minor==b->stx_dev_minor&&
 a->stx_mtime.tv_sec==b->stx_mtime.tv_sec&&a->stx_mtime.tv_nsec==b->stx_mtime.tv_nsec&&
 a->stx_ctime.tv_sec==b->stx_ctime.tv_sec&&a->stx_ctime.tv_nsec==b->stx_ctime.tv_nsec;
 /* Actual reads may update atime. Metadata equality is not immutability. */
}
static int creator_call(const sl_linux_call *c,long number,long raw,long a0,long a1,long a2,long a3,long a4){
 return atomic_load(&c->entered)&&atomic_load(&c->returned)&&c->source&&c->number==number&&c->result==raw&&
 c->arguments[0]==a0&&c->arguments[1]==a1&&c->arguments[2]==a2&&c->arguments[3]==a3&&c->arguments[4]==a4&&!c->arguments[5];
}
static int creator_boundary_match(const sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){
 return b->sequence==s&&sl_linux_deadline_boundary_matches_original(b);
}
static int creator_calls_match(const sl_linux_creator_file *o){
 unsigned mask=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE|STATX_MTIME|STATX_CTIME;
 if(!o->count||o->count>o->actual_read_capacity||o->extent!=o->before.stx_size||o->extent>o->actual_body_capacity||
    o->before_raw||o->after_raw||o->eof_raw||!creator_same(&o->before,&o->after)||
    !creator_call(&o->before_call,SYS_statx,0,o->actual_fd,(long)creator_empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->before)||
    !creator_call(&o->after_call,SYS_statx,0,o->actual_fd,(long)creator_empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->after)||
    !creator_call(&o->eof_call,SYS_pread64,0,o->actual_fd,(long)&o->eof_byte,1,o->extent,0))return 0;
 if(!o->actual_sequence||o->actual_sequence!=o->sequence_original||
    !creator_boundary_match(&o->before_boundary,o->actual_sequence)||
    !creator_boundary_match(&o->eof_boundary,o->actual_sequence)||
    !creator_boundary_match(&o->after_boundary,o->actual_sequence))return 0;
 unsigned long next=o->before_boundary.first+2;
 unsigned long extent=0;
 for(unsigned long i=0;i<o->count;i++){
  const sl_linux_creator_read *r=&o->actual_reads[i];
  if(!creator_boundary_match(&r->boundary,o->actual_sequence)||r->boundary.first!=next||!r->attempted||!r->returned||r->offset!=extent||r->requested!=o->extent-extent||r->raw<=0||
     (unsigned long)r->raw>r->requested||!creator_call(&r->call,SYS_pread64,r->raw,o->actual_fd,
       (long)(o->actual_body+extent),r->requested,extent,0))return 0;
  extent+=(unsigned long)r->raw;next+=2;
 }
 return extent==o->extent&&o->eof_boundary.first==next&&o->after_boundary.first==next+2;
}
int sl_linux_creator_file_matches_original(const sl_linux_creator_file *o){
 return o&&o->attempted&&o->returned&&o->complete&&!o->result&&o->actual_installation&&
 o->actual_installation==o->installation_original&&o->actual_body&&o->actual_body==o->body&&
 o->actual_body_capacity&&o->actual_body_capacity==o->body_capacity&&o->actual_reads&&o->actual_reads==o->reads&&
 o->actual_read_capacity&&o->actual_read_capacity==o->read_capacity&&
 o->actual_read_capacity<=ULONG_MAX/sizeof(*o->actual_reads)&&
 creator_disjoint(o,sizeof(*o),o->actual_body,o->actual_body_capacity)&&
 creator_disjoint(o,sizeof(*o),o->actual_reads,o->actual_read_capacity*sizeof(*o->actual_reads))&&
 creator_disjoint(o->actual_body,o->actual_body_capacity,o->actual_reads,o->actual_read_capacity*sizeof(*o->actual_reads))&&
 sl_linux_deadline_sequence_excludes_output(o->actual_sequence,o,sizeof(*o))&&
 sl_linux_deadline_sequence_excludes_output(o->actual_sequence,o->actual_body,o->actual_body_capacity)&&
 sl_linux_deadline_sequence_excludes_output(o->actual_sequence,o->actual_reads,o->actual_read_capacity*sizeof(*o->actual_reads))&&
 o->actual_installation->executable_fd==o->actual_fd&&creator_lift(o->actual_installation)&&creator_calls_match(o);
}
int sl_linux_creator_file_excludes_output(const sl_linux_creator_file *o,const void *out,unsigned long extent){
 if(!extent||!sl_linux_creator_file_matches_original(o)||!sl_linux_deadline_sequence_excludes_output(o->actual_sequence,out,extent)||!creator_disjoint(out,extent,o,sizeof(*o))||
    !creator_disjoint(out,extent,o->actual_body,o->actual_body_capacity)||
    !creator_disjoint(out,extent,o->actual_reads,o->actual_read_capacity*sizeof(*o->actual_reads)))return 0;
 const sl_linux_local_installation *s=o->actual_installation;
 if(!creator_disjoint(out,extent,s,sizeof(*s))||!s->entry_original||
    !creator_entry_views(s->entry_original,out,extent))return 0;
 const void *p[6]={s->executable_path,s->directory_path,s->prepared_executable_path,s->prepared_directory_path,s->payload.directory_locator,s->payload.prepared_locator};
 unsigned long n[6]={s->executable_capacity,s->directory_capacity,s->prepared_executable_capacity,s->prepared_directory_capacity,s->payload.directory_capacity,s->payload.directory_capacity};
 for(unsigned i=0;i<6;i++)if(n[i]&&!creator_disjoint(out,extent,p[i],n[i]))return 0;
 return !s->payload.entry_original||creator_entry_views(s->payload.entry_original,out,extent);
 /* Direct surviving source views only. Full transitive native source graph
  * remains an actual containing/entry caller obligation before activation. */
}
long sl_linux_capture_creator_file(sl_linux_creator_file *o){
 if(!o||!o->sequence_original||!o->installation_original||!o->body||!o->body_capacity||!o->reads||!o->read_capacity||
    o->read_capacity>ULONG_MAX/sizeof(*o->reads))return -EINVAL;
 const sl_linux_local_installation *source=o->installation_original;
 unsigned long rows=o->read_capacity*sizeof(*o->reads);
 if(!sl_linux_deadline_sequence_excludes_output(o->sequence_original,o,sizeof(*o)))return -EINVAL;
 sl_linux_deadline_sequence *sequence=o->sequence_original;
 const void *outputs[5]={o,o->body,o->reads,sequence,sequence->actual_observations};
 unsigned long sizes[5]={sizeof(*o),o->body_capacity,rows,sizeof(*sequence),sequence->actual_capacity*sizeof(*sequence->actual_observations)};
 for(unsigned i=0;i<5;i++){
  if(i<3&&!sl_linux_deadline_sequence_excludes_output(sequence,outputs[i],sizes[i]))return -EINVAL;
  if(!creator_disjoint(outputs[i],sizes[i],source,sizeof(*source)))return -EINVAL;
  if(!source->entry_original||!creator_entry_views(source->entry_original,outputs[i],sizes[i]))return -EINVAL;
  for(unsigned j=0;j<i;j++)if(!creator_disjoint(outputs[i],sizes[i],outputs[j],sizes[j]))return -EINVAL;
  if(source->prepared_executable_capacity&&!creator_disjoint(outputs[i],sizes[i],source->prepared_executable_path,source->prepared_executable_capacity))return -EINVAL;
  if(source->prepared_directory_capacity&&!creator_disjoint(outputs[i],sizes[i],source->prepared_directory_path,source->prepared_directory_capacity))return -EINVAL;
  if(source->executable_capacity&&!creator_disjoint(outputs[i],sizes[i],source->executable_path,source->executable_capacity))return -EINVAL;
  if(source->directory_capacity&&!creator_disjoint(outputs[i],sizes[i],source->directory_path,source->directory_capacity))return -EINVAL;
  if(source->payload.entry_original&&!creator_entry_views(source->payload.entry_original,outputs[i],sizes[i]))return -EINVAL;
  if(source->payload.directory_capacity&&(
     !creator_disjoint(outputs[i],sizes[i],source->payload.directory_locator,source->payload.directory_capacity)||
     !creator_disjoint(outputs[i],sizes[i],source->payload.prepared_locator,source->payload.directory_capacity)))return -EINVAL;
 }
 if(!creator_fresh(&o->actual_installation,sizeof(*o)-offsetof(sl_linux_creator_file,actual_installation))||
    !creator_fresh(o->reads,rows)||!creator_fresh(o->body,o->body_capacity)||
    !source->attempted||!source->returned||source->result||!creator_lift(source)||
    !atomic_load(&source->open_executable.entered)||!atomic_load(&source->open_executable.returned)||source->open_executable.result<0||
    source->open_executable.number!=SYS_openat||source->open_executable.arguments[0]!=source->proc_fd||
    source->open_executable.arguments[2]!=(O_RDONLY|O_NONBLOCK|O_CLOEXEC)||
    source->open_executable.arguments[3]||source->open_executable.arguments[4]||source->open_executable.arguments[5]||
    !source->open_executable.arguments[1]||!atomic_load(&source->executable_identity.entered)||
    !atomic_load(&source->executable_identity.returned)||source->executable_identity.result||
    source->executable_identity.number!=SYS_statx||source->executable_identity.arguments[0]!=source->executable_fd||
    !source->executable_identity.arguments[1]||source->executable_identity.arguments[2]!=(AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)||
    source->executable_identity.arguments[3]!=(STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE)||
    source->executable_identity.arguments[4]!=(long)&source->executable_state||source->executable_identity.arguments[5])return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->actual_sequence=o->sequence_original;o->actual_installation=source;o->actual_fd=source->executable_fd;
 o->actual_body=o->body;o->actual_body_capacity=o->body_capacity;o->actual_reads=o->reads;o->actual_read_capacity=o->read_capacity;
 creator_initialize(&o->before_call);creator_initialize(&o->eof_call);creator_initialize(&o->after_call);
 for(unsigned long i=0;i<o->actual_read_capacity;i++)creator_initialize(&o->actual_reads[i].call);
 unsigned mask=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE|STATX_MTIME|STATX_CTIME;
 o->result=sl_linux_begin_deadline_boundary(&o->before_boundary,o->actual_sequence);if(o->result)goto done;
 o->before_raw=sl_linux_call6(&o->before_call,"creator.file.before",SYS_statx,o->actual_fd,(long)creator_empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->before,0);
 o->result=sl_linux_end_deadline_boundary(&o->before_boundary);if(o->result)goto done;
 if(o->before_raw){o->result=o->before_raw;goto done;}
 if(!creator_same(&o->before,&o->before)||!o->before.stx_size||o->before.stx_size>o->actual_body_capacity||o->before.stx_size>LONG_MAX)goto done;
 while(o->extent<o->before.stx_size){
  if(o->count==o->actual_read_capacity){o->result=-ENOSPC;goto done;}
  sl_linux_creator_read *row=&o->actual_reads[o->count++];row->attempted=1;row->offset=o->extent;
  row->requested=o->before.stx_size-o->extent;
  o->result=sl_linux_begin_deadline_boundary(&row->boundary,o->actual_sequence);if(o->result)goto done;
  row->raw=sl_linux_call6(&row->call,"creator.file.pread",SYS_pread64,o->actual_fd,(long)(o->actual_body+row->offset),row->requested,row->offset,0,0);row->returned=1;
  o->result=sl_linux_end_deadline_boundary(&row->boundary);if(o->result)goto done;
  if(row->raw<=0||(unsigned long)row->raw>row->requested){o->result=row->raw<0?row->raw:-EIO;goto done;}
  o->extent+=(unsigned long)row->raw;
 }
 o->result=sl_linux_begin_deadline_boundary(&o->eof_boundary,o->actual_sequence);if(o->result)goto done;
 o->eof_raw=sl_linux_call6(&o->eof_call,"creator.file.eof",SYS_pread64,o->actual_fd,(long)&o->eof_byte,1,o->extent,0,0);
 o->result=sl_linux_end_deadline_boundary(&o->eof_boundary);if(o->result)goto done;
 if(o->eof_raw){o->result=o->eof_raw<0?o->eof_raw:-EIO;goto done;}
 o->result=sl_linux_begin_deadline_boundary(&o->after_boundary,o->actual_sequence);if(o->result)goto done;
 o->after_raw=sl_linux_call6(&o->after_call,"creator.file.after",SYS_statx,o->actual_fd,(long)creator_empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->after,0);
 o->result=sl_linux_end_deadline_boundary(&o->after_boundary);if(o->result)goto done;
 if(o->after_raw){o->result=o->after_raw;goto done;}
 if(o->actual_sequence!=o->sequence_original||!creator_same(&o->before,&o->after)||!creator_lift(source)||source!=o->installation_original||source->executable_fd!=o->actual_fd||
    o->body!=o->actual_body||o->body_capacity!=o->actual_body_capacity||o->reads!=o->actual_reads||o->read_capacity!=o->actual_read_capacity)goto done;
 if(!creator_calls_match(o))goto done;
 o->complete=1;o->result=0;
done:o->returned=1;if(!o->result&&!sl_linux_creator_file_matches_original(o)){o->complete=0;o->result=-EINVAL;}return o->result;
 /* Completed held bytes only. Original D/native operands/FD lift and complete
  * source/own-writer lifetime returning joins remain needed before activation. */
}
#endif
