#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "candidate_preexec_linux.h"
#include <sys/syscall.h>
#include <errno.h>
static int observed(const sl_linux_call *c,long number,long result){
 return atomic_load_explicit(&c->returned,memory_order_acquire)&&atomic_load_explicit(&c->entered,memory_order_acquire)&&c->source&&c->number==number&&c->result==result;
}
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;
 sl_linux_candidate_preexec *p=o->owner_original;sl_linux_birth *birth=o->birth_original;
 if(!p||!birth||!birth->attempted||!p->returned||p->result||!p->shared||p->actual_bootstrap!=birth->bootstrap)goto done;
 sl_linux_candidate_preexec_shared *s=p->shared;o->shared_original=s;
 if(s->bootstrap_original!=birth->bootstrap||birth->bootstrap->candidate_preexec!=s)goto done;
 /* Acquire the actual child publication before reading any child-written cell.
  * Every not-ready observation is distinct; it never claims parser backing. */
 if(!atomic_load_explicit(&s->returned,memory_order_acquire)){o->not_ready=1;o->result=1;goto done;}
 if(p->inspected_by){o->prior_original=p->inspected_by;if(!o->prior_original->returned)goto done;o->context_result=o->prior_original->context_result;o->result=o->prior_original->result;goto done;}
 p->inspected_by=o;
 if(!observed(&birth->bootstrap->clone_child,SYS_clone3,0)||!s->attempted||s->result||s->before_result||s->mutation_result||s->after_result||s->owner_original!=p->actual_owner||s->payload_original!=p->actual_payload||
    !s->before.returned||s->before.result||!s->after.returned||s->after.result||!s->before.framing_complete||!s->after.framing_complete||s->before.raw_query!=1||s->after.raw_query!=1||
    s->before.raw_before<=0||s->before.raw_after!=s->before.raw_before||s->after.raw_before!=s->before.raw_before||s->after.raw_after!=s->before.raw_before||
    !observed(&s->mutation,460,0)||s->mutation.arguments[0]!=100||s->mutation.arguments[1]!=(long)s->input||s->mutation.arguments[2]!=(long)s->input_extent||s->mutation.arguments[3])goto done;
 if(!s->pending_exec.actual_bytes||s->pending_exec.actual_bytes!=s->pending_exec.bytes||s->pending_exec.actual_capacity!=s->pending_exec.capacity||s->pending_exec.actual_capacity<34||s->pending_exec.supplied_size!=s->pending_exec.actual_capacity||s->exec_result||!s->pending_exec.returned||s->pending_exec.result||!s->pending_exec.framing_complete||s->pending_exec.raw_query!=1||s->pending_exec.raw_before!=s->before.raw_before||s->pending_exec.raw_after!=s->before.raw_before||s->pending_exec.requested_attr!=101||s->pending_exec.requested_id!=101||s->pending_exec.requested_flags!=1||s->pending_exec.observed.id!=101||s->pending_exec.observed.flags||s->pending_exec.observed.len!=32||s->pending_exec.observed.ctx_len||s->pending_exec.reported_size!=32||s->pending_exec.context||s->pending_exec.extent||!observed(&s->pending_exec.query,459,1)||s->pending_exec.query.arguments[0]!=101||s->pending_exec.query.arguments[1]!=(long)s->pending_exec.actual_bytes||s->pending_exec.query.arguments[2]!=(long)&s->pending_exec.reported_size||s->pending_exec.query.arguments[3]!=1)goto done;
 if(p->child_context.parse_original!=p->actual_payload->context.actual_parse)goto done;
 o->context_result=sl_linux_interpret_selinux_self(&p->child_context,&s->after);
 if(o->context_result){o->result=o->context_result;goto done;}
 sl_linux_selinux_context *actual=&p->child_context,*desired=&p->actual_payload->context;
 if(actual->user!=desired->user||actual->role!=desired->role||actual->type!=desired->type||!sl_linux_equal_selinux_status(&actual->after_context,&desired->after_context))goto done;
 o->result=0;
 done:o->returned=1;return o->result;
 /* This observes preexec CURRENT only. Loaded-entry, postexec confinement,
  * actor routes, complete profile and gate admission remain separate owners. */
}
#endif