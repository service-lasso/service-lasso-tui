/* UNRUN held archive-label observations. No policy/actor/seal proof. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
typedef struct {sl_linux_public_startup startup;unsigned char bytes[3][32];} episode;
static episode cases[8];static episode *current;static unsigned mode,effects;
long sl_linux_call6(sl_linux_call *c,const char *source,long nr,long a,long b,long d,long e,long f,long g){
 effects++;assert(a>=10&&a<=12);unsigned index=(unsigned)a-10;sl_linux_archive_label *owner=&current->startup.archive_labels[index];
 c->source=source;c->number=nr;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;atomic_store(&c->entered,1);long result=0;
 if(nr==SYS_statx){*(struct statx*)f=*owner->identity_original;if(c==&owner->after){if(mode==1&&index==0)result=-EPERM;if(mode==2&&index==1)((struct statx*)f)->stx_ino++;}}
 else{assert(nr==SYS_fgetxattr&&!strcmp((char*)b,"security.selinux")&&d==(long)owner->actual_bytes&&e==32);memcpy((void*)d,"abc",4);result=4;if(mode==1&&index==0)result=-EIO;if(mode==3&&index==2)memcpy((void*)d,"a\0b",4);}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../archive_label_linux.c"
int main(void){for(mode=0;mode<8;mode++){
 episode *e=current=&cases[mode];sl_linux_public_startup *s=&e->startup;s->returned=1;s->archive_files.returned=1;s->archive_directory.returned=1;
 s->archive_files.local.returned=s->archive_files.final.returned=1;s->archive_files.local.fd=10;s->archive_files.final.fd=11;s->archive_directory.fd=12;
 struct statx *identities[3]={&s->archive_files.local.identity,&s->archive_files.final.identity,&s->archive_directory.identity};
 for(unsigned i=0;i<3;i++){identities[i]->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;identities[i]->stx_mode=(i==2?S_IFDIR:S_IFREG)|0600;identities[i]->stx_ino=10+i;identities[i]->stx_mnt_id=5;s->archive_labels[i].startup_original=s;s->archive_labels[i].kind=(enum sl_linux_archive_label_kind)i;s->archive_labels[i].bytes=e->bytes[i];s->archive_labels[i].capacity=32;}
 s->archive_precreation_returned=1;
 for(unsigned i=0;i<2;i++){sl_linux_archive_file *file=i?&s->archive_files.final:&s->archive_files.local;file->directory_original=&s->archive_directory;file->raw_fd=file->fd;file->lift.attempted=1;file->lift.original_fd=file->lift.held_fd=file->fd;file->how.flags=O_RDWR|O_CREAT|O_EXCL|O_NONBLOCK|O_CLOEXEC;file->how.mode=0600;file->how.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV;file->open.number=SYS_openat2;file->open.arguments[0]=12;file->open.arguments[1]=(long)file->name;file->open.arguments[2]=(long)&file->how;file->open.arguments[3]=sizeof(file->how);file->open.result=file->raw_fd;atomic_init(&file->open.entered,1);atomic_init(&file->open.returned,1);}
 if(mode==5)s->archive_files.local.how.flags&=~O_EXCL;
 if(mode==6)atomic_store(&s->archive_files.local.open.returned,0);
 if(mode==7)s->archive_files.local.directory_original=0;
 if(mode==4)s->archive_labels[0].bytes=(void*)&s->archive_labels[0].before.entered;
 unsigned before=effects;long result=sl_linux_observe_archive_labels(s);
 if(mode==4){assert(result==-EINVAL&&!s->archive_labels_attempted&&!s->archive_labels[0].attempted&&effects==before);continue;}
 assert(s->archive_labels_returned&&s->archive_labels_count==3&&effects==before+(mode>=5?6:9));
 if(!mode)assert(!result&&!s->archive_label_failure&&s->archive_labels[0].creation_original==&s->archive_files.local.open&&s->archive_labels[1].creation_directory_original==&s->archive_directory);
 if(mode>=5)assert(result==-EINVAL&&s->archive_label_failure==&s->archive_labels[0]&&!atomic_load(&s->archive_labels[0].before.entered)&&s->archive_labels[1].framing_complete&&s->archive_labels[2].framing_complete&&s->archive_labels[0].creation_original==&s->archive_files.local.open);
 if(mode==1)assert(result==-EIO&&s->archive_label_failure==&s->archive_labels[0]&&s->archive_labels[0].raw_count==-EIO&&s->archive_labels[0].after_result==-EPERM&&s->archive_labels[2].framing_complete);
 if(mode==2)assert(result==-EINVAL&&s->archive_label_failure==&s->archive_labels[1]&&s->archive_labels[1].after_state.stx_ino==12&&s->archive_files.final.identity.stx_ino==11&&s->archive_labels[2].framing_complete);
 if(mode==3)assert(result==-EINVAL&&s->archive_label_failure==&s->archive_labels[2]&&!s->archive_labels[2].framing_complete&&s->archive_labels[2].raw_count==4);
 before=effects;assert(sl_linux_observe_archive_labels(s)==-EINVAL&&effects==before);
 }return 0;}
