/* UNRUN substituted raw operations. Not native rights/policy/identity proof. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
static sl_linux_public_startup *expected;
static unsigned calls,status_calls;static long open_failure;static int changed_identity,status_failure;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 calls++;c->source=source;c->number=number;long args[6]={a0,a1,a2,a3,a4,a5};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);
 if(number==SYS_openat2)c->result=open_failure?open_failure:(c==&expected->candidate_target.open_target?40:41);
 else if(number==SYS_fstatfs){((struct statfs*)a1)->f_type=CGROUP2_SUPER_MAGIC;c->result=0;}
 else if(number==SYS_statx){struct statx *out=(void*)a4;*out=c==&expected->candidate_target.target_identity?expected->actual_children[0].actual_views[0].identity:expected->actual_controls[0].identity;if(changed_identity&&c==&expected->candidate_target.target_identity)out->stx_ino++;c->result=0;}
 else assert(0);atomic_store(&c->returned,1);return c->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){l->attempted=1;l->original_fd=l->held_fd=fd;return fd;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){status_calls++;s->attempted=s->returned=1;s->actual_status=s->status_original;s->result=status_failure;return status_failure;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../candidate_target_linux.c"
typedef struct {sl_linux_public_startup s;sl_linux_public_controller controller;sl_candidate_profile profile;sl_candidate_record evidence,record;unsigned char link[24],tuple[32],body[1];sl_linux_candidate_policy policy;sl_linux_policy_parse parse;sl_linux_selinux_snapshot snapshot;sl_linux_selinux_status_sample status;sl_linux_cgroup_children children;sl_linux_child_view views[2];sl_linux_cgroup_control_label controls[2];sl_linux_selinux_context contexts[2];sl_linux_control_access access[4];sl_linux_candidate_routes routes[4];} fixture;
static void put(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(i*8));}
static void prepare(fixture *f){
 sl_linux_public_startup *s=&f->s;expected=s;s->returned=s->candidate_routes_returned=s->candidate_file_entry_returned=1;s->stage=SL_STARTUP_ACQUIRED;s->controller_original=&f->controller;f->controller.entry.birth.cgroup_fd=f->controller.entry.birth.payload_fd=-1;
 s->immutable.returned=s->immutable.seals_observed=s->immutable.readback_complete=1;s->immutable.fd=50;
 s->candidate_target.startup_original=s;s->candidate_target.statuses=&f->status;s->candidate_target.status_capacity=1;s->actual_candidate_policies=&f->policy;s->actual_candidate_policy_capacity=s->mounts.selinux_count=1;
 f->policy.returned=1;f->policy.actual_candidate=&f->profile;f->policy.actual_parse=&f->parse;f->parse.actual_snapshot=&f->snapshot;
 f->profile.returned=1;f->profile.actual_rows[0]=&f->evidence;f->profile.actual_rows[3]=&f->record;f->profile.counts[0]=f->profile.counts[3]=1;f->evidence.field_count=7;f->evidence.fields[6]=(sl_candidate_span){f->body,1};f->record.field_count=3;
 f->record.fields[0]=(sl_candidate_span){f->link,24};put(f->link+16,1);f->record.fields[1]=(sl_candidate_span){(const unsigned char*)"service-lasso.cgroup-target-object.v1",sizeof("service-lasso.cgroup-target-object.v1")-1};f->record.fields[2]=(sl_candidate_span){f->tuple,32};put(f->tuple,0);put(f->tuple+8,28);put(f->tuple+16,9);put(f->tuple+24,7);
 s->actual_children=&f->children;s->actual_children_capacity=1;f->children.attempted=f->children.returned=f->children.eof=1;f->children.actual_views=f->views;f->children.actual_view_capacity=2;f->children.view_count=1;
 f->views[0].attempted=f->views[0].returned=1;f->views[0].fd=30;f->views[0].filesystem.f_type=CGROUP2_SUPER_MAGIC;f->views[0].identity=(struct statx){.stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID,.stx_mode=S_IFDIR|0700,.stx_ino=9,.stx_mnt_id=7,.stx_dev_minor=28};
 s->actual_controls=f->controls;s->actual_control_capacity=2;s->actual_control_contexts=f->contexts;s->actual_control_access=f->access;s->actual_control_access_capacity=4;s->actual_candidate_routes=f->routes;s->actual_candidate_route_capacity=4;
 for(unsigned i=0;i<2;i++){sl_linux_cgroup_control_label *c=&f->controls[i];c->attempted=c->returned=1;c->view_original=c->actual_view=&f->views[0];c->children_original=c->actual_children=&f->children;c->kind=i+1;c->identity=f->views[0].identity;c->identity.stx_ino=10+i;c->identity.stx_mode=S_IFREG|0600;f->contexts[i].control_original=c;f->contexts[i].parse_original=&f->parse;
  for(unsigned j=0;j<2;j++){unsigned at=2*i+j;f->access[at].attempted=f->access[at].returned=1;f->access[at].actual_context=f->access[at].context_original=&f->contexts[i];f->access[at].actual_kind=j;f->routes[at].actual_control=&f->access[at];f->routes[at].returned=1;}}
}
int main(void){fixture cases[8]={0};for(unsigned i=0;i<8;i++){
 fixture *f=&cases[i];prepare(f);calls=status_calls=0;open_failure=i==6?-EACCES:0;status_failure=i==6?-77:0;changed_identity=i==4;
 if(i==1)put(f->tuple+16,99);
 if(i==2){f->views[1]=f->views[0];f->children.view_count=2;}
 if(i==3)f->views[0].returned=0;
 if(i==5)f->access[3].actual_kind=SL_CONTROL_WRITE_APPEND;
 if(i==7)f->record.fields[2].extent=31;
 long r=sl_linux_bind_candidate_target(&f->s.candidate_target);assert(f->s.candidate_target.returned&&status_calls==1);
 if(!i){assert(!r&&calls==6&&f->s.candidate_target.bound&&f->controller.entry.birth.cgroup_fd==40&&f->controller.entry.birth.payload_fd==50&&f->s.candidate_target.procs_fd==41&&f->s.candidate_target.control_episodes==4&&f->s.candidate_target.statement_original.bytes==f->body);}
 else {assert(r<0&&!f->s.candidate_target.bound&&f->controller.entry.birth.cgroup_fd==-1);if(i==6)assert(r==-EACCES&&f->s.candidate_target.status_result==-77&&calls==1);else if(i==4)assert(calls==3&&f->s.candidate_target.target_fd==40);else assert(!calls);}
 unsigned old=calls;assert(sl_linux_bind_candidate_target(&f->s.candidate_target)==-EINVAL&&calls==old);
 }return 0;}