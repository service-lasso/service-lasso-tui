#if defined(__linux__) && defined(__x86_64__)
#include "failed_parent_directory_linux.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static const char empty[]="";
static int separate(const void *a,size_t n,const void *b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static int fresh(const void *p,size_t n){const unsigned char *b=p;for(size_t i=0;i<n;i++)if(b[i])return 0;return 1;}
static int prebirth(const sl_linux_outer_birth *b){return b&&b->prepared==1&&!b->attempted&&!atomic_load_explicit(&b->entered,memory_order_acquire)&&!atomic_load_explicit(&b->parent_captured,memory_order_acquire)&&!atomic_load_explicit(&b->child_captured,memory_order_acquire);}
static long clock_failure(long r){return r<0?r:-ETIMEDOUT;}
static int call(const sl_linux_call *c,long number,long raw,long a,long b,long d,long e,long f,long g){
 return atomic_load_explicit(&c->entered,memory_order_acquire)==1&&atomic_load_explicit(&c->returned,memory_order_acquire)==1&&c->number==number&&c->result==raw&&c->arguments[0]==a&&c->arguments[1]==b&&c->arguments[2]==d&&c->arguments[3]==e&&c->arguments[4]==f&&c->arguments[5]==g;
}
long sl_linux_acquire_failed_parent_directory(sl_linux_failed_parent_directory *o,
 const sl_candidate_profile *p,const sl_linux_outer_birth *birth,
 sl_linux_deadline_sequence *s,unsigned char *path,size_t capacity,
 sl_linux_archive_declaration *rows,size_t row_capacity){
 if(!row_capacity||row_capacity>SIZE_MAX/sizeof(*rows))return -EINVAL;
 size_t row_bytes=row_capacity*sizeof(*rows);
 if(!o||!capacity||!separate(o,sizeof(*o),path,capacity)||!separate(o,sizeof(*o),birth,sizeof(*birth))||!separate(path,capacity,birth,sizeof(*birth))||
    !separate(o,sizeof(*o),rows,row_bytes)||!separate(path,capacity,rows,row_bytes)||!separate(rows,row_bytes,birth,sizeof(*birth))||
    !sl_linux_archive_candidate_excludes_output(p,rows,row_bytes)||!sl_linux_deadline_sequence_excludes_output(s,rows,row_bytes)||
    !sl_linux_archive_candidate_excludes_output(p,o,sizeof(*o))||!sl_linux_archive_candidate_excludes_output(p,path,capacity)||
    !sl_linux_deadline_sequence_excludes_output(s,o,sizeof(*o))||!sl_linux_deadline_sequence_excludes_output(s,path,capacity)||
    !sl_linux_deadline_sequence_excludes_output(s,birth,sizeof(*birth)))return -EINVAL;
 /* Reciprocal clock writers cannot overwrite the candidate operands. Geometry
  * and actual observation-roster extent were checked by the sequence helper. */
 if(!sl_linux_archive_candidate_excludes_output(p,s,sizeof(*s))||
    !sl_linux_archive_candidate_excludes_output(p,s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations))||
    !fresh(o,sizeof(*o))||!fresh(path,capacity)||!fresh(rows,row_bytes)||!prebirth(birth)||s->failed||s->expired||s->exhausted||s->next>s->actual_capacity||s->actual_capacity-s->next<10)return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->candidate_original=p;o->birth_original=birth;o->sequence_original=s;
 o->path_original=path;o->path_capacity=capacity;o->raw_fd=o->fd=-1;
 long declarations_result=sl_linux_interpret_archive_declaration_roster(&o->declarations,p,rows,row_capacity);
 if(declarations_result){o->result=declarations_result;goto done;}
 if(!sl_linux_archive_declaration_roster_matches_original(&o->declarations))goto done;
 o->declaration_index=o->declarations.first_index;
 const sl_linux_archive_declaration *first=&rows[0];
 if(first->path_original.extent>=capacity){o->result=-EOVERFLOW;goto done;}
 o->path_extent=first->path_original.extent;
 for(size_t i=0;i<o->path_extent;i++)path[i]=first->path_original.bytes[i];path[o->path_extent]=0;
 o->how.flags=O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC;o->how.resolve=RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS;
 o->boundary_result=sl_linux_begin_deadline_boundary(&o->open_boundary,s);if(o->boundary_result){o->result=clock_failure(o->boundary_result);goto done;}
 atomic_init(&o->open.entered,0);atomic_init(&o->open.returned,0);
 o->open_raw=sl_linux_call6(&o->open,"failed-parent.directory.open",SYS_openat2,AT_FDCWD,(long)path,(long)&o->how,sizeof(o->how),0,0);
 if(o->open_raw>=0&&o->open_raw<=INT_MAX)o->raw_fd=(int)o->open_raw;
 o->boundary_result=sl_linux_end_deadline_boundary(&o->open_boundary);if(o->boundary_result){o->result=clock_failure(o->boundary_result);goto done;}
 if(!sl_linux_archive_declaration_roster_matches_original(&o->declarations)||!sl_linux_deadline_boundary_matches_original(&o->open_boundary)||!prebirth(birth)||!call(&o->open,SYS_openat2,o->open_raw,AT_FDCWD,(long)path,(long)&o->how,sizeof(o->how),0,0))goto done;
 if(o->raw_fd<0){o->result=o->open_raw<0?o->open_raw:-EINVAL;goto done;}
 o->lift_result=sl_linux_lift_deadline_fd(&o->lift,&o->open,s);
 if(o->lift_result<3||o->lift_result>INT_MAX){o->result=o->lift_result<0?o->lift_result:-EINVAL;goto done;}o->fd=(int)o->lift_result;
 o->boundary_result=sl_linux_begin_deadline_boundary(&o->filesystem_boundary,s);if(o->boundary_result){o->result=clock_failure(o->boundary_result);goto done;}
 atomic_init(&o->filesystem_query.entered,0);atomic_init(&o->filesystem_query.returned,0);
 o->filesystem_raw=sl_linux_call6(&o->filesystem_query,"failed-parent.directory.filesystem",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);
 o->boundary_result=sl_linux_end_deadline_boundary(&o->filesystem_boundary);if(o->boundary_result){o->result=clock_failure(o->boundary_result);goto done;}
 if(!sl_linux_archive_declaration_roster_matches_original(&o->declarations)||!sl_linux_deadline_boundary_matches_original(&o->filesystem_boundary)||!prebirth(birth)||!call(&o->filesystem_query,SYS_fstatfs,o->filesystem_raw,o->fd,(long)&o->filesystem,0,0,0,0))goto done;
 if(o->filesystem_raw){o->result=o->filesystem_raw<0?o->filesystem_raw:-EINVAL;goto done;}
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 o->boundary_result=sl_linux_begin_deadline_boundary(&o->identity_boundary,s);if(o->boundary_result){o->result=clock_failure(o->boundary_result);goto done;}
 atomic_init(&o->identity_query.entered,0);atomic_init(&o->identity_query.returned,0);
 o->identity_raw=sl_linux_call6(&o->identity_query,"failed-parent.directory.identity",SYS_statx,o->fd,(long)empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0);
 o->boundary_result=sl_linux_end_deadline_boundary(&o->identity_boundary);if(o->boundary_result){o->result=clock_failure(o->boundary_result);goto done;}
 if(!sl_linux_archive_declaration_roster_matches_original(&o->declarations)||!sl_linux_deadline_boundary_matches_original(&o->identity_boundary)||!prebirth(birth)||!call(&o->identity_query,SYS_statx,o->identity_raw,o->fd,(long)empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0))goto done;
 if(o->identity_raw){o->result=o->identity_raw<0?o->identity_raw:-EINVAL;goto done;}
 if((o->identity.stx_mask&mask)!=mask||!S_ISDIR(o->identity.stx_mode)||first->tuple[0]!=o->identity.stx_dev_major||first->tuple[1]!=o->identity.stx_dev_minor||first->tuple[2]!=o->identity.stx_ino||first->tuple[3]!=o->identity.stx_mnt_id)goto done;
 o->result=0;
 done:o->returned=1;return o->result;
}
int sl_linux_failed_parent_directory_matches_original(const sl_linux_failed_parent_directory *o){
 if(!o||o->attempted!=1||o->returned!=1||o->result||o->fd<3||o->raw_fd<0||!prebirth(o->birth_original)||
    !o->path_capacity||o->path_extent>=o->path_capacity||
    !separate(o,sizeof(*o),o->path_original,o->path_capacity)||!separate(o,sizeof(*o),o->birth_original,sizeof(*o->birth_original))||
    !separate(o->path_original,o->path_capacity,o->birth_original,sizeof(*o->birth_original))||
    !sl_linux_archive_candidate_excludes_output(o->candidate_original,o,sizeof(*o))||
    !sl_linux_archive_declaration_roster_matches_original(&o->declarations)||
    o->declarations.candidate_original!=o->candidate_original||o->declaration_index!=o->declarations.first_index||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o,sizeof(*o))||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o->path_original,o->path_capacity)||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o->birth_original,sizeof(*o->birth_original))||
    !sl_linux_archive_candidate_excludes_output(o->candidate_original,o->path_original,o->path_capacity))return 0;
 const sl_linux_deadline_sequence *s=o->sequence_original;
 if(!sl_linux_archive_candidate_excludes_output(o->candidate_original,s,sizeof(*s))||
    !sl_linux_archive_candidate_excludes_output(o->candidate_original,s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations))||
    o->open_boundary.first>s->actual_capacity||s->actual_capacity-o->open_boundary.first<10)return 0;
 size_t rows_bytes=o->declarations.capacity*sizeof(*o->declarations.rows_original);
 if(!separate(o,sizeof(*o),o->declarations.rows_original,rows_bytes)||
    !separate(o->path_original,o->path_capacity,o->declarations.rows_original,rows_bytes)||
    !separate(o->declarations.rows_original,rows_bytes,o->birth_original,sizeof(*o->birth_original))||
    !sl_linux_deadline_sequence_excludes_output(o->sequence_original,o->declarations.rows_original,rows_bytes))return 0;
 const sl_linux_archive_declaration *first=&o->declarations.rows_original[0];
 if(o->path_extent!=first->path_original.extent||o->path_original[o->path_extent])return 0;
 for(size_t i=0;i<o->path_extent;i++)if(o->path_original[i]!=first->path_original.bytes[i])return 0;
 if(o->how.flags!=(O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC)||o->how.mode||o->how.resolve!=(RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS)||
    o->open_raw!=o->raw_fd||!call(&o->open,SYS_openat2,o->open_raw,AT_FDCWD,(long)o->path_original,(long)&o->how,sizeof(o->how),0,0)||
    o->lift.producer_original!=&o->open||o->lift.sequence_original!=o->sequence_original||o->lift_result!=o->fd||
    !sl_linux_deadline_fd_lift_matches_original(&o->lift)||o->lift.result!=o->fd||o->filesystem_raw||o->identity_raw||o->boundary_result)return 0;
 const sl_linux_deadline_boundary *boundaries[3]={&o->open_boundary,&o->filesystem_boundary,&o->identity_boundary};
 for(unsigned i=0;i<3;i++)if(boundaries[i]->sequence!=o->sequence_original||!sl_linux_deadline_boundary_matches_original(boundaries[i]))return 0;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 return call(&o->filesystem_query,SYS_fstatfs,0,o->fd,(long)&o->filesystem,0,0,0,0)&&
    call(&o->identity_query,SYS_statx,0,o->fd,(long)empty,AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0)&&
    (o->identity.stx_mask&mask)==mask&&S_ISDIR(o->identity.stx_mode)&&first->tuple[0]==o->identity.stx_dev_major&&
    first->tuple[1]==o->identity.stx_dev_minor&&first->tuple[2]==o->identity.stx_ino&&first->tuple[3]==o->identity.stx_mnt_id;
}
#endif
