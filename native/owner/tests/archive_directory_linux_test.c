/* UNRUN actual archive-directory constructor with substituted native calls.
 * No filesystem durability, policy authorization or real identity is proved. */
#if defined(__linux__) && defined(__x86_64__)
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
static unsigned calls,samples;static unsigned mode;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;atomic_store(&c->entered,1);calls++;
 long r=0;if(number==SYS_openat2){assert(!strcmp((const char*)b,"/vault"));struct open_how *how=(void*)d;assert(how->flags&(O_NONBLOCK));r=mode==4?-EACCES:40;}
 else if(number==SYS_fstatfs){((struct statfs*)b)->f_type=0xEF53;}
 else if(number==SYS_statx){struct statx *s=(void*)f;s->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;s->stx_mode=mode==7?S_IFREG:S_IFDIR;s->stx_ino=mode==5?23:22;s->stx_mnt_id=33;}
 else assert(0);
 c->result=r;atomic_store(&c->returned,1);return r;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=lift->held_fd=fd;return fd;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;s->result=mode==4||mode==8?-77:0;return s->result;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return mode!=9;}
#include "../archive_directory_linux.c"
typedef struct {sl_linux_public_startup startup;sl_linux_candidate_policy policy;sl_candidate_profile candidate;sl_candidate_record evidence,rows[2];unsigned char link[24],value[2][46],body[10],path[64];sl_linux_archive_declaration declarations[2];sl_linux_selinux_status_sample status;} fixture;
static void put(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
static void prepare(fixture *f){
 sl_linux_public_startup *s=&f->startup;s->returned=s->candidate_file_entry_returned=1;s->stage=SL_STARTUP_ACQUIRED;s->actual_candidate_policies=&f->policy;s->actual_candidate_policy_capacity=s->mounts.selinux_count=1;
 f->policy.returned=1;f->policy.actual_candidate=&f->candidate;f->candidate.returned=1;f->candidate.actual_rows[0]=&f->evidence;f->candidate.counts[0]=1;f->candidate.actual_rows[3]=f->rows;f->candidate.counts[3]=2;
 f->evidence.field_count=7;f->evidence.fields[6]=(sl_candidate_span){f->body,sizeof(f->body)};put(f->link+16,1);
 for(unsigned i=0;i<2;i++){put(f->value[i]+16,22);put(f->value[i]+24,33);put(f->value[i]+32,6);memcpy(f->value[i]+40,"/vault",6);f->rows[i].field_count=3;f->rows[i].fields[0]=(sl_candidate_span){f->link,24};f->rows[i].fields[1]=(sl_candidate_span){(const unsigned char*)"service-lasso.archive-directory-object.linux.v1",sizeof("service-lasso.archive-directory-object.linux.v1")-1};f->rows[i].fields[2]=(sl_candidate_span){f->value[i],46};}
 s->archive_directory.startup_original=s;s->archive_directory.path=f->path;s->archive_directory.path_capacity=sizeof(f->path);s->archive_directory.declarations=f->declarations;s->archive_directory.declaration_capacity=2;s->archive_directory.statuses=&f->status;s->archive_directory.status_capacity=1;
}
int main(void){fixture cases[10]={0};for(mode=0;mode<10;mode++){
 fixture *f=&cases[mode];prepare(f);if(mode==1)f->value[1][45]='x';if(mode==2)put(f->link+16,11);if(mode==3)f->startup.archive_directory.path_capacity=6;if(mode==6){f->candidate.counts[3]=0;}
 unsigned before=calls,prior_samples=samples;long r=sl_linux_acquire_archive_directory(&f->startup.archive_directory);sl_linux_archive_directory *o=&f->startup.archive_directory;
 assert(o->returned&&samples==prior_samples+1&&o->actual_path==f->path&&o->actual_declarations==f->declarations);
 if(!mode){assert(!r&&calls==before+3&&o->fd==40&&o->declaration_count==2&&!strcmp((char*)f->path,"/vault"));for(unsigned i=0;i<2;i++)assert(f->declarations[i].declaration_original==&f->rows[i]&&f->declarations[i].evidence_original==&f->evidence&&f->declarations[i].statement_original.bytes==f->body&&f->declarations[i].path_original.bytes==f->value[i]+40);}
 else if(mode==4)assert(r==-EACCES&&o->status_result==-77&&calls==before+1&&o->raw_fd==-1);
 else if(mode==8)assert(r==-77&&!o->primary_result&&o->fd==40&&calls==before+3);
 else if(mode==9)assert(r==-EINVAL&&!o->primary_result&&o->status_result==-EINVAL&&o->fd==40);
 else {assert(r<0);if(mode==1||mode==2||mode==3||mode==6)assert(calls==before);else assert(o->fd==40&&calls==before+3);}
 before=calls;assert(sl_linux_acquire_archive_directory(o)==-EINVAL&&calls==before);
 }return 0;
}
#endif