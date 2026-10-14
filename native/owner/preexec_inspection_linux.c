#if defined(__linux__) && defined(__x86_64__)
#include "owner_linux.h"
#include "capability_domain_linux.h"
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <linux/securebits.h>
#include <linux/magic.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <limits.h>
static int returned(const sl_linux_call *c,long number,long result){
 return atomic_load_explicit(&c->returned,memory_order_acquire)&&
        atomic_load_explicit(&c->entered,memory_order_acquire)&&c->source&&c->number==number&&c->result==result;
}
static int credentials(const sl_linux_credential_snapshot *s){
 if(!s->attempted||!s->returned||s->result||!s->eof||!s->length||s->length>=s->status_capacity||
    !s->read_count||s->read_count>s->read_capacity||s->group_count>s->group_capacity||!s->status||!s->reads||!s->groups||
    !returned(&s->open,SYS_openat,s->fd)||!returned(&s->filesystem_query,SYS_fstatfs,0)||
    s->filesystem.f_type!=PROC_SUPER_MAGIC||!returned(&s->groups_query,SYS_getgroups,s->group_count)||
    !returned(&s->close,SYS_close,0)||s->close.arguments[0]!=s->fd)return 0;
 unsigned long length=0;
 for(unsigned long i=0;i<s->read_count;i++){
  const sl_linux_call *c=&s->reads[i];
  if(!atomic_load_explicit(&c->returned,memory_order_acquire)||!atomic_load_explicit(&c->entered,memory_order_acquire)||
     !c->source||c->number!=SYS_read||c->arguments[0]!=s->fd||c->arguments[1]!=(long)(s->status+length)||
     c->arguments[2]!=(long)(s->status_capacity-length)||c->result<0||(unsigned long)c->result>s->status_capacity-length)return 0;
  if(i+1==s->read_count){if(c->result)return 0;}else if(!c->result)return 0;
  length+=(unsigned long)c->result;
 }
 return length==s->length;
}
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){
 long result=-1;sl_linux_birth *birth;sl_linux_bootstrap *b;sl_linux_preexec_authority *a;
 if(!o||o->attempted||!o->birth_original)return -1;o->attempted=1;
 birth=o->birth_original;b=birth->bootstrap;if(!b||!birth->attempted)goto done;
 o->bootstrap_failed=atomic_load_explicit(&b->failed,memory_order_acquire);
 if(o->bootstrap_failed)goto done;
 a=&b->authority;o->authority_original=a;
 if(!atomic_load_explicit(&a->returned,memory_order_acquire)){o->not_ready=1;result=1;goto done;}
 if(!a->domain_original||!a->domain_original->entry_original)goto done;
 sl_linux_entry *entry=a->domain_original->entry_original;
 if(&entry->birth!=birth||!entry->fd_actions_attempted||!entry->fd_action_inputs||entry->fd_action_input_count!=b->fd_action_count||
    b->fd_action_count>ULONG_MAX/sizeof(sl_linux_fd_action)||entry->fd_action_mapping_bytes!=b->fd_action_count*sizeof(sl_linux_fd_action)||
    entry->fd_action_mapping_result!=(long)b->fd_actions||!returned(&entry->fd_action_mapping,SYS_mmap,(long)b->fd_actions)||
    entry->fd_action_mapping.arguments[0]||entry->fd_action_mapping.arguments[1]!=(long)entry->fd_action_mapping_bytes||entry->fd_action_mapping.arguments[2]!=(PROT_READ|PROT_WRITE)||entry->fd_action_mapping.arguments[3]!=(MAP_SHARED|MAP_ANONYMOUS)||entry->fd_action_mapping.arguments[4]!=-1||entry->fd_action_mapping.arguments[5])goto done;
 o->fd_inputs_original=entry->fd_action_inputs;
 /* Child action writes precede this SAME release publication. Never inspect
  * them while the child can still be producing those raw call originals. */
 if(!atomic_load_explicit(&b->fd_actions_prepared,memory_order_acquire)||!b->fd_actions||!b->fd_action_count||b->controlling_terminal_fd<3||
    !atomic_load_explicit(&b->create_session.entered,memory_order_acquire)||!atomic_load_explicit(&b->create_session.returned,memory_order_acquire)||!b->create_session.source||b->create_session.number!=SYS_setsid||b->create_session.result<=0||
    !returned(&b->acquire_terminal,SYS_ioctl,0)||b->acquire_terminal.arguments[0]!=b->controlling_terminal_fd||b->acquire_terminal.arguments[1]!=TIOCSCTTY||b->acquire_terminal.arguments[2])goto done;
 for(unsigned long i=0;i<b->fd_action_count;i++){
  sl_linux_fd_action *action=&b->fd_actions[i];sl_linux_call *call=&action->call;o->fd_failure_original=action;
  const sl_linux_fd_action *input=&o->fd_inputs_original[i];
  if(action->kind!=input->kind||action->source_fd!=input->source_fd||action->target_fd!=input->target_fd||action->outer_original!=input->outer_original)goto done;
  if(!action->outer_original||action->source_fd<0||call->arguments[0]!=action->source_fd)goto done;
  if(action->kind==SL_FD_CLOSE_VIEW){if(!returned(call,SYS_close,0))goto done;}
  else if(action->kind==SL_FD_DUPLICATE_VIEW){if(action->target_fd<0||action->target_fd==action->source_fd||!returned(call,SYS_dup3,action->target_fd)||call->arguments[1]!=action->target_fd||call->arguments[2])goto done;}
  else if(action->kind==SL_FD_CLEAR_EXEC){if(!returned(call,SYS_fcntl,0)||call->arguments[1]!=F_SETFD||call->arguments[2])goto done;}
  else goto done;
  o->checked_fd_actions=i+1;o->fd_failure_original=NULL;
 }
 if(!a->prepared||!a->attempted||a->result||!a->domain_original||a->last_cap>=64||!a->domain_original->entry_original||
    a->domain_original->entry_original->birth.bootstrap!=b||a->domain_original->last_cap!=a->last_cap||
    !returned(&b->clone_child,SYS_clone3,0)||!credentials(&a->credentials_before)||!credentials(&a->credentials_after))goto done;
 for(unsigned i=0;i<4;i++)if(!a->credentials_before.uids[i]||
    a->credentials_before.uids[i]!=a->credentials_after.uids[i]||a->credentials_before.gids[i]!=a->credentials_after.gids[i])goto done;
 if(a->credentials_before.group_count!=a->credentials_after.group_count)goto done;
 for(unsigned i=0;i<a->credentials_before.group_count;i++)if(a->credentials_before.groups[i]!=a->credentials_after.groups[i])goto done;
 for(unsigned i=0;i<2;i++){
  if(!returned(&a->namespace_before[i],SYS_statx,0)||!returned(&a->namespace_after[i],SYS_statx,0))goto done;
  const struct statx *before=&a->namespaces_before[i],*after=&a->namespaces_after[i];
  unsigned mask=STATX_INO|STATX_TYPE|STATX_MNT_ID;
  if((before->stx_mask&mask)!=mask||(after->stx_mask&mask)!=mask||!before->stx_ino||!after->stx_ino||
     !S_ISREG(before->stx_mode)||!S_ISREG(after->stx_mode))goto done;
  if(!i&&(before->stx_ino!=after->stx_ino||before->stx_mnt_id!=after->stx_mnt_id||
          before->stx_dev_major!=after->stx_dev_major||before->stx_dev_minor!=after->stx_dev_minor))goto done;
  if(i&&before->stx_ino==after->stx_ino&&before->stx_dev_major==after->stx_dev_major&&before->stx_dev_minor==after->stx_dev_minor)goto done;
 }
 if(!returned(&a->unshare,SYS_unshare,0)||a->unshare.arguments[0]!=CLONE_NEWCGROUP)goto done;
 long bits=SECBIT_KEEP_CAPS_LOCKED|SECBIT_NO_SETUID_FIXUP|SECBIT_NO_SETUID_FIXUP_LOCKED|SECBIT_NOROOT|SECBIT_NOROOT_LOCKED|SECBIT_NO_CAP_AMBIENT_RAISE|SECBIT_NO_CAP_AMBIENT_RAISE_LOCKED;
 if(!returned(&a->securebits_set,SYS_prctl,0)||a->securebits_set.arguments[0]!=PR_SET_SECUREBITS||a->securebits_set.arguments[1]!=bits||
    !returned(&a->securebits_get,SYS_prctl,bits)||!returned(&a->ambient_clear,SYS_prctl,0)||
    !returned(&a->cap_clear,SYS_capset,0)||!returned(&a->cap_after,SYS_capget,0)||
    !returned(&a->nnp_set,SYS_prctl,0)||!returned(&a->nnp_get,SYS_prctl,1))goto done;
 for(unsigned i=0;i<=a->last_cap;i++)if(!returned(&a->bounding_drop[i],SYS_prctl,0)||
    !returned(&a->bounding_read[i],SYS_prctl,0)||!returned(&a->ambient_read[i],SYS_prctl,0))goto done;
 for(unsigned i=0;i<2;i++)if(a->caps_after[i].effective||a->caps_after[i].permitted||a->caps_after[i].inheritable)goto done;
 result=0;
done:o->result=result;o->returned=1;return result;
 /* Success is a narrow source transition observation, not owning-userns,
  * target root, current mount visibility, actor/OFD/broker exclusion, loaded
  * image, finite fit, custody/EOF/FINAL or permission to release any gate. */
}
#endif
