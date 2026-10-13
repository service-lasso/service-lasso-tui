#if defined(__linux__) && defined(__x86_64__)
#include "failed_parent_file_linux.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static const char empty[]="";
static int separate(const void *a,size_t n,const void *b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static long failure(long r){return r<0?r:-ETIMEDOUT;}
static int native(const sl_linux_call *c,long number,long raw,long a,long b,long d,long e,long f,long g){
 return atomic_load_explicit(&c->entered,memory_order_acquire)==1&&atomic_load_explicit(&c->returned,memory_order_acquire)==1&&c->number==number&&c->result==raw&&c->arguments[0]==a&&c->arguments[1]==b&&c->arguments[2]==d&&c->arguments[3]==e&&c->arguments[4]==f&&c->arguments[5]==g;
}
static int source_excludes(const sl_linux_failed_parent_directory *d,const sl_linux_failed_archive_name *n,const void *out,size_t bytes){
 return separate(out,bytes,empty,sizeof(empty))&&sl_linux_archive_candidate_excludes_output(d->candidate_original,out,bytes)&&
    separate(out,bytes,d,sizeof(*d))&&separate(out,bytes,d->path_original,d->path_capacity)&&
    separate(out,bytes,d->declarations.rows_original,d->declarations.capacity*sizeof(*d->declarations.rows_original))&&
    separate(out,bytes,d->birth_original,sizeof(*d->birth_original))&&separate(out,bytes,n,sizeof(*n))&&
    separate(out,bytes,n->identity_original,32)&&separate(out,bytes,n->process_original,sizeof(*n->process_original))&&
    separate(out,bytes,n->clock_original,sizeof(*n->clock_original))&&separate(out,bytes,n->instant_original,sizeof(*n->instant_original));
}
static int inputs(const sl_linux_failed_parent_directory *d,const sl_linux_failed_archive_name *n,sl_linux_deadline_sequence *s){
 if(!sl_linux_failed_parent_directory_matches_original(d)||!sl_linux_failed_archive_name_matches_original(n)||s!=d->sequence_original)return 0;
 const sl_candidate_profile *p=d->candidate_original;uintptr_t base=(uintptr_t)p->original.bytes,id=(uintptr_t)p->source_identity.bytes;
 return p->source_identity.extent==32&&n->identity_original==p->source_identity.bytes&&id>=base&&p->original.extent>=32&&id-base<=p->original.extent-32;
}
int sl_linux_failed_parent_file_matches_original(const sl_linux_failed_parent_file *o){
 if(!o||o->attempted!=1||o->returned!=1||o->result||o->reader>1||o->fd<3||o->raw_fd<0||
    !inputs(o->directory_original,o->name_original,o->sequence_original)||
    !source_excludes(o->directory_original,o->name_original,o,sizeof(*o))||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o,sizeof(*o)))return 0;
 const sl_linux_failed_parent_directory *d=o->directory_original;const sl_linux_failed_archive_name *n=o->name_original;
 const sl_linux_deadline_sequence *s=o->sequence_original;
 if(!source_excludes(d,n,s,sizeof(*s))||
    !source_excludes(d,n,s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations))||
    o->open_boundary.first>s->actual_capacity||s->actual_capacity-o->open_boundary.first<10)return 0;
 size_t next=o->open_boundary.first+2;
 if(o->raw_fd<3){if(o->lift.duplicate_boundary.first!=next||o->lift.close_boundary.first!=next+2)return 0;next+=4;}
 if(o->filesystem_boundary.first!=next||o->identity_boundary.first!=next+2)return 0;
 const sl_linux_failed_parent_file *w=o->writer_original;
 if(!o->reader){if(w)return 0;}
 else if(!w||!separate(o,sizeof(*o),w,sizeof(*w))||w->reader||w->writer_original||
    !sl_linux_failed_parent_file_matches_original(w)||w->directory_original!=d||w->name_original!=n||w->sequence_original!=s||
    !separate(s,sizeof(*s),w,sizeof(*w))||!separate(s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations),w,sizeof(*w))||
    o->open_boundary.first<w->identity_boundary.first+2||o->fd==w->fd||o->identity.stx_ino!=w->identity.stx_ino||o->identity.stx_mnt_id!=w->identity.stx_mnt_id||
    o->identity.stx_dev_major!=w->identity.stx_dev_major||o->identity.stx_dev_minor!=w->identity.stx_dev_minor||
    o->identity.stx_size!=w->identity.stx_size||o->filesystem.f_type!=w->filesystem.f_type)return 0;
 uint64_t flags=o->reader?(O_RDONLY|O_NONBLOCK|O_CLOEXEC):(O_WRONLY|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC);
 if(o->how.flags!=flags||o->how.mode!=(o->reader?0:0600)||o->how.resolve!=(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV)||
    o->open_raw!=o->raw_fd||!native(&o->open,SYS_openat2,o->open_raw,d->fd,(long)n->name,(long)&o->how,sizeof(o->how),0,0)||
    o->lift.producer_original!=&o->open||o->lift.sequence_original!=o->sequence_original||!sl_linux_deadline_fd_lift_matches_original(&o->lift)||
    o->lift_result!=o->fd||o->lift.result!=o->fd||o->fd==d->fd||o->filesystem_raw||o->identity_raw||o->boundary_result)return 0;
 const sl_linux_deadline_boundary *boundaries[3]={&o->open_boundary,&o->filesystem_boundary,&o->identity_boundary};
 for(unsigned i=0;i<3;i++)if(boundaries[i]->sequence!=o->sequence_original||!sl_linux_deadline_boundary_matches_original(boundaries[i]))return 0;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 return native(&o->filesystem_query,SYS_fstatfs,0,o->fd,(long)&o->filesystem,0,0,0,0)&&
    native(&o->identity_query,SYS_statx,0,o->fd,(long)empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0)&&
    (o->identity.stx_mask&mask)==mask&&S_ISREG(o->identity.stx_mode)&&!o->identity.stx_size&&
    o->identity.stx_dev_major==d->identity.stx_dev_major&&o->identity.stx_dev_minor==d->identity.stx_dev_minor&&
    o->identity.stx_mnt_id==d->identity.stx_mnt_id&&o->identity.stx_ino!=d->identity.stx_ino;
}
static long acquire(sl_linux_failed_parent_file *o,const sl_linux_failed_parent_directory *d,
 const sl_linux_failed_archive_name *n,const sl_linux_failed_parent_file *writer,sl_linux_deadline_sequence *s){
 if(!o||!inputs(d,n,s)||!source_excludes(d,n,o,sizeof(*o))||!sl_linux_deadline_sequence_excludes_output(s,o,sizeof(*o))||
    !source_excludes(d,n,s,sizeof(*s))||!source_excludes(d,n,s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations))||
    s->failed||s->expired||s->exhausted||s->next>s->actual_capacity||s->actual_capacity-s->next<10)return -EINVAL;
 if(writer&&(!separate(o,sizeof(*o),writer,sizeof(*writer))||!sl_linux_failed_parent_file_matches_original(writer)||writer->reader||writer->writer_original||
    writer->directory_original!=d||writer->name_original!=n||writer->sequence_original!=s||
    !separate(s,sizeof(*s),writer,sizeof(*writer))||!separate(s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations),writer,sizeof(*writer))))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->raw_fd=o->fd=-1;o->reader=writer?1:0;
 o->directory_original=d;o->name_original=n;o->writer_original=writer;o->sequence_original=s;
 o->how.flags=writer?(O_RDONLY|O_NONBLOCK|O_CLOEXEC):(O_WRONLY|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC);
 o->how.mode=writer?0:0600;o->how.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV;
 o->boundary_result=sl_linux_begin_deadline_boundary(&o->open_boundary,s);if(o->boundary_result){o->result=failure(o->boundary_result);goto done;}
 atomic_init(&o->open.entered,0);atomic_init(&o->open.returned,0);
 o->open_raw=sl_linux_call6(&o->open,writer?"failed-parent.reader.open":"failed-parent.writer.exclusive",SYS_openat2,d->fd,(long)n->name,(long)&o->how,sizeof(o->how),0,0);
 if(o->open_raw>=0&&o->open_raw<=INT_MAX)o->raw_fd=(int)o->open_raw;
 o->boundary_result=sl_linux_end_deadline_boundary(&o->open_boundary);if(o->boundary_result){o->result=failure(o->boundary_result);goto done;}
 if(!inputs(d,n,s)||!sl_linux_deadline_boundary_matches_original(&o->open_boundary)||
    !native(&o->open,SYS_openat2,o->open_raw,d->fd,(long)n->name,(long)&o->how,sizeof(o->how),0,0))goto done;
 if(o->raw_fd<0){o->result=o->open_raw<0?o->open_raw:-EINVAL;goto done;}
 o->lift_result=sl_linux_lift_deadline_fd(&o->lift,&o->open,s);
 if(o->lift_result<3||o->lift_result>INT_MAX){o->result=o->lift_result<0?o->lift_result:-EINVAL;goto done;}o->fd=(int)o->lift_result;
 if(o->fd==d->fd||(writer&&o->fd==writer->fd))goto done;
 o->boundary_result=sl_linux_begin_deadline_boundary(&o->filesystem_boundary,s);if(o->boundary_result){o->result=failure(o->boundary_result);goto done;}
 atomic_init(&o->filesystem_query.entered,0);atomic_init(&o->filesystem_query.returned,0);
 o->filesystem_raw=sl_linux_call6(&o->filesystem_query,"failed-parent.file.filesystem",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);
 o->boundary_result=sl_linux_end_deadline_boundary(&o->filesystem_boundary);if(o->boundary_result){o->result=failure(o->boundary_result);goto done;}
 if(!inputs(d,n,s)||!sl_linux_deadline_boundary_matches_original(&o->filesystem_boundary)||!native(&o->filesystem_query,SYS_fstatfs,o->filesystem_raw,o->fd,(long)&o->filesystem,0,0,0,0))goto done;
 if(o->filesystem_raw){o->result=o->filesystem_raw<0?o->filesystem_raw:-EINVAL;goto done;}
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 o->boundary_result=sl_linux_begin_deadline_boundary(&o->identity_boundary,s);if(o->boundary_result){o->result=failure(o->boundary_result);goto done;}
 atomic_init(&o->identity_query.entered,0);atomic_init(&o->identity_query.returned,0);
 o->identity_raw=sl_linux_call6(&o->identity_query,"failed-parent.file.identity",SYS_statx,o->fd,(long)empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0);
 o->boundary_result=sl_linux_end_deadline_boundary(&o->identity_boundary);if(o->boundary_result){o->result=failure(o->boundary_result);goto done;}
 if(!inputs(d,n,s)||!sl_linux_deadline_boundary_matches_original(&o->identity_boundary)||!native(&o->identity_query,SYS_statx,o->identity_raw,o->fd,(long)empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0))goto done;
 if(o->identity_raw){o->result=o->identity_raw<0?o->identity_raw:-EINVAL;goto done;}
 if((o->identity.stx_mask&mask)!=mask||!S_ISREG(o->identity.stx_mode)||o->identity.stx_size||o->identity.stx_mnt_id!=d->identity.stx_mnt_id||o->identity.stx_dev_major!=d->identity.stx_dev_major||o->identity.stx_dev_minor!=d->identity.stx_dev_minor||o->identity.stx_ino==d->identity.stx_ino)goto done;
 if(writer&&(!sl_linux_failed_parent_file_matches_original(writer)||o->identity.stx_ino!=writer->identity.stx_ino||o->identity.stx_mnt_id!=writer->identity.stx_mnt_id||
    o->identity.stx_dev_major!=writer->identity.stx_dev_major||o->identity.stx_dev_minor!=writer->identity.stx_dev_minor||
    o->identity.stx_size!=writer->identity.stx_size||o->filesystem.f_type!=writer->filesystem.f_type))goto done;
 o->result=0;
 done:o->returned=1;if(!o->result&&!sl_linux_failed_parent_file_matches_original(o))o->result=-EINVAL;return o->result;
}
long sl_linux_create_failed_parent_writer(sl_linux_failed_parent_file *o,const sl_linux_failed_parent_directory *d,
 const sl_linux_failed_archive_name *n,sl_linux_deadline_sequence *s){return acquire(o,d,n,0,s);}
long sl_linux_open_failed_parent_reader(sl_linux_failed_parent_file *o,const sl_linux_failed_parent_file *w,sl_linux_deadline_sequence *s){
 if(!w||!sl_linux_failed_parent_file_matches_original(w)||w->reader||w->writer_original)return -EINVAL;
 return acquire(o,w->directory_original,w->name_original,w,s);
}
#endif
