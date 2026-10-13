#if defined(__linux__) && defined(__x86_64__)
#include "public_main_linux.h"
#include "outer_birth_linux.h"
#include "shared_range_linux.h"
#include <errno.h>
#include <sys/syscall.h>
long sl_linux_attach_public_main_owner(sl_linux_public_main_owner *o,sl_linux_candidate_arena *a,
 int argc,char **argv,char **environment){
 if(!o||!a||!a->returned||a->result||!a->bound||!a->layout_complete||a->public_main_owner!=o||
    !a->outer_birth||a->outer_birth->attempted||atomic_load(&a->outer_birth->entered)||
    argc<0||(argc&&!argv))return -EINVAL;
 sl_linux_shared_range range={&a->mapping,a->layout.public_main_owner.offset,a->layout.public_main_owner.bytes};
 if(sl_linux_shared_range_address(&range,sizeof(*o),_Alignof(sl_linux_public_main_owner))!=o)return -EINVAL;
 uintptr_t first=(uintptr_t)o,root=(uintptr_t)a;
 if(sizeof(*o)>UINTPTR_MAX-first||sizeof(*a)>UINTPTR_MAX-root||
    (root<first+sizeof(*o)&&first<root+sizeof(*a)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;
 for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 o->attachment_attempted=1;o->attachment_result=-EINVAL;o->prepared_arena_original=a;
 o->argc=argc;o->argv=argv;o->environment=environment;
 o->attachment_result=0;o->attachment_returned=1;return 0;
 /* Real prebirth source attachment, not native execution or birth admission.
  * Original argument/environment and containing acquisition lifetimes remain
  * held by the actual generated entry through every later borrower. */
}
long sl_linux_construct_public_main_owner(sl_linux_public_main_owner *o){
 if(!o||o->attempted)return -EINVAL;
 o->attempted=1;o->result=-EINVAL;
 if(o->argc<0||(o->argc&&!o->argv))goto done;
 /* Consume the exact prepared attachment on the outer-observer route.
  * The historical callback route retains its actual separate return marker;
  * attaching an existing arena never fabricates a callback/map return. */
 if(o->prepared_arena_original){
  if(!o->attachment_attempted||!o->attachment_returned||o->attachment_result||o->construct_arena_original||
     o->prepared_arena_original->public_main_owner!=o)goto done;
  o->arena_original=o->prepared_arena_original;
 }else{
  if(!o->construct_arena_original)goto done;
  o->arena_original=o->construct_arena_original();o->arena_returned=1;
 }
 if(!o->arena_original)goto done;
 o->arena_result=o->arena_original->result;
 if(!o->arena_original->returned||o->arena_result||!o->arena_original->bound){
  if(o->arena_result)o->result=o->arena_result;goto done;
 }
 o->startup_original=o->arena_original->startup;
 if(!o->startup_original)goto done;
 o->controller_original=o->startup_original->controller_original;
 if(!o->controller_original||o->controller_original->startup_original!=o->startup_original)goto done;
 o->startup_original->argc=o->argc;o->startup_original->argv=o->argv;o->startup_original->environment=o->environment;
 o->startup_result=sl_linux_acquire_public_startup(o->startup_original);o->startup_returned=1;
 if(o->startup_result){o->result=o->startup_result;goto done;}
 if(!o->startup_original->returned||o->startup_original->result||o->startup_original->stage!=SL_STARTUP_ACQUIRED)goto done;
 o->construction_result=sl_linux_construct_contained_producer(o->controller_original);o->construction_returned=1;
 if(o->construction_result<=0||!o->controller_original->construction_returned||
    !o->controller_original->construction_body_returned||!o->controller_original->construction_body_completed||
    o->controller_original->construction_result||o->construction_result!=o->controller_original->birth_result){
  o->result=o->construction_result<0?o->construction_result:
   o->controller_original->construction_result?o->controller_original->construction_result:-EINVAL;goto done;
 }
 /* Actual construction leaves the child gate closed. The owning entry must
  * complete separate selected admission and actual gate write before calling
  * the normal drain/capture continuation. Never wait here on that blocked child. */
 o->result=0;
 done:o->returned=1;return o->result;
 /* SAME owning storage retains all partial arena/native roots on every return.
  * Full profile/gate, complete prefix, seal/ACK and final disposition are
  * required before the public entry may consume an eligible return status. */
}
long sl_linux_admit_public_main_owner(sl_linux_public_main_owner *o){
 if(!o||o->admission_attempted)return -EINVAL;
 o->admission_attempted=1;o->admission_result=-EINVAL;
 sl_linux_public_controller *c=o->controller_original;
 if(!o->returned||o->result||!o->construction_returned||!c||c->startup_original!=o->startup_original||o->construction_result<=0||o->construction_result!=c->birth_result)goto done;
 o->admission_result=sl_linux_admit_candidate_birth(c);
 if(o->admission_result)goto done;
 o->gate_result=sl_linux_deliver_gate_token(&o->gate_original,c);o->gate_returned=1;
 if(o->gate_result)o->admission_result=o->gate_result;
 done:o->admission_returned=1;return o->admission_result;
 /* This operation retains actual admission/delivery outcomes. It does not
  * authorize process return, full archive acceptance or graph disposal. */
}
long sl_linux_capture_public_main_owner(sl_linux_public_main_owner *o){
 if(!o||o->capture_returned)return -EINVAL;
 o->capture_result=-EINVAL;
 sl_linux_public_controller *c=o->controller_original;
 sl_linux_gate_delivery *g=&o->gate_original;
 if(!o->returned||o->result||!o->construction_returned||!c||
    c->startup_original!=o->startup_original||!c->construction_returned||
    !c->construction_body_returned||!c->construction_body_completed||c->construction_result||
    o->construction_result<=0||o->construction_result!=c->birth_result||
    !g->attempted||!g->returned||g->result||g->controller_original!=c||
    g->birth_original!=&c->entry.birth||g->bootstrap_original!=c->entry.birth.bootstrap||
    !g->bootstrap_original||g->bytes_original!=g->bootstrap_original->gate_expected||
    g->offset!=8||!g->count||g->count>8||g->signal_result||g->write_failure_original||
    !g->close_attempted||g->close_result||g->restore_result)goto done;
 unsigned offset=0;
 for(unsigned i=0;i<g->count;i++){
  sl_linux_call *call=&g->writes[i];
  if(!atomic_load_explicit(&call->returned,memory_order_acquire)||
     call->number!=SYS_write||call->arguments[0]!=g->fd||
     call->arguments[1]!=(long)(g->bytes_original+offset)||call->arguments[2]!=(long)(8-offset)||
     call->result<=0||(unsigned long)call->result>8-offset)goto done;
  offset+=(unsigned)call->result;
 }
 if(offset!=8||!atomic_load_explicit(&g->close.returned,memory_order_acquire)||
    g->close.number!=SYS_close||g->close.arguments[0]!=g->fd||g->close.result)goto done;
 c->final_capture.gate_original=g;
 o->capture_result=sl_linux_capture_drained_sources(c);
 done:o->capture_returned=1;return o->capture_result;
 /* No gate invocation, admission or process-return authority. SAME surviving
  * owning storage retains failed and partial capture roots through final disposition. */
}
#endif
