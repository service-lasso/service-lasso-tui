#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
static uint64_t target_u64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static int target_meaning(sl_candidate_span s){static const char text[]="service-lasso.cgroup-target-object.v1";if(!s.bytes||s.extent!=sizeof(text)-1)return 0;for(size_t i=0;i<s.extent;i++)if(s.bytes[i]!=(unsigned char)text[i])return 0;return 1;}
static int target_same(const struct statx *a,const struct statx *b){return a->stx_ino==b->stx_ino&&a->stx_mnt_id==b->stx_mnt_id&&a->stx_dev_major==b->stx_dev_major&&a->stx_dev_minor==b->stx_dev_minor;}
static int target_tuple(const sl_linux_candidate_target *o,const struct statx *s){return o->tuple[0]==s->stx_dev_major&&o->tuple[1]==s->stx_dev_minor&&o->tuple[2]==s->stx_ino&&o->tuple[3]==s->stx_mnt_id;}
static void target_init(sl_linux_call *c){atomic_init(&c->entered,0);atomic_init(&c->returned,0);}
static long target_identify(sl_linux_call *fs,sl_linux_call *identity,int fd,struct statfs *filesystem,struct statx *state,int directory){
 unsigned mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;target_init(fs);target_init(identity);
 long r=sl_linux_call6(fs,"candidate.target.held-filesystem",SYS_fstatfs,fd,(long)filesystem,0,0,0,0);if(r)return r;
 if(filesystem->f_type!=CGROUP2_SUPER_MAGIC)return -EINVAL;
 r=sl_linux_call6(identity,"candidate.target.held-identity",SYS_statx,fd,(long)"",AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW,mask,(long)state,0);if(r)return r;
 if((state->stx_mask&mask)!=mask||(directory?!S_ISDIR(state->stx_mode):!S_ISREG(state->stx_mode)))return -EINVAL;return 0;
}
long sl_linux_bind_candidate_target(sl_linux_candidate_target *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->primary_result=o->result=-EINVAL;o->raw_target_fd=o->target_fd=o->raw_procs_fd=o->procs_fd=-1;
 o->actual_startup=o->startup_original;o->actual_statuses=o->statuses;o->actual_status_capacity=o->status_capacity;
 sl_linux_public_startup *s=o->actual_startup;
 if(!s||o!=&s->candidate_target||!s->immutable.returned||s->immutable.result||!s->immutable.seals_observed||!s->immutable.readback_complete||s->immutable.fd<3||!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->candidate_routes_returned||s->candidate_routes_result||!s->candidate_file_entry_returned||s->candidate_file_entry_result||!s->controller_original||s->controller_original->entry.birth.attempted||s->controller_original->entry.birth.cgroup_fd!=-1||s->controller_original->entry.birth.payload_fd!=-1||!s->actual_candidate_policies||!s->actual_candidate_policy_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_candidate_policy_capacity||!o->actual_statuses||o->actual_status_capacity!=s->actual_candidate_policy_capacity||!s->actual_children||!s->actual_controls||!s->actual_control_contexts||!s->actual_control_access||!s->actual_candidate_routes)goto done;
 for(unsigned long p=0;p<o->actual_status_capacity;p++)if(o->actual_statuses[p].attempted||o->actual_statuses[p].status_original)goto done;
 o->candidate_original=s->actual_candidate_policies[0].actual_candidate;
 sl_candidate_profile *candidate=o->candidate_original;
 if(!candidate||!candidate->returned||candidate->result||!candidate->actual_rows[0]||!candidate->actual_rows[3])goto done;
 for(unsigned long p=0;p<s->mounts.selinux_count;p++)if(!s->actual_candidate_policies[p].returned||s->actual_candidate_policies[p].result||s->actual_candidate_policies[p].actual_candidate!=candidate)goto done;
 for(size_t i=0;i<candidate->counts[3];i++){
  const sl_candidate_record *d=&candidate->actual_rows[3][i];if(d->field_count!=3||!target_meaning(d->fields[1]))continue;
  sl_candidate_span link=d->fields[0],tuple=d->fields[2];if(!link.bytes||link.extent!=24||!tuple.bytes||tuple.extent!=32)goto sampled;
  uint64_t index=target_u64(link.bytes),offset=target_u64(link.bytes+8),extent=target_u64(link.bytes+16);if(index>=candidate->counts[0]||!extent)goto sampled;
  const sl_candidate_record *e=&candidate->actual_rows[0][index];if(e->field_count!=7||!e->fields[6].bytes||offset>e->fields[6].extent||extent>e->fields[6].extent-offset)goto sampled;
  uint64_t values[4];for(unsigned k=0;k<4;k++)values[k]=target_u64(tuple.bytes+k*8);
  if(values[0]>UINT32_MAX||values[1]>UINT32_MAX||!values[2]||!values[3])goto sampled;
  if(o->declarations){for(unsigned k=0;k<4;k++)if(values[k]!=o->tuple[k])goto sampled;}else{for(unsigned k=0;k<4;k++)o->tuple[k]=values[k];o->first_declaration=d;}
  o->declarations++;o->declaration_original=d;o->evidence_original=e;o->evidence_index=index;o->statement_offset=offset;o->statement_extent=extent;o->statement_original=(sl_candidate_span){e->fields[6].bytes+(size_t)offset,(size_t)extent};
 }
 if(!o->declarations)goto sampled;
 unsigned long matches=0;
 for(unsigned long i=0;i<s->actual_children_capacity;i++){
  sl_linux_cgroup_children *c=&s->actual_children[i];o->children_examined=i+1;if(!c->attempted)continue;
  if(!c->returned||c->result||!c->eof||!c->actual_views||c->view_count>c->actual_view_capacity)goto sampled;
  for(unsigned long j=0;j<c->actual_view_capacity;j++){
   sl_linux_child_view *v=&c->actual_views[j];o->views_examined++;if(!v->attempted){if(j<c->view_count&&!v->dot)goto sampled;continue;}
   if(!v->returned)goto sampled;if(v->not_directory){if(v->result!=-ENOTDIR)goto sampled;continue;}
   if(v->result||v->fd<3||v->filesystem.f_type!=CGROUP2_SUPER_MAGIC||!S_ISDIR(v->identity.stx_mode))goto sampled;
   if(target_tuple(o,&v->identity)){if(++matches!=1)goto sampled;o->children_original=c;o->view_original=v;}
  }
 }
 if(matches!=1)goto sampled;
 for(unsigned kind=SL_CGROUP_PROCS;kind<=SL_CGROUP_THREADS;kind++){
  sl_linux_cgroup_control_label *control=0;unsigned long found=0;
  for(unsigned long i=0;i<s->actual_control_capacity;i++){
   sl_linux_cgroup_control_label *c=&s->actual_controls[i];if(c->view_original!=o->view_original||c->children_original!=o->children_original||c->kind!=kind)continue;
   if(++found!=1||!c->attempted||!c->returned||c->result||c->actual_view!=o->view_original||c->actual_children!=o->children_original)goto sampled;control=c;
  }
  if(found!=1)goto sampled;if(kind==SL_CGROUP_PROCS)o->procs_original=control;
  for(unsigned long p=0;p<s->mounts.selinux_count;p++){
   unsigned long count=0,hits=0;
   for(unsigned long i=0;i<s->actual_control_access_capacity;i++){
    sl_linux_control_access *a=&s->actual_control_access[i];if(!a->context_original||a->context_original->control_original!=control||a->context_original->parse_original!=s->actual_candidate_policies[p].actual_parse)continue;
    if(!a->attempted||!a->returned||a->result||a->status_result||a->actual_context!=a->context_original||i>=s->actual_candidate_route_capacity||s->actual_candidate_routes[i].actual_control!=a||!s->actual_candidate_routes[i].returned||s->actual_candidate_routes[i].result)goto sampled;
    if(a->actual_kind!=SL_CONTROL_WRITE_APPEND&&a->actual_kind!=SL_CONTROL_RELABEL_FROM)goto sampled;if(count&(1UL<<a->actual_kind))goto sampled;count|=1UL<<a->actual_kind;hits++;o->control_episodes++;
   }
   if(count!=3||hits!=2)goto sampled;
  }
 }
 o->target_how=(struct open_how){.flags=O_RDONLY|O_DIRECTORY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};target_init(&o->open_target);
 long r=sl_linux_call6(&o->open_target,"candidate.target.independent-directory",SYS_openat2,o->view_original->fd,(long)".",(long)&o->target_how,sizeof o->target_how,0,0);o->primary_result=r;if(r<0||r>INT_MAX){if(r>INT_MAX)o->primary_result=-EINVAL;goto sampled;}o->raw_target_fd=(int)r;
 r=sl_linux_lift_internal_fd(&o->target_lift,(int)r);o->primary_result=r;if(r<0)goto sampled;o->target_fd=(int)r;
 r=target_identify(&o->target_filesystem,&o->target_identity,o->target_fd,&o->target_fs,&o->target_state,1);o->primary_result=r;if(r||!target_same(&o->target_state,&o->view_original->identity)){if(!r)o->primary_result=-EINVAL;goto sampled;}
 o->procs_how=(struct open_how){.flags=O_WRONLY|O_NONBLOCK|O_CLOEXEC,.resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV};target_init(&o->open_procs);
 r=sl_linux_call6(&o->open_procs,"candidate.target.independent-procs-write",SYS_openat2,o->target_fd,(long)"cgroup.procs",(long)&o->procs_how,sizeof o->procs_how,0,0);o->primary_result=r;if(r<0||r>INT_MAX){if(r>INT_MAX)o->primary_result=-EINVAL;goto sampled;}o->raw_procs_fd=(int)r;
 r=sl_linux_lift_internal_fd(&o->procs_lift,(int)r);o->primary_result=r;if(r<0)goto sampled;o->procs_fd=(int)r;
 r=target_identify(&o->procs_filesystem,&o->procs_identity,o->procs_fd,&o->procs_fs,&o->procs_state,0);o->primary_result=r;if(r||!target_same(&o->procs_state,&o->procs_original->identity)){if(!r)o->primary_result=-EINVAL;goto sampled;}
 o->primary_result=0;
 sampled:for(unsigned long p=0;p<s->mounts.selinux_count;p++){
  sl_linux_candidate_policy *policy=&s->actual_candidate_policies[p];sl_linux_selinux_status_sample *sample=&o->actual_statuses[p];o->status_count=p+1;
  if(!policy->actual_parse||!policy->actual_parse->actual_snapshot){if(!o->status_result)o->status_result=-EINVAL;continue;}
  sample->status_original=&policy->actual_parse->actual_snapshot->status;long r=sl_linux_sample_selinux_status(sample);
  if(!r&&!sl_linux_equal_selinux_status(&policy->after_binding,sample))r=-EINVAL;if(r&&!o->status_result){o->status_result=r;o->status_failure_original=sample;}
 }
 o->result=o->primary_result?o->primary_result:o->status_result;
 if(!o->result){s->controller_original->entry.birth.cgroup_fd=o->target_fd;s->controller_original->entry.birth.payload_fd=s->immutable.fd;o->bound=1;}
 done:o->returned=1;return o->result;
}
#endif