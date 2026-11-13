/* UNRUN finite service scheduling case with substituted native observations.
 * A transition observation is not profile/gate/native/runtime acceptance. */
#if defined(__linux__) && defined(__x86_64__)
#include "../public_controller_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
static unsigned inspected,root_calls,yields,selinux_calls;static int never_ready,failure,selinux_delayed,selinux_failure;
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){assert(!o->attempted);o->attempted=o->returned=1;selinux_calls++;o->result=selinux_failure?-8:selinux_delayed&&selinux_calls<3?1:0;o->not_ready=o->result==1;return o->result;}
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){
 assert(!o->attempted);o->attempted=o->returned=1;inspected++;
 o->result=inspected==1||never_ready?1:failure?-7:0;o->not_ready=o->result==1;return o->result;
}
long sl_linux_observe_root_exit(sl_linux_root_observation *o){assert(!o->attempted);o->attempted=1;root_calls++;return 1;}
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *o){(void)o;assert(0);return -1;}
long sl_linux_release_forward_source(sl_linux_forward_release *o){(void)o;assert(0);return -1;}
int sl_linux_observe_forward_join(sl_linux_forward_join *o){(void)o;assert(0);return -1;}
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 assert(n==SYS_sched_yield);yields++;c->source=s;c->number=n;
 long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);c->result=0;atomic_store(&c->returned,1);return 0;
}
long sl_linux_start_family_worker(sl_linux_family_worker *w){(void)w;assert(0);return -1;}
long sl_linux_observe_family_join(sl_linux_family_join *j){(void)j;assert(0);return -1;}
/* This earlier fixture has no bound schedule; scheduling is covered separately. */
long sl_linux_schedule_observe(sl_linux_schedule_wait *o){(void)o;assert(0);return -1;}
long sl_linux_schedule_block(sl_linux_schedule_wait *o){(void)o;assert(0);return -1;}
#include "../service_linux.c"
int main(void){
 sl_linux_public_controller c;sl_linux_service_observation roster[3];
 sl_linux_reader_join raw[3];sl_linux_forward_release releases[3];sl_linux_forward_join joins[3];
 sl_linux_capture_worker reader;sl_linux_forward_worker forward;
 for(unsigned mode=0;mode<5;mode++){
  memset(&c,0,sizeof c);memset(roster,0,sizeof roster);memset(raw,0,sizeof raw);memset(releases,0,sizeof releases);memset(joins,0,sizeof joins);
  memset(&reader,0,sizeof reader);memset(&forward,0,sizeof forward);
  atomic_init(&forward.forwarding_finished,0);atomic_init(&forward.release_attempt_original,0);
  inspected=root_calls=yields=selinux_calls=0;never_ready=mode==1;failure=mode==2;selinux_delayed=mode==3;selinux_failure=mode==4;
  c.readers=&reader;c.forwarders=&forward;c.reader_count=1;c.service_roster=c.initial_service=roster;c.service_capacity=3;
  for(unsigned i=0;i<3;i++){
   roster[i].raw_joins=&raw[i];roster[i].forward_releases=&releases[i];roster[i].forward_joins=&joins[i];roster[i].capacity=1;
   assert(sl_linux_prepare_service_observation(&roster[i],&c)==0);
  }
  long result=sl_linux_service_until_authority(&c);
  assert(roster[0].authority_result==1&&roster[0].authority.not_ready);
  if(!mode){assert(result==0&&c.authority_observed_by==&roster[1]&&c.service_index==2);assert(roster[1].root_result==1);}
  else if(mode==1){assert(result<0&&!c.authority_observed_by&&c.service_index==3);assert(!sl_linux_service_next(&c));}
  else if(mode==2)assert(result==-7&&!c.authority_observed_by&&c.service_index==2);
  else if(mode==3)assert(result==0&&c.authority_observed_by==&roster[2]&&c.service_index==3&&roster[1].authority_result==0&&roster[1].selinux_result==1);
  else assert(result==-8&&!c.authority_observed_by&&c.service_index==1);
  assert(root_calls==inspected&&selinux_calls==inspected&&yields+1==inspected);
  unsigned before=root_calls;if(!mode)assert(sl_linux_service_until_authority(&c)<0&&root_calls==before);
  /* Source service calls never write the existing child gate. */
  assert(!atomic_load(&c.entry.entry_pipe.entered));
 }
 return 0;
}
#endif