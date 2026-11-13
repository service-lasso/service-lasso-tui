#if defined(__linux__) && defined(__x86_64__)
#include "local_mounts_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <limits.h>
static int mounts_fresh(sl_linux_call *c){
 if(c->source||c->number||atomic_load(&c->entered)||atomic_load(&c->returned))return -1;
 atomic_init(&c->entered,0);atomic_init(&c->returned,0);return 0;
}
static int mounts_number(const unsigned char *b,sl_linux_mount_field f,unsigned long *out){
 unsigned long n=0;if(!f.length)return -1;
 for(unsigned long i=0;i<f.length;i++){unsigned char c=b[f.offset+i];if(c<'0'||c>'9'||n>(ULONG_MAX-(c-'0'))/10)return -1;n=n*10+c-'0';}
 if(!n)return -1;*out=n;return 0;
}
static int mounts_equal(const unsigned char *b,sl_linux_mount_field f,const char *text){
 unsigned long i=0;while(text[i]){if(i>=f.length||b[f.offset+i]!=(unsigned char)text[i])return 0;i++;}return i==f.length;
}
static int mounts_token(const unsigned char *b,unsigned long end,unsigned long *cursor,sl_linux_mount_field *f){
 unsigned long at=*cursor;if(at>=end||b[at]==' ')return -1;f->offset=at;
 while(at<end&&b[at]!=' '){if(b[at]<32||b[at]==127)return -1;at++;}f->length=at-f->offset;
 *cursor=at<end?at+1:at;return f->length?0:-1;
}
static int mounts_path(const unsigned char *b,sl_linux_mount_field f){
 if(!f.length||b[f.offset]!='/')return -1;
 for(unsigned long i=0;i<f.length;i++)if(b[f.offset+i]=='\\'){
  if(f.length-i<4)return -1;
  unsigned char a=b[f.offset+i+1],d=b[f.offset+i+2],e=b[f.offset+i+3];
  if(!((a=='0'&&d=='4'&&e=='0')||(a=='0'&&d=='1'&&(e=='1'||e=='2'))||(a=='1'&&d=='3'&&e=='4')))return -1;i+=3;
 }
 return 0;
}
static int mounts_parse(sl_linux_local_mounts *o){
 unsigned long start=0;
 while(start<o->length){
  unsigned long end=start;while(end<o->length&&o->actual_bytes[end]!='\n')end++;
  if(end==o->length||end==start||o->actual_bytes[end-1]==' ')return -1;
  sl_linux_mount_row row={0};unsigned long at=start;sl_linux_mount_field separator;
  row.line.offset=start;row.line.length=end-start+1;
  if(mounts_token(o->actual_bytes,end,&at,&row.id)||mounts_token(o->actual_bytes,end,&at,&row.parent)||
     mounts_token(o->actual_bytes,end,&at,&row.device)||mounts_token(o->actual_bytes,end,&at,&row.root)||
     mounts_token(o->actual_bytes,end,&at,&row.mountpoint)||mounts_token(o->actual_bytes,end,&at,&row.options)||
     mounts_number(o->actual_bytes,row.id,&row.mount_id)||mounts_number(o->actual_bytes,row.parent,&row.parent_id)||
     mounts_path(o->actual_bytes,row.root)||mounts_path(o->actual_bytes,row.mountpoint))return -1;
  row.optional.offset=at;
  for(;;){if(mounts_token(o->actual_bytes,end,&at,&separator))return -1;
   if(mounts_equal(o->actual_bytes,separator,"-")){row.optional.length=separator.offset-row.optional.offset;break;}}
  if(mounts_token(o->actual_bytes,end,&at,&row.filesystem)||mounts_token(o->actual_bytes,end,&at,&row.source)||
     mounts_token(o->actual_bytes,end,&at,&row.super_options)||at!=end)return -1;
  if(mounts_equal(o->actual_bytes,row.filesystem,"cgroup2")){
   if(o->row_count==o->actual_row_capacity)return -1;
   for(unsigned long i=0;i<o->row_count;i++)if(o->actual_rows[i].mount_id==row.mount_id)return -1;
   o->actual_rows[o->row_count++]=row;
  }
  if(mounts_equal(o->actual_bytes,row.filesystem,"selinuxfs")){
   if(o->selinux_count==o->actual_selinux_capacity)return -1;
   for(unsigned long i=0;i<o->selinux_count;i++)if(o->actual_selinux_rows[i].mount_id==row.mount_id)return -1;
   o->actual_selinux_rows[o->selinux_count++]=row;
  }
  start=end+1;
 }
 return o->row_count?0:-1;
}
long sl_linux_observe_local_mounts(sl_linux_local_mounts *o){
 long result=-1;int held=-1;
 if(!o||o->attempted)return -1;o->attempted=1;o->fd=-1;
 o->actual_membership=o->membership_original;o->actual_bytes=o->bytes;o->actual_reads=o->reads;o->actual_rows=o->rows;
 o->actual_selinux_rows=o->selinux_rows;o->actual_selinux_capacity=o->selinux_capacity;
 if(o->length||o->read_count||o->row_count||o->selinux_count||o->eof||(o->actual_selinux_capacity&&!o->actual_selinux_rows))goto done;
 o->actual_capacity=o->capacity;o->actual_read_capacity=o->read_capacity;o->actual_row_capacity=o->row_capacity;
 if(!o->actual_membership||!o->actual_membership->returned||o->actual_membership->result||!o->actual_membership->eof||
    !o->actual_bytes||!o->actual_reads||!o->actual_rows||!o->actual_capacity||o->actual_capacity>LONG_MAX||
    !o->actual_read_capacity||!o->actual_row_capacity||o->lift.attempted||mounts_fresh(&o->open)||
    mounts_fresh(&o->filesystem_query)||mounts_fresh(&o->close))goto done;
 for(unsigned long i=0;i<o->actual_read_capacity;i++)if(mounts_fresh(&o->actual_reads[i]))goto done;
 o->raw_fd=sl_linux_call6(&o->open,"startup.mountinfo.open",SYS_openat,AT_FDCWD,(long)"/proc/thread-self/mountinfo",O_RDONLY|O_NONBLOCK|O_CLOEXEC,0,0,0);
 result=o->raw_fd;if(result<0)goto done;if(result>INT_MAX){result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&o->lift,(int)o->raw_fd);if(result<0)goto done;held=o->fd=(int)result;
 result=sl_linux_call6(&o->filesystem_query,"startup.mountinfo.procfs",SYS_fstatfs,held,(long)&o->filesystem,0,0,0,0);
 if(result){if(result>0)result=-1;goto cleanup;}if(o->filesystem.f_type!=PROC_SUPER_MAGIC){result=-1;goto cleanup;}
 while(o->read_count<o->actual_read_capacity){
  if(o->length==o->actual_capacity){result=-1;goto cleanup;}
  unsigned long remaining=o->actual_capacity-o->length;sl_linux_call *read=&o->actual_reads[o->read_count++];
  result=sl_linux_call6(read,"startup.mountinfo.read-frame",SYS_read,held,(long)(o->actual_bytes+o->length),remaining,0,0,0);
  if(result<0)goto cleanup;if((unsigned long)result>remaining){result=-1;goto cleanup;}
  if(!result){o->eof=1;break;}o->length+=(unsigned long)result;
 }
 result=o->eof?mounts_parse(o):-1;
cleanup:
 if(held>=0){long closed=sl_linux_call6(&o->close,"startup.mountinfo.close-once",SYS_close,held,0,0,0,0,0);if(!result&&closed)result=closed<0?closed:-1;}
done:o->result=result;o->returned=1;return result;
}
#endif