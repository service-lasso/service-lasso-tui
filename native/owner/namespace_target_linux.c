#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <sys/syscall.h>
#include <errno.h>
#include <linux/magic.h>
static int nt_returned(sl_linux_call *c,long number,long result){return atomic_load_explicit(&c->returned,memory_order_acquire)&&c->number==number&&c->result==result;}
static int nt_same(const struct statx *a,const struct statx *b){unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;return (a->stx_mask&mask)==mask&&(b->stx_mask&mask)==mask&&a->stx_ino&&a->stx_ino==b->stx_ino&&a->stx_mnt_id==b->stx_mnt_id&&a->stx_dev_major==b->stx_dev_major&&a->stx_dev_minor==b->stx_dev_minor;}
long sl_linux_interpret_namespace_target(sl_linux_public_controller *c){
 if(!c||c->namespace_target.attempted)return -EINVAL;
 sl_linux_namespace_target *o=&c->namespace_target;o->attempted=1;o->controller_original=c;o->result=-EINVAL;
 sl_linux_public_startup *s=c->startup_original;sl_linux_birth *birth=&c->entry.birth;sl_linux_bootstrap *b=birth->bootstrap;
 if(!s||s->controller_original!=c||!b||!c->authority_observed_by)goto done;
 sl_linux_candidate_target *t=&s->candidate_target;o->target_original=t;o->namespace_original=&c->namespace_observation;
 sl_linux_parent_namespace *n=o->namespace_original;
 if(!t->returned||t->result||!t->bound||t->actual_startup!=s||!t->children_original||!t->view_original||birth->cgroup_fd!=t->target_fd||t->target_fd<3||t->target_fs.f_type!=CGROUP2_SUPER_MAGIC||!nt_same(&t->target_state,&t->view_original->identity))goto done;
 if(!n->returned||n->result||n->birth_original!=birth||n->transition_original!=&c->authority_observed_by->authority||!n->transition_original->returned||n->transition_original->result||n->transition_original->authority_original!=&b->authority)goto done;
 if(!nt_returned(&b->clone_parent,SYS_clone3,c->birth_result)||c->birth_result<=0||!nt_returned(&b->clone_child,SYS_clone3,0)||b->clone_parent.arguments[0]!=(long)&b->arguments||b->clone_child.arguments[0]!=(long)&b->arguments||b->clone_parent.arguments[1]!=sizeof b->arguments||b->clone_child.arguments[1]!=sizeof b->arguments||b->arguments.flags!=(CLONE_INTO_CGROUP|CLONE_PIDFD)||b->arguments.cgroup!=(uint64_t)t->target_fd||b->arguments.pidfd!=(uint64_t)(uintptr_t)&b->pidfd||b->arguments.exit_signal!=SIGCHLD||b->pidfd<0)goto done;
 if(!atomic_load_explicit(&b->authority.returned,memory_order_acquire)||b->authority.result||!nt_returned(&b->authority.unshare,SYS_unshare,0)||b->authority.unshare.arguments[0]!=CLONE_NEWCGROUP||!nt_same(&n->views[3].identity,&b->authority.namespaces_after[1]))goto done;
 sl_linux_mount_location *l=t->children_original->actual_location;o->location_original=l;
 if(!l||!l->returned||l->result||l->outside_mount||!l->actual_mounts||!l->actual_row||l->actual_mounts!=&s->mounts||!s->mounts.returned||s->mounts.result||!s->mounts.eof||s->mounts.actual_rows!=s->mounts.rows||l->row_index>=s->mounts.row_count||l->actual_row!=&s->mounts.actual_rows[l->row_index])goto done;
 sl_linux_mount_row *row=l->actual_row;o->row_original=row;
 if(row->mount_id!=t->target_state.stx_mnt_id||l->mount_state.stx_mnt_id!=row->mount_id||row->super_options.offset>s->mounts.length||row->super_options.length>s->mounts.length-row->super_options.offset||!s->mounts.actual_bytes)goto done;
 o->options_original=s->mounts.actual_bytes+row->super_options.offset;o->options_extent=row->super_options.length;
 for(unsigned long at=0;at<o->options_extent;){unsigned long end=at;while(end<o->options_extent&&o->options_original[end]!=',')end++;o->checked_options=end;static const unsigned char token[]="nsdelegate";if(end-at==sizeof token-1){unsigned i=0;while(i<sizeof token-1&&o->options_original[at+i]==token[i])i++;if(i==sizeof token-1)o->nsdelegate_observed++;}if(end==o->options_extent)break;at=end+1;}
 if(o->nsdelegate_observed!=1)goto done;o->result=0;
 done:o->returned=1;return o->result;
 /* Conditional selected kernel css_set_fork/post_fork -> unshare root_cset
  * correspondence only. No actor/cohort/interference/loaded-code admission. */
}
#endif