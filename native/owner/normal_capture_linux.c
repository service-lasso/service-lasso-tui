#if defined(__linux__) && defined(__x86_64__)
#include "public_controller_linux.h"
#include <errno.h>

/* Actual owning call sequence, not a FINAL acceptance/disposition shortcut.
 * The prepared serializer still needs every required source segment before
 * eventual complete-prefix qualification and the separate258 seal/ACK cut. */
long sl_linux_capture_drained_sources(sl_linux_public_controller *c){
 if(!c||c->normal_capture_attempted)return -EINVAL;
 c->normal_capture_attempted=1;c->normal_capture_result=-EINVAL;
 c->capture_original=&c->final_capture;
 if(!c->construction_returned||c->construction_result||c->birth_result<=0||
    !c->final_capture.prepared||c->final_capture.prepared_controller!=c||
    c->drain_attempted||c->final_capture.attempted)goto done;
 c->normal_capture_drain_result=sl_linux_drain_sources(c);
 c->capture_cut_original=c->drained_by;
 if(c->normal_capture_drain_result){c->normal_capture_result=c->normal_capture_drain_result;goto done;}
 sl_linux_source_cut *cut=c->capture_cut_original;
 if(!c->drain_returned||c->drain_result||!cut||!cut->attempted||!cut->returned||cut->result||
    cut->controller_original!=c||!cut->service_original||cut!=&cut->service_original->source_cut||
    !cut->service_original->returned||cut->service_original->controller_original!=c)goto done;
 c->normal_capture_result=sl_linux_persist_final_capture(c->capture_original);
 done:c->normal_capture_returned=1;return c->normal_capture_result;
 /* No native view/arena release; every partial source and file stays owned. */
}
#endif
