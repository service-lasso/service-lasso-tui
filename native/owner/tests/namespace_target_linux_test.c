/* UNRUN actual conditional namespace-target consumer; no kernel/profile proof. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <sys/syscall.h>
#include <linux/magic.h>
#include <string.h>
#include "../namespace_target_linux.c"
typedef struct {sl_linux_public_controller c;sl_linux_public_startup s;sl_linux_bootstrap b;sl_linux_cgroup_children children;sl_linux_child_view view;sl_linux_mount_location location;sl_linux_mount_row row;sl_linux_service_observation service;unsigned char options[40];} fixture;
static void returned(sl_linux_call *c,long n,long r){atomic_init(&c->entered,1);atomic_init(&c->returned,1);c->number=n;c->result=r;}
int main(void){fixture originals[7]={0};for(unsigned mode=0;mode<7;mode++){
 fixture *f=&originals[mode];sl_linux_public_controller *c=&f->c;sl_linux_candidate_target *t=&f->s.candidate_target;sl_linux_bootstrap *b=&f->b;
 c->startup_original=&f->s;f->s.controller_original=c;c->entry.birth.bootstrap=b;c->entry.birth.cgroup_fd=9;c->birth_result=123;c->authority_observed_by=&f->service;
 t->returned=t->bound=1;t->actual_startup=&f->s;t->children_original=&f->children;t->view_original=&f->view;t->target_fd=9;t->target_fs.f_type=CGROUP2_SUPER_MAGIC;
 t->target_state.stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;t->target_state.stx_ino=7;t->target_state.stx_mnt_id=8;f->view.identity=t->target_state;
 sl_linux_parent_namespace *n=&c->namespace_observation;n->returned=1;n->birth_original=&c->entry.birth;n->transition_original=&f->service.authority;f->service.authority.returned=1;f->service.authority.authority_original=&b->authority;
 returned(&b->clone_parent,SYS_clone3,123);returned(&b->clone_child,SYS_clone3,0);b->clone_parent.arguments[0]=b->clone_child.arguments[0]=(long)&b->arguments;b->clone_parent.arguments[1]=b->clone_child.arguments[1]=sizeof b->arguments;b->arguments.flags=CLONE_INTO_CGROUP|CLONE_PIDFD;b->arguments.cgroup=9;b->arguments.pidfd=(uint64_t)(uintptr_t)&b->pidfd;b->arguments.exit_signal=SIGCHLD;b->pidfd=10;
 atomic_init(&b->authority.returned,1);returned(&b->authority.unshare,SYS_unshare,0);b->authority.unshare.arguments[0]=CLONE_NEWCGROUP;b->authority.namespaces_after[1]=t->target_state;n->views[3].identity=t->target_state;
 f->children.actual_location=&f->location;f->location.returned=1;f->location.actual_mounts=&f->s.mounts;f->location.actual_row=&f->row;f->location.mount_state.stx_mnt_id=8;f->row.mount_id=8;
 memcpy(f->options,"rw,nsdelegate,memory_recursiveprot",33);f->row.super_options.length=33;f->s.mounts.length=33;f->s.mounts.returned=f->s.mounts.eof=1;f->s.mounts.actual_rows=f->s.mounts.rows=&f->row;f->s.mounts.row_count=1;f->s.mounts.actual_bytes=f->options;
 if(mode==1)b->arguments.cgroup=11;if(mode==2)b->clone_child.result=-EIO;if(mode==3)f->row.mount_id=88;if(mode==4)f->row.super_options.offset=34;if(mode==5)f->options[3]='x';if(mode==6)n->views[3].identity.stx_ino=99;
 long r=sl_linux_interpret_namespace_target(c);assert(c->namespace_target.returned);if(!mode)assert(!r&&c->namespace_target.nsdelegate_observed==1&&c->namespace_target.options_original==f->options);else assert(r==-EINVAL);
 long saved=c->namespace_target.result;assert(sl_linux_interpret_namespace_target(c)==-EINVAL&&c->namespace_target.result==saved);assert(b->clone_child.result==(mode==2?-EIO:0));
 }return 0;}