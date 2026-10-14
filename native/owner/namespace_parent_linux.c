#if defined(__linux__) && defined(__x86_64__)
#include "namespace_parent_linux.h"
#include <sys/syscall.h>
#include <linux/nsfs.h>
#include <linux/magic.h>
#include <fcntl.h>
#include <limits.h>
static int init_call(sl_linux_call *c){
 if(c->source||c->number||atomic_load(&c->entered)||atomic_load(&c->returned))return -1;
 atomic_init(&c->entered,0);atomic_init(&c->returned,0);return 0;}
static long hold(sl_linux_namespace_view *v,long raw){
 v->raw_fd=raw;if(raw<0)return raw;if(raw>INT_MAX)return -1;
 long held=sl_linux_lift_internal_fd(&v->lift,(int)raw);if(held<0)return held;v->fd=(int)held;return 0;
}
static long open_view(sl_linux_namespace_view *v,int root,const char *path,int flags){
 return hold(v,sl_linux_call6(&v->acquire,"controller.namespace.open-held",SYS_openat,root,(long)path,flags,0,0,0));
}
static long filesystem(sl_linux_namespace_view *v,long magic){
 long result=sl_linux_call6(&v->filesystem_query,"controller.namespace.filesystem",SYS_fstatfs,v->fd,(long)&v->filesystem,0,0,0,0);
 if(result<0)return result;if(result)return -1;return v->filesystem.f_type==magic?0:-1;
}
static long identity(sl_linux_namespace_view *v){
 long result=sl_linux_call6(&v->identity_query,"controller.namespace.held-identity",SYS_statx,v->fd,(long)"",
                          AT_EMPTY_PATH,STATX_TYPE|STATX_INO|STATX_MNT_ID,(long)&v->identity,0);
 if(result<0)return result;if(result)return -1;
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;
 return (v->identity.stx_mask&mask)==mask&&v->identity.stx_ino?0:-1;
}
static long type(sl_linux_namespace_view *v,long expected){
 v->type=sl_linux_call6(&v->type_query,"controller.namespace.actual-type",SYS_ioctl,v->fd,NS_GET_NSTYPE,0,0,0,0);
 if(v->type<0)return v->type;return v->type==expected?0:-1;
}
static int same(const struct statx *a,const struct statx *b){
 return a->stx_ino==b->stx_ino&&a->stx_mnt_id==b->stx_mnt_id&&a->stx_dev_major==b->stx_dev_major&&a->stx_dev_minor==b->stx_dev_minor;
}
#define OBSERVE(expression) do{result=(expression);if(result<0)goto done;}while(0)
long sl_linux_observe_parent_namespace(sl_linux_parent_namespace *o){
 long result=-1;sl_linux_birth *birth;sl_linux_bootstrap *b;
 if(!o||o->attempted||!o->birth_original)return -1;o->attempted=1;birth=o->birth_original;b=birth->bootstrap;
 if(!b||!o->transition_original||!o->transition_original->returned||o->transition_original->result||
    o->transition_original->birth_original!=birth||o->transition_original->authority_original!=&b->authority||
    !atomic_load_explicit(&b->clone_parent.returned,memory_order_acquire)||b->clone_parent.result<=0||b->clone_parent.result>INT_MAX)goto done;
 for(unsigned i=0;i<6;i++){
  sl_linux_namespace_view *v=&o->views[i];
  if(v->acquire.source||v->acquire.number||v->lift.attempted||v->filesystem_query.source||v->identity_query.source||v->type_query.source||v->close_held.source||v->close_held.number||v->close_raw.source||v->close_raw.number||v->close_held_attempted||v->close_raw_attempted)goto done;
  v->fd=-1;if(init_call(&v->acquire)||init_call(&v->filesystem_query)||init_call(&v->identity_query)||init_call(&v->type_query)||init_call(&v->close_held)||init_call(&v->close_raw))goto done;
 }
 o->root_before.birth_original=birth;o->root_after.birth_original=birth;
 OBSERVE(sl_linux_observe_root_exit(&o->root_before));if(result||o->root_before.information.si_pid){result=-1;goto done;}
 OBSERVE(open_view(&o->views[0],AT_FDCWD,"/proc",O_PATH|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
 OBSERVE(filesystem(&o->views[0],PROC_SUPER_MAGIC));
 o->pidfd_info.birth_original=birth;o->pidfd_info.procfs_original=&o->views[0];
 OBSERVE(sl_linux_observe_pidfd_proc(&o->pidfd_info));
 /* This is the retained original pidfd's PID in the SAME held procfs realm,
  * not a guessed equality with clone3's caller-relative numeric result. */
 unsigned long pid=(unsigned long)o->pidfd_info.raw_pid;unsigned at=sizeof(o->pid_digits)-1;o->pid_digits[at]=0;
 do{o->pid_digits[--at]=(char)('0'+pid%10);pid/=10;}while(pid);o->pid_locator=o->pid_digits+at;
 OBSERVE(open_view(&o->views[1],o->views[0].fd,o->pid_locator,O_PATH|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
 OBSERVE(filesystem(&o->views[1],PROC_SUPER_MAGIC));
 OBSERVE(open_view(&o->views[2],o->views[1].fd,"ns",O_PATH|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW));
 OBSERVE(filesystem(&o->views[2],PROC_SUPER_MAGIC));
 /* Intentional kernel namespace magic-link resolution under the held actual
  * procfs task directory, not acceptance of arbitrary public symlinks. */
 OBSERVE(open_view(&o->views[3],o->views[2].fd,"cgroup",O_RDONLY|O_NONBLOCK|O_CLOEXEC));
 OBSERVE(filesystem(&o->views[3],NSFS_MAGIC));OBSERVE(type(&o->views[3],CLONE_NEWCGROUP));OBSERVE(identity(&o->views[3]));
 OBSERVE(hold(&o->views[4],sl_linux_call6(&o->views[4].acquire,"controller.namespace.get-owning-userns",SYS_ioctl,
                                      o->views[3].fd,NS_GET_USERNS,0,0,0,0)));
 OBSERVE(filesystem(&o->views[4],NSFS_MAGIC));OBSERVE(type(&o->views[4],CLONE_NEWUSER));OBSERVE(identity(&o->views[4]));
 OBSERVE(open_view(&o->views[5],o->views[0].fd,"thread-self/ns/user",O_RDONLY|O_NONBLOCK|O_CLOEXEC));
 OBSERVE(filesystem(&o->views[5],NSFS_MAGIC));OBSERVE(type(&o->views[5],CLONE_NEWUSER));OBSERVE(identity(&o->views[5]));
 if(!same(&o->views[3].identity,&b->authority.namespaces_after[1])||
    !same(&o->views[4].identity,&o->views[5].identity)||
    !same(&o->views[5].identity,&b->authority.namespaces_after[0])){result=-1;goto done;}
 OBSERVE(sl_linux_observe_root_exit(&o->root_after));if(result||o->root_after.information.si_pid){result=-1;goto done;}
 result=0;
done:o->result=result;o->returned=1;return result;
 /* No target-root/nsdelegate/mount visibility/actor/open-file/broker policy,
  * loaded identity, finite/native qualification or final gate is granted. */
}
#undef OBSERVE
#endif