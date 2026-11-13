/* UNRUN owning continuation with actual gate delivery and substituted native
 * calls. No selected175 admission, kernel, full prefix/seal/ACK proof. */
#include "../public_main_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned mode,captures;
sl_linux_candidate_arena *unused_arena(void){return 0;}
long sl_linux_acquire_public_startup(sl_linux_public_startup *s){(void)s;assert(0);return -EINVAL;}
long sl_linux_construct_contained_producer(sl_linux_public_controller *c){(void)c;assert(0);return -EINVAL;}
long sl_linux_capture_drained_sources(sl_linux_public_controller *c){captures++;c->normal_capture_returned=1;return c->normal_capture_result=mode==3?-ENOSPC:0;}
long sl_linux_write_signal_begin(sl_linux_write_signal *s){s->attempted=1;return 0;}
long sl_linux_write_signal_finish(sl_linux_write_signal *s){(void)s;return mode==2?-EIO:0;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;
 atomic_store(&c->entered,1);c->result=number==SYS_write?(mode==4?-EPIPE:1):0;atomic_store(&c->returned,1);return c->result;
}
long sl_linux_admit_candidate_birth(sl_linux_public_controller *c){(void)c;assert(0);return -EINVAL;}
int main(void){
 static sl_linux_public_main_owner owners[5];static sl_linux_public_controller controllers[5];
 static sl_linux_public_startup startups[5];static sl_linux_bootstrap bootstraps[5];static sl_linux_service_observation services[5];
 for(mode=0;mode<5;mode++){
  sl_linux_public_main_owner *o=&owners[mode];sl_linux_public_controller *c=&controllers[mode];sl_linux_service_observation *s=&services[mode];
  o->returned=o->construction_returned=1;o->construction_result=42;o->controller_original=c;o->startup_original=&startups[mode];
  c->startup_original=o->startup_original;c->construction_returned=c->construction_body_returned=c->construction_body_completed=1;c->birth_result=42;
  c->birth_admission.returned=1;c->birth_admission.controller_original=c;c->birth_admission.startup_original=c->startup_original;c->birth_admission.stage=4;
  c->entry.birth.bootstrap=&bootstraps[mode];c->entry.entry_endpoints[1]=9;c->authority_observed_by=s;
  s->returned=1;s->controller_original=c;s->authority.returned=1;s->authority.birth_original=&c->entry.birth;
  s->selinux.returned=1;s->selinux.birth_original=&c->entry.birth;c->namespace_observation.returned=1;c->namespace_observation.birth_original=&c->entry.birth;
  if(mode!=1)assert(sl_linux_deliver_gate_token(&o->gate_original,c)==(mode==2?-EIO:mode==4?-EPIPE:0));
  unsigned before=captures;long result=sl_linux_capture_public_main_owner(o);
  assert(result==(mode==0?0:mode==3?-ENOSPC:-EINVAL));
  assert(captures==before+(mode==0||mode==3));assert(o->capture_returned&&o->capture_result==result);
  assert(sl_linux_capture_public_main_owner(o)==-EINVAL&&captures==before+(mode==0||mode==3)&&o->capture_result==result);
 }
 return 0;
}