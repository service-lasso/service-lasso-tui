/* UNRUN source-controlled parent publication/parser correspondence. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_preexec_linux.h"
#include <sys/syscall.h>
#include <errno.h>
#include <assert.h>
static unsigned calls;static int parser_result,status_equal=1;static unsigned observed_type=3;
int sl_linux_interpret_selinux_self(sl_linux_selinux_context *o,sl_linux_lsm_self *s){calls++;o->self_original=o->actual_self=s;o->user=1;o->role=2;o->type=observed_type;return parser_result;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return status_equal;}
#include "../candidate_preexec_inspection_linux.c"
typedef struct{sl_linux_candidate_preexec owner;sl_linux_candidate_preexec_shared shared;sl_linux_candidate_payload_context payload;sl_linux_selinux_context context;sl_linux_policy_parse parse;sl_linux_bootstrap bootstrap;sl_linux_birth birth;sl_linux_candidate_preexec_inspection observations[3];unsigned char input[40],exec_bytes[64];} fixture;
static void raw(sl_linux_call *c,long n,long value){atomic_init(&c->entered,1);atomic_init(&c->returned,1);c->source="fixture";c->number=n;c->result=value;}
static void prepare(fixture *f){
 f->owner.returned=1;f->owner.shared=&f->shared;f->owner.actual_bootstrap=&f->bootstrap;f->owner.actual_payload=&f->payload;f->owner.actual_owner=&f->context;
 f->owner.child_context.parse_original=&f->parse;f->payload.context.actual_parse=&f->parse;f->payload.context.user=1;f->payload.context.role=2;f->payload.context.type=3;
 f->birth.bootstrap=&f->bootstrap;f->birth.attempted=1;f->bootstrap.candidate_preexec=&f->shared;raw(&f->bootstrap.clone_child,SYS_clone3,0);
 f->shared.bootstrap_original=&f->bootstrap;f->shared.owner_original=&f->context;f->shared.payload_original=&f->payload;f->shared.attempted=1;atomic_init(&f->shared.returned,0);
 f->shared.before.returned=f->shared.after.returned=1;f->shared.before.framing_complete=f->shared.after.framing_complete=1;f->shared.before.raw_query=f->shared.after.raw_query=1;
 f->shared.before.raw_before=f->shared.before.raw_after=f->shared.after.raw_before=f->shared.after.raw_after=17;
 f->shared.pending_exec.bytes=f->shared.pending_exec.actual_bytes=f->exec_bytes;f->shared.pending_exec.capacity=f->shared.pending_exec.actual_capacity=f->shared.pending_exec.supplied_size=64;
 f->shared.pending_exec.returned=f->shared.pending_exec.framing_complete=1;f->shared.pending_exec.raw_query=1;f->shared.pending_exec.raw_before=f->shared.pending_exec.raw_after=17;f->shared.pending_exec.requested_attr=f->shared.pending_exec.requested_id=101;f->shared.pending_exec.requested_flags=1;f->shared.pending_exec.observed=(sl_linux_lsm_header){101,0,32,0};f->shared.pending_exec.reported_size=32;raw(&f->shared.pending_exec.query,459,1);f->shared.pending_exec.query.arguments[0]=101;f->shared.pending_exec.query.arguments[1]=(long)f->shared.pending_exec.actual_bytes;f->shared.pending_exec.query.arguments[2]=(long)&f->shared.pending_exec.reported_size;f->shared.pending_exec.query.arguments[3]=1;
 f->shared.input=f->input;f->shared.input_extent=40;raw(&f->shared.mutation,460,0);f->shared.mutation.arguments[0]=100;f->shared.mutation.arguments[1]=(long)f->input;f->shared.mutation.arguments[2]=40;
 for(unsigned i=0;i<3;i++){f->observations[i].owner_original=&f->owner;f->observations[i].birth_original=&f->birth;}
}
int main(void){fixture cases[7]={0};for(unsigned i=0;i<7;i++){
 fixture *f=&cases[i];prepare(f);unsigned before=calls;
 assert(sl_linux_inspect_candidate_preexec(&f->observations[0])==1&&f->observations[0].not_ready&&!f->owner.inspected_by&&calls==before);
 atomic_store_explicit(&f->shared.returned,1,memory_order_release);parser_result=i==1?-71:0;observed_type=i==2?4:3;status_equal=i!=3;if(i==4)f->shared.mutation.arguments[1]++;if(i==5)f->shared.pending_exec.observed.ctx_len=2;if(i==6)f->shared.pending_exec.query.arguments[0]=100;
 long result=sl_linux_inspect_candidate_preexec(&f->observations[1]);assert(result==(i==1?-71:i?-EINVAL:0));assert(f->owner.inspected_by==&f->observations[1]);assert(calls==before+(i>=4?0:1));
 before=calls;assert(sl_linux_inspect_candidate_preexec(&f->observations[2])==result&&calls==before&&f->observations[2].prior_original==&f->observations[1]);
 assert(sl_linux_inspect_candidate_preexec(&f->observations[1])==-EINVAL&&calls==before);
 }return 0;}