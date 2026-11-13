/* UNRUN source fixture; substituted calls do not establish storage authority,
 * actual filesystem durability, sealing or independent LOCAL/FINAL acceptance. */
#if defined(__linux__) && defined(__x86_64__)
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static unsigned mode,calls,opens,samples;
long sl_linux_call6(sl_linux_call *c,const char *source,long n,long a,long b,long d,long e,long f,long g){
 c->source=source;c->number=n;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;atomic_store(&c->entered,1);calls++;long r=0;
 if(n==SYS_getpid)r=mode==1?-EPERM:123;
 else if(n==SYS_clock_gettime){struct timespec *t=(void*)b;t->tv_sec=456;t->tv_nsec=789;if(mode==2)r=-EIO;if(mode==3)t->tv_nsec=1000000000L;}
 else if(n==SYS_openat2){opens++;assert(a==30);struct open_how *h=(void*)d;assert(h->flags==(O_RDWR|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC)&&h->mode==0600);assert(h->resolve==(RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV));const char *name=(void*)b;assert(!strncmp(name,"service-lasso-tui.",17));assert(strstr(name,".123.456.789."));r=40+opens;if(mode==4&&opens==1)r=-EEXIST;if((mode==5||mode==10)&&opens==2)r=-EACCES;}
 else if(n==SYS_fstatfs)((struct statfs*)b)->f_type=0xEF53;
 else if(n==SYS_statx){struct statx *x=(void*)f;x->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;x->stx_mode=mode==6?S_IFIFO:S_IFREG;x->stx_ino=mode==7?50:(uint64_t)a+100;x->stx_mnt_id=60;if(mode==8)x->stx_size=1;if(mode==9&&a==42)x->stx_ino=141;}
 else assert(0);c->result=r;atomic_store(&c->returned,1);return r;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){l->attempted=1;l->original_fd=l->held_fd=fd;return fd;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;s->result=mode==10?-77:0;return s->result;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return mode!=11;}
#include "../archive_files_linux.c"
typedef struct {sl_linux_public_startup startup;sl_candidate_profile candidate;sl_linux_candidate_policy policy;sl_linux_selinux_status status;sl_linux_selinux_status_sample sample;unsigned char identity[32],label[3];sl_linux_selinux_context contexts[3];sl_linux_archive_access access[25];sl_linux_archive_creation creation;} fixture;
int main(void){static fixture cases[16];for(mode=0;mode<16;mode++){
 fixture *f=&cases[mode];sl_linux_public_startup *s=&f->startup;sl_linux_archive_files *o=&s->archive_files;o->startup_original=s;o->statuses=&f->sample;o->status_capacity=1;s->actual_candidate_policies=&f->policy;s->actual_candidate_policy_capacity=s->mounts.selinux_count=1;f->policy.returned=1;f->policy.after_binding.status_original=&f->status;s->archive_directory.returned=1;s->archive_directory.fd=30;s->archive_directory.candidate_original=&f->candidate;s->archive_directory.identity.stx_ino=50;s->archive_directory.identity.stx_mnt_id=60;f->candidate.returned=1;f->candidate.source_identity=(sl_candidate_span){f->identity,32};opens=0;unsigned before=calls,prior_samples=samples;
 s->actual_parse_capacity=1;s->archive_precreation_attempted=s->archive_precreation_returned=1;s->actual_archive_contexts=f->contexts;s->actual_archive_context_capacity=3;s->archive_accesses=f->access;s->archive_access_capacity=25;
 sl_linux_archive_label *label=&s->archive_labels[2];label->attempted=label->returned=label->framing_complete=1;label->actual_startup=s;label->object_original=&s->archive_directory;label->identity_original=&s->archive_directory.identity;label->fd=30;label->bytes=label->actual_bytes=f->label;label->capacity=label->actual_capacity=3;
 s->archive_creations=&f->creation;s->archive_creation_capacity=1;f->creation.returned=1;sl_linux_selinux_context *context=&f->contexts[2];context->archive_attempted=context->archive_returned=context->attempted=context->returned=1;context->archive_original=label;
 for(unsigned purpose=12;purpose<25;purpose++){sl_linux_archive_access *access=&f->access[purpose];access->attempted=access->returned=1;access->startup_original=s;access->purpose=purpose;access->context_original=context;if(purpose>=19){access->creation_original=&f->creation;access->inventory.actual_creation=&f->creation.types;access->inventory.returned=1;}}
 if(mode==12)s->archive_precreation_returned=0;
 if(mode==13)f->access[15].result=-89; if(mode==15)f->access[24].inventory.actual_creation=0;
 if(mode==14)context->archive_original=&s->archive_labels[0];
 long r=sl_linux_create_archive_files(o);
 if(mode>=12){assert(r==-EINVAL&&o->returned&&o->actual_startup==s&&calls==before&&samples==prior_samples&&!opens&&!o->local.attempted&&!o->final.attempted);if(mode==13)assert(f->access[15].result==-89);assert(sl_linux_create_archive_files(o)==-EINVAL&&calls==before);continue;}
 assert(samples==prior_samples+1);assert(o->returned&&o->actual_startup==s&&o->source_identity_original.bytes==f->identity);
 if(mode==0){assert(!r&&o->local.fd==41&&o->final.fd==42&&o->local.identity.stx_ino!=o->final.identity.stx_ino);assert(strcmp((char*)o->local.name,(char*)o->final.name));assert(o->local.name_extent<sizeof(o->local.name)&&o->final.name_extent<sizeof(o->final.name));assert(!strcmp((char*)o->local.name+o->local.name_extent-6,".local")&&!strcmp((char*)o->final.name+o->final.name_extent-6,".final"));}
 else {assert(r<0);if(mode<=3)assert(!o->local.attempted&&!o->final.attempted);if(mode==4)assert(o->local.raw_fd==-1&&!o->final.attempted);if(mode==5)assert(o->local.fd==41&&!o->local.result&&o->final.raw_fd==-1);if(mode>=6)assert(o->local.raw_fd==41);if(mode==10)assert(r==-EACCES&&o->primary_result==-EACCES&&o->status_result==-77&&o->status_failure_original==&f->sample);if(mode==11)assert(r==-EINVAL&&!o->primary_result&&o->status_result==-EINVAL);}
 unsigned after=calls;assert(after>before);assert(sl_linux_create_archive_files(o)==-EINVAL&&calls==after);
 }return 0;}
#endif