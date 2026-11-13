#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
static void reader_call_init(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
long sl_linux_acquire_archive_read_view(sl_linux_archive_read_view *o){
 if(!o||o->attempted)return -EINVAL;
 sl_linux_public_startup *s=o->startup_original;
 /* Embedded source roots are disjoint fresh arena members, not caller-selected
  * file/identity arguments. Validate association before any owner write. */
 if(!s||o!=&s->final_reader)return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->raw_fd=o->fd=-1;o->actual_startup=s;
 o->file_original=&s->archive_files.final;o->directory_original=&s->archive_directory;
 sl_linux_archive_file *f=o->file_original;sl_linux_archive_directory *d=o->directory_original;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 if(!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->archive_files.returned||s->archive_files.result||!f->returned||f->result||f->fd<3||f->directory_original!=d||!d->returned||d->result||d->fd<3||!f->name_extent||f->name_extent>=sizeof f->name||f->name[f->name_extent]||!atomic_load_explicit(&f->identity_query.returned,memory_order_acquire)||f->identity_query.result||(f->identity.stx_mask&mask)!=mask||!S_ISREG(f->identity.stx_mode))goto done;
 o->how.flags=O_RDONLY|O_NONBLOCK|O_CLOEXEC;
 o->how.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV;
 reader_call_init(&o->open);long r=sl_linux_call6(&o->open,"archive.independent.reader.open",SYS_openat2,d->fd,(long)f->name,(long)&o->how,sizeof o->how,0,0);
 if(r<0||r>INT_MAX){o->result=r<0?r:-EINVAL;goto done;}o->raw_fd=(int)r;
 r=sl_linux_lift_internal_fd(&o->lift,o->raw_fd);if(r<0||r>INT_MAX){o->result=r<0?r:-EINVAL;goto done;}o->fd=(int)r;
 reader_call_init(&o->filesystem_query);r=sl_linux_call6(&o->filesystem_query,"archive.independent.reader.filesystem",SYS_fstatfs,o->fd,(long)&o->filesystem,0,0,0,0);if(r){o->result=r;goto done;}
 reader_call_init(&o->identity_query);r=sl_linux_call6(&o->identity_query,"archive.independent.reader.identity",SYS_statx,o->fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)&o->identity,0);if(r){o->result=r;goto done;}
 if((o->identity.stx_mask&mask)!=mask||!S_ISREG(o->identity.stx_mode)||o->identity.stx_dev_major!=f->identity.stx_dev_major||o->identity.stx_dev_minor!=f->identity.stx_dev_minor||o->identity.stx_ino!=f->identity.stx_ino||o->identity.stx_mnt_id!=f->identity.stx_mnt_id||o->identity.stx_size!=f->identity.stx_size||o->filesystem.f_type!=f->filesystem.f_type||o->fd==f->fd||o->fd==d->fd)goto done;
 o->result=0;
 done:o->returned=1;return o->result;
}
#endif