/* Controlled source cases UNRUN: substituted calls are not kernel/policy proof. */
#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "../cgroup_control_label_linux.h"
#include <sys/syscall.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <fcntl.h>
static unsigned calls,mode;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 c->source=source;c->number=number;atomic_store(&c->entered,1);calls++;
 if(number==SYS_openat2){assert(a==20);struct open_how *h=(void*)d;assert(h->flags&O_NONBLOCK);assert(!strcmp((char*)b,"cgroup.procs")||!strcmp((char*)b,"cgroup.threads"));c->result=21;}
 else if(number==SYS_fstatfs){((struct statfs*)b)->f_type=CGROUP2_SUPER_MAGIC;c->result=0;}
 else if(number==SYS_statx){struct statx *s=(void*)f;s->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;s->stx_mode=S_IFREG;s->stx_mnt_id=9;c->result=0;}
 else {assert(number==SYS_fgetxattr&&a==21&&!strcmp((char*)b,"security.selinux"));memcpy((void*)d,"a:b:c",6);c->result=mode==1?-ERANGE:mode==2?e+1:mode==3?5:6;}
 atomic_store(&c->returned,1);return c->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){(void)l;return fd;}
#include "../cgroup_control_label_linux.c"
int main(void){
 sl_linux_cgroup_children children;sl_linux_child_view view;sl_linux_cgroup_control_label root;unsigned char bytes[32];
 for(mode=0;mode<4;mode++)for(unsigned kind=1;kind<=2;kind++){
 memset(&children,0,sizeof children);memset(&view,0,sizeof view);memset(&root,0,sizeof root);memset(bytes,0,sizeof bytes);calls=0;
 children.attempted=children.returned=children.eof=1;children.actual_views=&view;children.view_count=children.actual_view_capacity=1;
 view.attempted=view.returned=1;view.fd=20;view.filesystem.f_type=CGROUP2_SUPER_MAGIC;view.identity.stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;view.identity.stx_mode=S_IFDIR;view.identity.stx_mnt_id=9;
 root.children_original=&children;root.view_original=&view;root.kind=kind;root.bytes=bytes;root.capacity=sizeof bytes;
 long result=sl_linux_acquire_cgroup_control_label(&root);assert(root.returned&&calls==4&&root.raw_fd==21&&root.fd==21&&root.actual_bytes==bytes);
 assert(mode?result<0:result==0);if(!mode)assert(root.raw_extent==6&&root.extent==5&&root.framing_complete);if(mode==1)assert(root.raw_label_count==-ERANGE);
 assert(sl_linux_acquire_cgroup_control_label(&root)==-1&&calls==4);
 }
 memset(&root,0,sizeof root);calls=0;root.children_original=&children;root.view_original=&view;root.kind=1;root.bytes=bytes;root.capacity=sizeof bytes;children.view_count=2;
 assert(sl_linux_acquire_cgroup_control_label(&root)==-1&&!calls&&root.returned);
 return 0;
}
#endif