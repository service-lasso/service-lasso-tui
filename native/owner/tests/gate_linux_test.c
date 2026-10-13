/* UNRUN actual delivery body with controlled native/signal results. This is
 * not complete admission, real pipe/SIGPIPE, kernel, or gate authority proof. */
#include "../gate_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned mode,writes,closes,begins,finishes;
long sl_linux_write_signal_begin(sl_linux_write_signal *s){begins++;s->attempted=s->blocked=1;return mode==6?-EPERM:0;}
long sl_linux_write_signal_finish(sl_linux_write_signal *s){finishes++;s->finish_attempted=1;s->restored=mode!=5;return mode==5?-EIO:0;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long n,long d,long e,long f){
 c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=n;c->arguments[3]=d;c->arguments[4]=e;c->arguments[5]=f;atomic_store(&c->entered,1);
 long result;
 if(number==SYS_write){writes++;result=mode==1&&writes==2?-EPIPE:mode==2?0:mode==3?n+1:1;}
 else{assert(number==SYS_close);closes++;result=mode==4||mode==5?-EINTR:0;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
int main(void){
 static sl_linux_gate_delivery originals[13];static sl_linux_public_controller controllers[13];
 static sl_linux_service_observation observations[13];static sl_linux_bootstrap bootstraps[13];
 for(mode=0;mode<13;mode++){
  writes=closes=begins=finishes=0;sl_linux_gate_delivery *o=&originals[mode];sl_linux_public_controller *c=&controllers[mode];
  sl_linux_service_observation *s=&observations[mode];c->entry.birth.bootstrap=&bootstraps[mode];c->entry.entry_endpoints[1]=71;
  for(unsigned i=0;i<8;i++)bootstraps[mode].gate_expected[i]=(unsigned char)(40+i);
  c->construction_returned=c->construction_body_returned=c->construction_body_completed=1;c->birth_result=42;
  c->birth_admission.returned=mode!=12;c->birth_admission.controller_original=c;c->birth_admission.startup_original=c->startup_original;c->birth_admission.stage=4;
  c->authority_observed_by=s;s->controller_original=c;s->returned=1;s->authority.returned=s->selinux.returned=1;
  s->authority.birth_original=s->selinux.birth_original=&c->entry.birth;
  c->namespace_observation.returned=mode!=7;c->namespace_observation.birth_original=&c->entry.birth;
  if(mode==8)o->count=8;if(mode==9)o->offset=8;if(mode==10)o->count=~0U;if(mode==11)o->offset=~0U;
  unsigned initial_count=o->count,initial_offset=o->offset;
  long result=sl_linux_deliver_gate_token(o,c);
  assert(o->returned&&o->controller_original==c);
  assert(result==(mode==0?0:mode==1?-EPIPE:mode==4||mode==5?-EINTR:mode==6?-EPERM:-EINVAL));
  if(mode==0||mode==4||mode==5){assert(writes==8&&closes==1&&finishes==1&&o->offset==8);for(unsigned i=0;i<8;i++)assert(o->writes[i].arguments[1]==(long)&bootstraps[mode].gate_expected[i]);}
  if(mode==1){assert(writes==2&&o->offset==1&&o->write_failure_original==&o->writes[1]&&!closes&&!finishes);}
  if(mode==2||mode==3)assert(writes==1&&!o->offset&&!closes&&!finishes);
  if(mode==5)assert(o->close_result==-EINTR&&o->restore_result==-EIO);
  if(mode==6||mode==7)assert(!writes&&!closes&&!finishes);
  if(mode>=8)assert(!writes&&!closes&&!begins&&!finishes&&o->count==initial_count&&o->offset==initial_offset);
  unsigned w=writes,cl=closes,be=begins,fi=finishes;assert(sl_linux_deliver_gate_token(o,c)==-EINVAL&&o->result==result&&writes==w&&closes==cl&&begins==be&&finishes==fi);
 }
 return 0;
}
