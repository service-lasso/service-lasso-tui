/* UNRUN source schedule fixture. Substituted workers are not native joins. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned roots,starts,joins;static long next_join;static int root_code;
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){(void)o;return 0;}
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){(void)o;return 0;}
long sl_linux_observe_root_exit(sl_linux_root_observation *o){roots++;o->information.si_pid=root_code?44:0;o->information.si_code=root_code;o->wait.result=0;atomic_init(&o->wait.returned,1);return 0;}
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
int main(void){
 sl_linux_public_controller c={0};sl_linux_public_startup startup={0};sl_linux_service_observation rows[8]={0};
 sl_linux_capture_worker reader={0};sl_linux_forward_worker forward={0};sl_linux_reader_join raw[8]={0};sl_linux_forward_release releases[8]={0};sl_linux_forward_join forwards[8]={0};
 c.startup_original=&startup;c.readers=&reader;c.forwarders=&forward;c.reader_count=1;c.service_roster=rows;c.service_capacity=8;
 atomic_init(&forward.forwarding_finished,0);atomic_init(&forward.release_attempt_original,0);
 for(unsigned i=0;i<8;i++){rows[i].raw_joins=&raw[i];rows[i].forward_releases=&releases[i];rows[i].forward_joins=&forwards[i];rows[i].capacity=1;assert(!sl_linux_prepare_service_observation(&rows[i],&c));}
 assert(sl_linux_service_next(&c)==&rows[0]&&starts==1&&!c.root_terminal_original);
 /* A completed zero sampled before root exit cannot stop future sampling. */
 rows[0].family.observation.returned=rows[0].family.observation.framing_complete=rows[0].family.observation.eof=1;
 root_code=CLD_STOPPED;next_join=0;
 assert(sl_linux_service_next(&c)==&rows[1]&&starts==2&&!c.root_terminal_original&&!c.family_dispatch_stopped_by);
 root_code=CLD_EXITED;next_join=EBUSY;
 assert(sl_linux_service_next(&c)==&rows[2]&&starts==2&&c.root_terminal_original==&rows[2].root&&!c.family_dispatch_stopped_by);
 /* Joined older sample still cannot use newly observed root exit. */
 rows[1].family.observation.returned=rows[1].family.observation.framing_complete=rows[1].family.observation.eof=1;next_join=0;
 assert(sl_linux_service_next(&c)==&rows[3]&&starts==3&&rows[3].family.root_terminal_original==&rows[2].root&&!c.family_dispatch_stopped_by);
 /* A post-root nonzero sample is genuine progress, not terminal. */
 rows[3].family.observation.returned=rows[3].family.observation.framing_complete=rows[3].family.observation.eof=1;rows[3].family.observation.populated=1;
 assert(sl_linux_service_next(&c)==&rows[4]&&starts==4&&!c.family_dispatch_stopped_by);
 next_join=EBUSY;assert(sl_linux_service_next(&c)==&rows[5]&&starts==4&&!c.family_dispatch_stopped_by);
 rows[4].family.observation.returned=rows[4].family.observation.framing_complete=rows[4].family.observation.eof=1;next_join=0;
 assert(sl_linux_service_next(&c)==&rows[6]&&starts==4&&c.family_dispatch_stopped_by==&rows[6].family_join&&c.family_latest_joined==&rows[4].family);
 unsigned previous_joins=joins;
 assert(sl_linux_service_next(&c)==&rows[7]&&starts==4&&joins==previous_joins&&roots==8);
 assert(!sl_linux_service_next(&c)&&c.family_dispatch_stopped_by==&rows[6].family_join);
 /* Actual preexec caller must not grant ready success for a dead root. */
 sl_linux_public_controller dead={0};sl_linux_service_observation once={0};sl_linux_reader_join dead_raw={0};sl_linux_forward_release dead_release={0};sl_linux_forward_join dead_forward={0};
 dead.startup_original=&startup;dead.readers=&reader;dead.forwarders=&forward;dead.reader_count=1;dead.service_roster=&once;dead.service_capacity=1;
 once.raw_joins=&dead_raw;once.forward_releases=&dead_release;once.forward_joins=&dead_forward;once.capacity=1;
 assert(!sl_linux_prepare_service_observation(&once,&dead));root_code=CLD_KILLED;
 unsigned before_starts=starts;
 assert(sl_linux_service_until_authority(&dead)==-ECHILD&&dead.root_terminal_original==&once.root&&!dead.authority_observed_by);
 assert(starts==before_starts+1&&dead.family_active==&once.family&&!dead.family_dispatch_stopped_by);
 assert(once.root.information.si_code==CLD_KILLED&&once.returned);
 return 0;
}