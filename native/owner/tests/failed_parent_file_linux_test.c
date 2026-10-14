/* Actual file/name/FD-lift bodies; synthetic policy/directory, clock and native
 * substitutes. UNRUN: no authentic parent, provider, descriptor or finance. */
#include "../failed_parent_file_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
static const sl_linux_failed_parent_directory *selected_directory;
static unsigned mode,calls,ends;
static int apart(const void *a,size_t n,const void *b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
int sl_linux_failed_parent_directory_matches_original(const sl_linux_failed_parent_directory *d){return d==selected_directory&&d->birth_original->prepared==1&&!d->birth_original->attempted;}
int sl_linux_archive_candidate_excludes_output(const sl_candidate_profile *p,const void *out,size_t bytes){return p&&apart(out,bytes,p,sizeof(*p))&&apart(out,bytes,p->original.bytes,p->original.extent);}
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *s,const void *p,size_t n){return s&&s->actual_capacity&&s->actual_capacity<=SIZE_MAX/sizeof(*s->actual_observations)&&apart(p,n,s,sizeof(*s))&&apart(p,n,s->actual_observations,s->actual_capacity*sizeof(*s->actual_observations));}
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){b->sequence=s;b->first=s->next;b->attempted=b->ready=1;s->next+=2;return 0;}
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *b){b->returned=b->after_attempted=1;ends++;return mode==2&&ends==1?1:0;}
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *b){return b->returned;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 calls++;c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;long raw=0;
 if(number==SYS_openat2){const struct open_how *h=(const struct open_how *)d;assert(a==5&&((char *)b)[0]=='s');assert(h->resolve==(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV));if(h->flags&O_CREAT){assert(h->flags==(O_WRONLY|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC)&&h->mode==0600);raw=mode==1?-EEXIST:mode==3?0:7;}else{assert(h->flags==(O_RDONLY|O_NONBLOCK|O_CLOEXEC)&&!h->mode);raw=8;}}
 else if(number==SYS_fcntl){assert(a==0&&b==F_DUPFD_CLOEXEC&&d==3);raw=7;}
 else if(number==SYS_close){assert(a==0);raw=0;}
 else if(number==SYS_fstatfs)((struct statfs *)b)->f_type=0xEF53;
 else if(number==SYS_statx){struct statx *v=(struct statx *)f;v->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;v->stx_mode=S_IFREG;v->stx_dev_major=1;v->stx_dev_minor=2;v->stx_ino=mode==4&&a==8?99:6;v->stx_mnt_id=4;}
 else assert(0);
 c->result=raw;atomic_store(&c->entered,1);atomic_store(&c->returned,1);return raw;
}
int main(void){
 unsigned char body[32]={0},path[16]="/archive";sl_candidate_profile candidate={.source=body,.source_extent=sizeof(body),.original={body,sizeof(body)},.source_identity={body,32},.returned=1};
 sl_linux_call process={.number=SYS_getpid,.result=11},clock={.number=SYS_clock_gettime};struct timespec instant={.tv_sec=12,.tv_nsec=13};
 clock.arguments[0]=CLOCK_MONOTONIC;clock.arguments[1]=(long)&instant;atomic_init(&process.entered,1);atomic_init(&process.returned,1);atomic_init(&clock.entered,1);atomic_init(&clock.returned,1);
 sl_linux_failed_archive_name name={0};assert(!sl_linux_construct_failed_archive_name(&name,body,32,&process,&clock,&instant));
 sl_linux_outer_birth birth={.prepared=1};sl_linux_archive_declaration declaration={0};sl_linux_deadline_observation observations[32]={0};
 sl_linux_deadline_sequence sequence={.actual_observations=observations,.actual_capacity=32};
 sl_linux_failed_parent_directory directory={.candidate_original=&candidate,.birth_original=&birth,.sequence_original=&sequence,.path_original=path,.path_capacity=sizeof(path),.fd=5};
 directory.declarations.rows_original=&declaration;directory.declarations.capacity=1;directory.identity.stx_dev_major=1;directory.identity.stx_dev_minor=2;directory.identity.stx_mnt_id=4;directory.identity.stx_ino=3;selected_directory=&directory;
 for(mode=0;mode<5;mode++){
  calls=ends=0;sequence.next=0;sl_linux_failed_parent_file writer={0},reader={0};long r=sl_linux_create_failed_parent_writer(&writer,&directory,&name,&sequence);
  if(mode==1)assert(r==-EEXIST&&calls==1&&writer.raw_fd==-1&&!writer.lift.attempted);
  else if(mode==2)assert(r==-ETIMEDOUT&&calls==1&&writer.raw_fd==7&&!writer.lift.attempted);
  else{
   assert(!r&&writer.fd==7&&sl_linux_failed_parent_file_matches_original(&writer));assert(calls==(mode==3?5:3));
   r=sl_linux_open_failed_parent_reader(&reader,&writer,&sequence);
   if(mode==4)assert(r<0&&reader.fd==8&&reader.identity.stx_ino==99);
   else{assert(!r&&reader.fd==8&&reader.writer_original==&writer&&sl_linux_failed_parent_file_matches_original(&reader));reader.open.arguments[0]=9;assert(!sl_linux_failed_parent_file_matches_original(&reader));reader.open.arguments[0]=5;assert(sl_linux_failed_parent_file_matches_original(&reader));}
   sl_linux_failed_parent_file saved=writer;assert(sl_linux_open_failed_parent_reader(&writer,&writer,&sequence)<0&&!memcmp(&writer,&saved,sizeof(writer)));
   writer.open.arguments[3]++;assert(!sl_linux_failed_parent_file_matches_original(&writer));writer.open.arguments[3]--;
   saved=writer;assert(sl_linux_create_failed_parent_writer(&writer,&directory,&name,&sequence)<0&&!memcmp(&writer,&saved,sizeof(writer)));
  }
 }
 mode=0;sequence.next=0;calls=0;sl_linux_failed_parent_file output={0};sl_linux_deadline_observation *old=sequence.actual_observations;
 sequence.actual_observations=(sl_linux_deadline_observation *)body;unsigned char snapshot[32];memcpy(snapshot,body,32);
 assert(sl_linux_create_failed_parent_writer(&output,&directory,&name,&sequence)<0&&!output.attempted&&!calls&&!memcmp(snapshot,body,32));sequence.actual_observations=old;
 sequence.actual_capacity=9;assert(sl_linux_create_failed_parent_writer(&output,&directory,&name,&sequence)<0&&!output.attempted&&!calls);sequence.actual_capacity=32;
 /* Actual file preflight rejects a writable occurrence covering a name source. */
 sl_linux_failed_parent_file storage={0};sl_linux_failed_archive_name *alias=(sl_linux_failed_archive_name *)&storage;
 memcpy(alias,&name,sizeof(name));sl_linux_failed_parent_file saved=storage;
 assert(sl_linux_create_failed_parent_writer(&storage,&directory,alias,&sequence)<0&&!calls&&!memcmp(&storage,&saved,sizeof(saved)));
 return 0;
}
