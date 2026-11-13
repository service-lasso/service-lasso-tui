#if defined(__linux__) && defined(__x86_64__)
#include "gate_linux.h"
#include <errno.h>
#include <sys/syscall.h>
long sl_linux_deliver_gate_token(sl_linux_gate_delivery *o,sl_linux_public_controller *c){
 if(!o||o->attempted)return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->controller_original=c;
 if(o->count||o->offset)goto done;
 if(!c)goto done;
 if(!c->birth_admission.returned||c->birth_admission.result||c->birth_admission.controller_original!=c||c->birth_admission.startup_original!=c->startup_original||c->birth_admission.stage!=4)goto done;
 o->birth_original=&c->entry.birth;o->bootstrap_original=c->entry.birth.bootstrap;
 sl_linux_bootstrap *b=o->bootstrap_original;
 if(!c->construction_returned||!c->construction_body_returned||!c->construction_body_completed||c->construction_result||c->birth_result<=0||
    !b||!c->authority_observed_by||!c->authority_observed_by->returned||c->authority_observed_by->controller_original!=c||
    !c->authority_observed_by->authority.returned||c->authority_observed_by->authority.result||c->authority_observed_by->authority.birth_original!=o->birth_original||
    !c->authority_observed_by->selinux.returned||c->authority_observed_by->selinux.result||c->authority_observed_by->selinux.birth_original!=o->birth_original||
    !c->namespace_observation.returned||c->namespace_observation.result||c->namespace_observation.birth_original!=o->birth_original||
    c->entry.entry_endpoints[1]<3)goto done;
 o->fd=c->entry.entry_endpoints[1];o->bytes_original=b->gate_expected;
 for(unsigned i=0;i<8;i++){if(o->writes[i].source)goto done;}
 if(o->close.source||o->signal.attempted)goto done;
 for(unsigned i=0;i<8;i++){atomic_init(&o->writes[i].entered,0);atomic_init(&o->writes[i].returned,0);}
 atomic_init(&o->close.entered,0);atomic_init(&o->close.returned,0);
 o->signal_result=sl_linux_write_signal_begin(&o->signal);
 if(o->signal_result){o->result=o->signal_result;goto done;}
 while(o->offset<8){
  sl_linux_call *call=&o->writes[o->count++];unsigned remaining=8-o->offset;
  o->write_result=sl_linux_call6(call,"owner.entry-gate.write",SYS_write,o->fd,(long)(o->bytes_original+o->offset),remaining,0,0,0);
  if(o->write_result<=0||(unsigned long)o->write_result>remaining){o->write_failure_original=call;o->result=o->write_result<0?o->write_result:-EINVAL;goto done;}
  o->offset+=(unsigned)o->write_result;
 }
 o->close_attempted=1;o->close_result=sl_linux_call6(&o->close,"owner.entry-gate.close-parent",SYS_close,o->fd,0,0,0,0,0);
 /* Every write borrower has returned before this source-specific mask cut.
  * Keep a close failure independently; restore must not erase that original. */
 o->restore_result=sl_linux_write_signal_finish(&o->signal);
 o->result=o->close_result?o->close_result:o->restore_result;
 done:o->returned=1;return o->result;
 /* Partial/failed writes preserve the blocked signal episode, FD, expected
  * bytes and all native originals. No retry, drain, release or exit here. */
}
#endif
