/* UNRUN: actual service/drain bodies; substituted native observations are not
 * genuine family, EOF, pthread join or archive durability qualification. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned passes,blocks;
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *o){(void)o;return 0;}
long sl_linux_inspect_candidate_preexec(sl_linux_candidate_preexec_inspection *o){(void)o;return 0;}
long sl_linux_observe_root_exit(sl_linux_root_observation *o){(void)o;passes++;return 1;}
int sl_linux_observe_raw_reader_join(sl_linux_reader_join *o){(void)o;assert(0);return -1;}
long sl_linux_release_forward_source(sl_linux_forward_release *o){(void)o;assert(0);return -1;}
int sl_linux_observe_forward_join(sl_linux_forward_join *o){(void)o;assert(0);return -1;}
int sl_linux_observe_collector_join(sl_linux_collector_join *o){(void)o;assert(0);return -1;}
long sl_linux_start_family_worker(sl_linux_family_worker *o){(void)o;assert(0);return -1;}
long sl_linux_observe_family_join(sl_linux_family_join *o){(void)o;assert(0);return -1;}
long sl_linux_call6(sl_linux_call *o,const char *s,long n,long a,long b,long c,long d,long e,long f){(void)o;(void)s;(void)n;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return 0;}
long sl_linux_schedule_observe(sl_linux_schedule_wait *o){assert(!o->observed);o->observed=1;return 0;}
long sl_linux_schedule_block(sl_linux_schedule_wait *o){assert(o->observed&&!o->returned);o->returned=1;o->result=-EINTR;blocks++;return o->result;}
#include "../service_linux.c"
typedef struct episode {
 sl_linux_public_controller c;sl_linux_public_startup startup;
 sl_linux_service_observation rows[2];sl_linux_schedule_wait waits[2];
 sl_linux_root_observation root;sl_linux_family_worker family;sl_linux_family_join family_join;
 sl_linux_collector_join collector;sl_linux_capture_worker reader;sl_linux_forward_worker forward;
 sl_linux_reader_join raw,raw_rows[2];sl_linux_forward_join joined,join_rows[2];sl_linux_forward_release released,release_rows[2];
 sl_linux_terminal_request request;
} episode;
int main(void){
 episode cases[18]={0};
 for(unsigned mode=0;mode<18;mode++){
  episode *e=&cases[mode];sl_linux_public_controller *c=&e->c;
  c->construction_returned=1;c->birth_result=42;c->startup_original=&e->startup;
  c->readers=&e->reader;c->forwarders=&e->forward;c->reader_count=1;
  c->service_roster=e->rows;c->service_capacity=2;
  c->root_terminal_original=&e->root;e->root.birth_original=&c->entry.birth;e->root.information.si_pid=42;e->root.information.si_code=CLD_EXITED;atomic_init(&e->root.wait.returned,1);
  c->family_dispatch_stopped_by=&e->family_join;c->family_latest_joined=c->family_active=&e->family;
  e->family_join.worker_original=&e->family;e->family_join.returned=1;e->family.joined_by=&e->family_join;e->family.root_terminal_original=&e->root;
  e->family.observation.actual_target=&e->startup.candidate_target;e->family.observation.returned=e->family.observation.framing_complete=e->family.observation.eof=1;
  c->collector_joined_by=&e->collector;e->collector.controller_original=c;e->collector.returned=e->collector.interpretation_returned=1;e->collector.channel_original=&c->channel;e->collector.collection_original=&c->collection;e->collector.channel_result=e->collector.collection_result=8;
  c->channel.prepared_requests=&e->request;c->channel.prepared_capacity=1;
  e->reader.controller_original=c;e->reader.joined_by=&e->raw;e->reader.outcome=SL_READER_READ_EOF;
  e->raw.worker_original=&e->reader;e->raw.joined_result=&e->reader;e->raw.returned=e->raw.interpretation_returned=1;
  e->forward.reader_original=&e->reader;e->forward.joined_by=&e->joined;e->forward.release_original=&e->released;e->released.raw_join_original=&e->raw;
  e->joined.worker_original=&e->forward;e->joined.joined_result=&e->forward;e->joined.returned=e->joined.interpretation_returned=1;
  atomic_init(&e->forward.release_attempt_original,&e->released);atomic_init(&e->forward.forwarding_finished,1);
  for(unsigned i=0;i<2;i++){e->rows[i].raw_joins=&e->raw_rows[i];e->rows[i].forward_releases=&e->release_rows[i];e->rows[i].forward_joins=&e->join_rows[i];e->rows[i].capacity=1;e->rows[i].schedule_original=&e->waits[i];assert(!sl_linux_prepare_service_observation(&e->rows[i],c));}
  if(mode==1)c->collector_joined_by=0;
  if(mode==2)e->raw.interpretation_result=-EIO;
  if(mode==3)e->family.root_terminal_original=0;
  if(mode==4)e->forward.result=-EPIPE;
  if(mode==5)e->collector.interpretation_result=-EINVAL;
  if(mode==6)e->reader.outcome=SL_READER_READ_FAILURE;
  if(mode==7)e->raw.interpretation_returned=0;
  if(mode>=8){
   sl_linux_terminal_request *r=&e->request;sl_linux_input_worker *input=&r->input;
   input->owner_original=r;input->mode_original=&r->mode;r->mode.input_original=input;
   input->start_attempted=input->start_returned=input->create_entered=input->create_returned=1;input->outcome=SL_INPUT_STOPPED;
   atomic_init(&input->returned,1);r->mode.terminal_original=&c->terminal;
   input->stop_original=&r->input_stop;r->input_stop.worker_original=input;r->input_stop.join_entered=r->input_stop.join_returned=r->input_stop.interpretation_returned=1;r->input_stop.joined_result=input;
   r->input_stop.attempted=r->input_stop.send_completed=1;
   atomic_init(&r->mode.change.entered,1);atomic_init(&r->mode.change.returned,1);
   r->mode.restore_original=&r->restoration;r->restoration.mode_original=&r->mode;atomic_init(&r->restoration.restore.returned,1);
   if(mode==9)r->input_stop.interpretation_result=-EIO;
   if(mode==10)r->input_stop.interpretation_returned=0;
   if(mode==11)r->input_stop.join_result=EINVAL;
   if(mode==12)r->restoration.mode_original=0;
   if(mode==13)r->restoration.restore.result=-EIO;
   if(mode==14)atomic_store(&r->mode.change.returned,0);
   if(mode==15)c->terminal.active_mode=&r->mode;
   if(mode==16){c->channel.count=0;input->start_returned=0;}
  }
  if(mode==17)e->collector.interpretation_returned=0;
  unsigned before=passes,wait_before=blocks;long result=sl_linux_drain_sources(c);
  assert(c->drain_attempted&&c->drain_returned&&!c->construction_result);
  if(!mode||mode==8){assert(!result&&c->drained_by==&e->rows[0].source_cut&&c->drained_by->checked_readers==1&&c->drained_by->checked_requests==1&&passes==before+1&&blocks==wait_before&&!c->drain_exhausted);}
  else if(mode==1){assert(result==-ENOSPC&&c->drain_exhausted&&!c->drained_by&&passes==before+2&&blocks==wait_before+2);}
  else{assert(result<0&&!c->drained_by&&!c->drain_exhausted&&passes==before+1&&blocks==wait_before);}
  assert(c->root_terminal_original==&e->root&&e->raw.result==0&&e->joined.result==0);
  before=passes;assert(sl_linux_drain_sources(c)==-EINVAL&&passes==before);
 }
 return 0;
}
