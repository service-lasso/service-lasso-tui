/* UNRUN source schedule fixture. Substituted workers are not native joins. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned roots,starts,joins;static long next_join;
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){(void)o;return 0;}
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){(void)o;return 0;}
long sl_linux_observe_root_exit(sl_linux_root_observation *o){(void)o;roots++;return 0;}
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *o){(void)o;assert(0);return -1;}
long sl_linux_release_forward_source(sl_linux_forward_release *o){(void)o;assert(0);return -1;}
int sl_linux_observe_forward_join(sl_linux_forward_join *o){(void)o;assert(0);return -1;}
int sl_linux_observe_collector_join(sl_linux_collector_join *o){(void)o;assert(0);return -1;}
long sl_linux_call6(sl_linux_call *o,const char *s,long n,long a,long b,long c,long d,long e,long f){(void)o;(void)s;(void)n;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return 0;}
long sl_linux_start_family_worker(sl_linux_family_worker *w){assert(!w->attempted&&w->observation.target_original);starts++;w->attempted=w->create_returned=1;return 0;}
long sl_linux_observe_family_join(sl_linux_family_join *o){joins++;assert(o->worker_original&&!o->worker_original->joined_by);o->attempted=o->returned=1;o->interpretation_result=next_join;o->native_result=next_join==EBUSY?EBUSY:0;if(next_join!=EBUSY)o->worker_original->joined_by=o;return next_join;}
/* This earlier fixture has no bound schedule; scheduling is covered separately. */
long sl_linux_schedule_observe(sl_linux_schedule_wait *o){(void)o;assert(0);return -1;}
long sl_linux_schedule_block(sl_linux_schedule_wait *o){(void)o;assert(0);return -1;}
#include "../service_linux.c"
int main(void){sl_linux_public_controller c={0};sl_linux_public_startup startup={0};sl_linux_service_observation rows[5]={0};sl_linux_capture_worker reader={0};sl_linux_forward_worker forward={0};sl_linux_reader_join raw[5]={0};sl_linux_forward_release releases[5]={0};sl_linux_forward_join forwards[5]={0};
 c.startup_original=&startup;c.readers=&reader;c.forwarders=&forward;c.reader_count=1;c.service_roster=rows;c.service_capacity=5;
 atomic_init(&forward.forwarding_finished,0);atomic_init(&forward.release_attempt_original,0);
 for(unsigned i=0;i<5;i++){rows[i].raw_joins=&raw[i];rows[i].forward_releases=&releases[i];rows[i].forward_joins=&forwards[i];rows[i].capacity=1;assert(!sl_linux_prepare_service_observation(&rows[i],&c));}
 assert(sl_linux_service_next(&c)==&rows[0]&&roots==1&&starts==1&&c.family_active==&rows[0].family);
 next_join=EBUSY;assert(sl_linux_service_next(&c)==&rows[1]&&roots==2&&starts==1&&joins==1&&!c.family_failure_observed_by&&rows[1].family_join.native_result==EBUSY);
 next_join=0;assert(sl_linux_service_next(&c)==&rows[2]&&roots==3&&starts==2&&joins==2&&c.family_latest_joined==&rows[0].family&&c.family_active==&rows[2].family);
 next_join=-77;assert(sl_linux_service_until_authority(&c)==-77&&roots==4&&starts==2&&joins==3&&c.family_failure_observed_by==&rows[3]&&!c.authority_observed_by&&rows[3].family_join.native_result==0&&rows[3].family_join.interpretation_result==-77);
 assert(sl_linux_service_until_authority(&c)==-77&&roots==5&&starts==2&&joins==3&&c.family_failure_observed_by==&rows[3]);return 0;}