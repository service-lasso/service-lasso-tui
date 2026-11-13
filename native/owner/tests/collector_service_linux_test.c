/* UNRUN controlled source regression; substituted join is not native proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../public_controller_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned calls;static int next_result;static void *next_pointer;static long readiness=1;
static int fixture_tryjoin(pthread_t thread,void **result){
 (void)thread;calls++;if(!next_result)*result=next_pointer;return next_result;
}
#define pthread_tryjoin_np fixture_tryjoin
#include "../collector_linux.c"
#undef pthread_tryjoin_np
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){(void)o;return readiness;}
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){(void)o;return readiness;}
long sl_linux_observe_root_exit(sl_linux_root_observation *o){(void)o;return 0;}
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *o){(void)o;assert(0);return -1;}
long sl_linux_release_forward_source(sl_linux_forward_release *o){(void)o;assert(0);return -1;}
int sl_linux_observe_forward_join(sl_linux_forward_join *o){(void)o;assert(0);return -1;}
long sl_linux_call6(sl_linux_call *o,const char *source,long number,long a,long b,long c,long d,long e,long f){
 (void)o;(void)source;(void)number;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return 0;
}
long sl_linux_start_family_worker(sl_linux_family_worker *w){(void)w;assert(0);return -1;}
long sl_linux_observe_family_join(sl_linux_family_join *j){(void)j;assert(0);return -1;}
/* This earlier fixture has no bound schedule; scheduling is covered separately. */
long sl_linux_schedule_observe(sl_linux_schedule_wait *o){(void)o;assert(0);return -1;}
long sl_linux_schedule_block(sl_linux_schedule_wait *o){(void)o;assert(0);return -1;}
#include "../service_linux.c"
static void prepare(sl_linux_public_controller *c,sl_linux_collector_join *joins,unsigned long count){
 c->collector_join_roster=joins;c->collector_join_capacity=count;
 c->collector_create_entered=c->collector_create_returned=1;atomic_init(&c->collector_returned,0);
 for(unsigned long i=0;i<count;i++)joins[i].controller_original=c;
}
static void service(sl_linux_public_controller *c,sl_linux_service_observation *o){
 o->controller_original=c;o->prepared=1;
 assert(sl_linux_service_sources(o)==0&&o->returned);
 assert(o->authority_result==1&&o->selinux_result==1);
}
int main(void){
 sl_linux_public_controller first={0},exhausted={0},invalid={0},uncreated={0};
 sl_linux_collector_join first_rows[2]={0},exhausted_rows[1]={0},invalid_rows[1]={0};
 sl_linux_service_observation observations[8]={0};
 prepare(&first,first_rows,2);next_result=EBUSY;service(&first,&observations[0]);
 assert(calls==1&&first.collector_join_index==1&&!first.collector_joined_by);
 assert(observations[0].collector_original==first_rows&&first_rows[0].result==EBUSY&&first_rows[0].returned&&first_rows[0].interpretation_returned);
 atomic_store_explicit(&first.collector_returned,1,memory_order_release);
 sl_linux_custody_prefix first_prefix={0};
 first.channel.returned=1;first.channel.result=8;first.channel.prepared_collection=&first.collection;
 first.channel.local_prefix_original=&first_prefix;first.collection.header_original=&first_prefix;
 first.collection.returned=1;first.collection.result=8;first.collection.readback_equal=1;
 next_result=0;next_pointer=&first.collection;service(&first,&observations[1]);
 assert(calls==2&&first.collector_joined_by==&first_rows[1]);
 assert(first_rows[1].interpretation_returned&&!first_rows[1].interpretation_result&&first_rows[1].joined_result==&first.collection);
 service(&first,&observations[2]);assert(calls==2&&!observations[2].collector_original);
 assert(sl_linux_observe_collector_join(&first_rows[1])==-1&&calls==2);
 prepare(&exhausted,exhausted_rows,1);next_result=EBUSY;
 service(&exhausted,&observations[3]);service(&exhausted,&observations[4]);
 assert(calls==3&&observations[4].collector_exhausted&&observations[4].collector_result==-1);
 assert(exhausted_rows[0].result==EBUSY&&!exhausted.collector_joined_by);
 prepare(&invalid,invalid_rows,1);next_result=0;next_pointer=&first.collection;
 service(&invalid,&observations[5]);
 assert(calls==4&&invalid.collector_joined_by==invalid_rows&&invalid_rows[0].result==0);
 assert(invalid_rows[0].interpretation_returned&&invalid_rows[0].interpretation_result==-1&&observations[5].collector_result==-1);
 service(&invalid,&observations[6]);assert(calls==4);
 service(&uncreated,&observations[7]);assert(calls==4&&!observations[7].collector_original);
 /* Actual controlling caller: ready authority cannot hide failed collector. */
 sl_linux_public_controller caller_bad={0},caller_empty={0},caller_busy={0},caller_errno={0};
 sl_linux_collector_join bad_row[1]={0},empty_row[1]={0},busy_row[1]={0},errno_row[1]={0};
 sl_linux_service_observation bad_service[2]={0},empty_service[1]={0},busy_service[1]={0},errno_service[1]={0};
 readiness=0;
 prepare(&caller_bad,bad_row,1);caller_bad.service_roster=bad_service;caller_bad.service_capacity=2;
 for(unsigned i=0;i<2;i++){bad_service[i].prepared=1;bad_service[i].controller_original=&caller_bad;}
 next_result=0;next_pointer=&first.collection;
 assert(sl_linux_service_until_authority(&caller_bad)==-1);
 assert(caller_bad.collector_failure_observed_by==bad_service&&!caller_bad.authority_observed_by);
 assert(bad_row[0].returned&&bad_row[0].result==0&&bad_row[0].interpretation_result==-1);
 unsigned before=calls;
 assert(sl_linux_service_until_authority(&caller_bad)==-1&&calls==before);
 assert(caller_bad.collector_failure_observed_by==bad_service&&bad_service[1].returned);
 prepare(&caller_empty,empty_row,1);caller_empty.collector_join_index=1;
 caller_empty.service_roster=empty_service;caller_empty.service_capacity=1;
 empty_service[0].prepared=1;empty_service[0].controller_original=&caller_empty;
 assert(sl_linux_service_until_authority(&caller_empty)==-1&&calls==before);
 assert(caller_empty.collector_failure_observed_by==empty_service&&empty_service[0].collector_exhausted);
 prepare(&caller_busy,busy_row,1);caller_busy.service_roster=busy_service;caller_busy.service_capacity=1;
 busy_service[0].prepared=1;busy_service[0].controller_original=&caller_busy;next_result=EBUSY;
 assert(sl_linux_service_until_authority(&caller_busy)==0&&caller_busy.authority_observed_by==busy_service);
 assert(!caller_busy.collector_failure_observed_by&&!caller_busy.collector_joined_by&&busy_row[0].result==EBUSY);
 prepare(&caller_errno,errno_row,1);caller_errno.service_roster=errno_service;caller_errno.service_capacity=1;
 errno_service[0].prepared=1;errno_service[0].controller_original=&caller_errno;next_result=EINVAL;
 assert(sl_linux_service_until_authority(&caller_errno)==-1&&!caller_errno.authority_observed_by);
 assert(errno_row[0].result==EINVAL&&errno_service[0].collector_result==EINVAL);
 sl_linux_public_controller channel_failed={0};sl_linux_collector_join channel_join[1]={0};
 sl_linux_service_observation channel_service[1]={0};
 prepare(&channel_failed,channel_join,1);channel_failed.service_roster=channel_service;channel_failed.service_capacity=1;
 channel_service[0].prepared=1;channel_service[0].controller_original=&channel_failed;
 atomic_store_explicit(&channel_failed.collector_returned,1,memory_order_release);
 channel_failed.channel.returned=1;channel_failed.channel.result=-77;
 next_result=0;next_pointer=&channel_failed.collection;
 assert(sl_linux_service_until_authority(&channel_failed)==-77&&!channel_failed.authority_observed_by);
 assert(channel_failed.collector_joined_by==channel_join&&channel_join[0].result==0);
 assert(channel_join[0].channel_original==&channel_failed.channel&&channel_join[0].channel_result==-77);
 assert(channel_join[0].interpretation_returned&&channel_join[0].interpretation_result==-77&&channel_failed.collector_failure_observed_by==channel_service);
 sl_linux_public_controller early={0};
 sl_linux_collector_join early_row={0};early_row.controller_original=&early;
 before=calls;
 assert(sl_linux_observe_collector_join(&early_row)==-1&&calls==before);
 assert(early_row.attempted&&!early_row.returned&&early_row.interpretation_returned&&early_row.interpretation_result==-1);
 assert(sl_linux_observe_collector_join(&early_row)==-1&&calls==before);
 assert(early_row.interpretation_returned&&early_row.interpretation_result==-1);
 return 0;
}
#endif