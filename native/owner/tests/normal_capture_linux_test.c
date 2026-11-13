/* UNRUN: actual continuation body with substituted drain/persistence.
 * This checks call-site cuts, not family EOF, durability or seal authority. */
#include "../public_controller_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned drains,persists;static unsigned mode;
static sl_linux_service_observation services[6];
long sl_linux_drain_sources(sl_linux_public_controller *c){
 drains++;c->drain_attempted=1;c->drain_returned=1;
 if(mode==1){c->drain_result=-ENOSPC;c->drain_exhausted=1;return -ENOSPC;}
 sl_linux_service_observation *s=&services[mode];s->controller_original=c;s->returned=1;
 s->source_cut.controller_original=c;s->source_cut.service_original=s;s->source_cut.attempted=s->source_cut.returned=1;
 c->drained_by=&s->source_cut;
 if(mode==2)s->source_cut.returned=0;
 if(mode==3)s->source_cut.controller_original=0;
 return 0;
}
long sl_linux_persist_final_capture(sl_linux_final_capture *r){persists++;r->attempted=r->returned=1;r->result=mode==4?-EIO:0;return r->result;}
int main(void){
 static sl_linux_public_controller originals[6];
 for(mode=0;mode<6;mode++){
  sl_linux_public_controller *c=&originals[mode];c->construction_returned=1;c->birth_result=42;
  c->final_capture.prepared=1;c->final_capture.prepared_controller=c;
  if(mode==5)c->final_capture.attempted=1;
  unsigned before_drains=drains,before_persists=persists;
  long result=sl_linux_capture_drained_sources(c);
  assert(c->normal_capture_attempted&&c->normal_capture_returned&&c->normal_capture_result==result&&c->capture_original==&c->final_capture);
  assert(drains==before_drains+(mode!=5));
  assert(persists==before_persists+(mode==0||mode==4));
  assert(result==(mode==0?0:mode==1?-ENOSPC:mode==4?-EIO:-EINVAL));
  assert(sl_linux_capture_drained_sources(c)==-EINVAL&&drains==before_drains+(mode!=5)&&persists==before_persists+(mode==0||mode==4));
  assert(c->normal_capture_result==result);
 }
 return 0;
}
