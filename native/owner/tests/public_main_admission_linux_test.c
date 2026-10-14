/* UNRUN actual public admission->gate caller, controlled stage results only. */
#include "../public_main_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,admissions,gates;
long sl_linux_acquire_public_startup(sl_linux_public_startup *s){(void)s;assert(0);return -1;}
long sl_linux_construct_contained_producer(sl_linux_public_controller *c){(void)c;assert(0);return -1;}
long sl_linux_capture_drained_sources(sl_linux_public_controller *c){(void)c;assert(0);return -1;}
long sl_linux_admit_candidate_birth(sl_linux_public_controller *c){admissions++;c->birth_admission.controller_original=c;c->birth_admission.returned=1;return c->birth_admission.result=mode==1?-EPERM:0;}
long sl_linux_deliver_gate_token(sl_linux_gate_delivery *g,sl_linux_public_controller *c){gates++;assert(c->birth_admission.returned&&!c->birth_admission.result);g->controller_original=c;g->attempted=g->returned=1;return g->result=mode==2?-EPIPE:0;}
#include "../public_main_linux.c"
int main(void){static sl_linux_public_main_owner originals[4];static sl_linux_public_controller controllers[4];static sl_linux_public_startup startups[4];for(mode=0;mode<4;mode++){
 sl_linux_public_main_owner *o=&originals[mode];sl_linux_public_controller *c=&controllers[mode];o->returned=o->construction_returned=1;o->construction_result=42;o->controller_original=c;o->startup_original=&startups[mode];c->startup_original=o->startup_original;c->birth_result=mode==3?43:42;
 unsigned before=admissions,oldgates=gates;long r=sl_linux_admit_public_main_owner(o);assert(o->admission_returned&&r==(mode==0?0:mode==1?-EPERM:mode==2?-EPIPE:-EINVAL));assert(admissions==before+(mode!=3)&&gates==oldgates+(mode==0||mode==2));assert(o->gate_returned==(mode==0||mode==2));
 assert(sl_linux_admit_public_main_owner(o)==-EINVAL&&o->admission_result==r&&admissions==before+(mode!=3)&&gates==oldgates+(mode==0||mode==2));
 }return 0;}