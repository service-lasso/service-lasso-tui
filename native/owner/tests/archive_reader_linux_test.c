/* UNRUN source-owned read-view acquisition. No seal/ACK/native proof. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static unsigned mode,calls;static sl_linux_public_startup *current;
long sl_linux_call6(sl_linux_call *c,const char *source,long nr,long a,long b,long d,long e,long f,long g){
 calls++;c->source=source;c->number=nr;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;atomic_store(&c->entered,1);long r=0;
 if(nr==SYS_openat2){struct open_how *how=(void*)d;assert(a==3&&!strcmp((char*)b,"original.final")&&!(how->flags&O_CREAT)&&(how->flags&O_NONBLOCK)&&(how->flags&O_ACCMODE)==O_RDONLY);r=mode==1?-EIO:mode==4?0:8;}
 else if(nr==SYS_fstatfs){assert(a==8);*(struct statfs*)b=current->archive_files.final.filesystem;if(mode==2)r=-EPERM;}
 else if(nr==SYS_statx){assert(a==8);*(struct statx*)f=current->archive_files.final.identity;if(mode==3)((struct statx*)f)->stx_ino++;}
 else assert(0);
 c->result=r;atomic_store(&c->returned,1);return r;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){l->attempted=1;l->original_fd=fd;l->held_fd=8;return mode==4?-EINTR:8;}
#include "../archive_reader_linux.c"
static sl_linux_public_startup cases[6];
int main(void){for(mode=0;mode<6;mode++){
 sl_linux_public_startup *s=current=&cases[mode];s->returned=1;s->stage=SL_STARTUP_ACQUIRED;s->archive_directory.returned=1;s->archive_directory.fd=3;
 s->archive_files.returned=1;sl_linux_archive_file *f=&s->archive_files.final;f->directory_original=&s->archive_directory;f->returned=1;f->fd=4;strcpy((char*)f->name,"original.final");f->name_extent=strlen((char*)f->name);f->identity.stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;f->identity.stx_mode=S_IFREG|0600;f->identity.stx_ino=91;f->identity.stx_mnt_id=17;f->identity.stx_dev_major=8;f->filesystem.f_type=0xEF53;atomic_init(&f->identity_query.entered,1);atomic_init(&f->identity_query.returned,1);
 sl_linux_archive_read_view *v=&s->final_reader;v->startup_original=s;if(mode==5)f->name_extent=sizeof f->name;unsigned before=calls;long result=sl_linux_acquire_archive_read_view(v);assert(v->returned&&v->file_original==f&&v->directory_original==&s->archive_directory);
 if(!mode)assert(!result&&v->fd==8&&v->raw_fd==8&&calls==before+3&&v->identity.stx_ino==f->identity.stx_ino);
 if(mode==1)assert(result==-EIO&&v->raw_fd==-1&&calls==before+1);
 if(mode==2)assert(result==-EPERM&&v->raw_fd==8&&v->fd==8&&calls==before+2);
 if(mode==3)assert(result==-EINVAL&&v->identity.stx_ino==92&&f->identity.stx_ino==91);
 if(mode==4)assert(result==-EINTR&&v->raw_fd==0&&v->fd==-1&&v->lift.held_fd==8&&calls==before+1);
 if(mode==5)assert(result==-EINVAL&&calls==before);
 before=calls;assert(sl_linux_acquire_archive_read_view(v)==-EINVAL&&calls==before);
 }return 0;}