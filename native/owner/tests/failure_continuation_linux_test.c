/* UNRUN: actual finite continuation body, controlled source/native observations. */
#include "../public_controller_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned passes,observations,blocks;
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){assert(!o->attempted);o->attempted=o->returned=1;return -7;}
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){assert(!o->attempted);o->attempted=o->returned=1;return -8;}
long sl_linux_observe_root_exit(sl_linux_root_observation *o){assert(!o->attempted);o->attempted=1;passes++;return 1;}
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *o){(void)o;assert(0);return -1;}
long sl_linux_release_forward_source(sl_linux_forward_release *o){(void)o;assert(0);return -1;}
int sl_linux_observe_forward_join(sl_linux_forward_join *o){(void)o;assert(0);return -1;}
int sl_linux_observe_collector_join(sl_linux_collector_join *o){(void)o;assert(0);return -1;}
long sl_linux_start_family_worker(sl_linux_family_worker *w){(void)w;assert(0);return -1;}
long sl_linux_observe_family_join(sl_linux_family_join *o){(void)o;assert(0);return -1;}
long sl_linux_call6(sl_linux_call *o,const char *s,long n,long a,long b,long c,long d,long e,long f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;o->source=s;o->number=n;return 0;}
long sl_linux_schedule_observe(sl_linux_schedule_wait *o){assert(!o->observed);o->observed=1;observations++;return 0;}
long sl_linux_schedule_block(sl_linux_schedule_wait *o){assert(o->observed&&!o->returned);o->returned=1;o->result=-EINTR;blocks++;return o->result;}
#include "../service_linux.c"
typedef struct case_original{sl_linux_public_controller c;sl_linux_service_observation rows[3];sl_linux_schedule_wait waits[3];}case_original;
int main(void){
 case_original cases[3]={0};
 for(unsigned mode=0;mode<3;mode++){
  case_original *r=&cases[mode];sl_linux_public_controller *c=&r->c;
  c->construction_body_returned=1;c->construction_result=-EIO;c->birth_result=mode==2?0:12;c->service_roster=r->rows;c->service_capacity=3;c->service_index=1;
  r->rows[0].prepared=r->rows[0].attempted=r->rows[0].returned=1;r->rows[0].authority_result=-55;
  for(unsigned i=1;i<3;i++){r->rows[i].prepared=1;r->rows[i].controller_original=c;if(mode!=1||i!=1)r->rows[i].schedule_original=&r->waits[i];}
  unsigned before=passes,oldblocks=blocks;
  long result=sl_linux_continue_failed_sources(c);
  assert(c->construction_result==-EIO&&!c->construction_returned&&r->rows[0].authority_result==-55);
  if(!mode){assert(result==-EIO&&passes==before+2&&blocks==oldblocks+2&&c->failure_continuation_exhausted&&c->failure_continuation_last==&r->rows[2]);}
  else if(mode==1){assert(result==-EINVAL&&passes==before+1&&blocks==oldblocks&&!c->failure_continuation_exhausted&&c->failure_continuation_last==&r->rows[1]);}
  else{assert(result==-EINVAL&&passes==before&&blocks==oldblocks&&!c->failure_continuation_attempted);}
  before=passes;assert(sl_linux_continue_failed_sources(c)==-EINVAL&&passes==before);
 }
 assert(observations==2&&blocks==2);return 0;
}