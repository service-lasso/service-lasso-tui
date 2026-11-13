#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <sys/syscall.h>
#include <errno.h>
long sl_linux_prepare_service_observation(sl_linux_service_observation *observation,sl_linux_public_controller *controller){
 unsigned long index;

 if(!observation||observation->prepared||observation->attempted||!controller||
    !observation->raw_joins||!observation->forward_releases||!observation->forward_joins||
    observation->capacity!=controller->reader_count)return -1;
 for(index=0;index<observation->capacity;index++){
  if(observation->raw_joins[index].attempted||observation->forward_releases[index].attempted||observation->forward_joins[index].attempted)return -1;
 }
 observation->controller_original=controller;
 observation->root.birth_original=&controller->entry.birth;
 if(controller->startup_original)observation->family.observation.target_original=&controller->startup_original->candidate_target;
 observation->authority.birth_original=&controller->entry.birth;
 observation->selinux.birth_original=&controller->entry.birth;
 observation->selinux.owner_original=controller->candidate_preexec_original;
 for(index=0;index<observation->capacity;index++){
  observation->raw_joins[index].worker_original=&controller->readers[index];
  observation->forward_releases[index].worker_original=&controller->forwarders[index];
  observation->forward_joins[index].worker_original=&controller->forwarders[index];
 }
 if(observation->schedule_yield.source||observation->schedule_yield.number||
    atomic_load(&observation->schedule_yield.entered)||atomic_load(&observation->schedule_yield.returned))return -1;
 atomic_init(&observation->schedule_yield.entered,0);atomic_init(&observation->schedule_yield.returned,0);
 observation->prepared=1;return 0;
}
long sl_linux_service_sources(sl_linux_service_observation *observation){
 unsigned long index;
 sl_linux_public_controller *controller;
 if(!observation||!observation->prepared||observation->attempted||!observation->controller_original)return -1;
 observation->attempted=1;
 controller=observation->controller_original;
 observation->authority_result=sl_linux_inspect_preexec_authority(&observation->authority);
 observation->selinux_result=sl_linux_inspect_candidate_preexec(&observation->selinux);
 observation->root_result=sl_linux_observe_root_exit(&observation->root);
 if(!controller->root_terminal_original&&!observation->root_result&&
    atomic_load_explicit(&observation->root.wait.returned,memory_order_acquire)&&
    observation->root.wait.result==0&&observation->root.information.si_pid>0&&
    (observation->root.information.si_code==CLD_EXITED||
     observation->root.information.si_code==CLD_KILLED||observation->root.information.si_code==CLD_DUMPED))
  controller->root_terminal_original=&observation->root;
 /* Family IO is dispatched independently below; controller never reads it. */
 /* A root failure/notyet does not skip independently eligible source/join
  * observations. None of these operations forwards bytes or waits for IO. */
 for(index=0;index<observation->capacity;index++){
  sl_linux_capture_worker *reader=observation->raw_joins[index].worker_original;
  sl_linux_forward_worker *forward=observation->forward_joins[index].worker_original;
  if(reader->create_returned&&!reader->create_result&&!reader->joined_by)
   sl_linux_observe_raw_reader_join(&observation->raw_joins[index]);
  if(reader->joined_by&&(!reader->joined_by->interpretation_returned||reader->joined_by->interpretation_result)&&!controller->source_failure_observed_by){observation->raw_failure_original=reader->joined_by;observation->source_result=reader->joined_by->interpretation_result?reader->joined_by->interpretation_result:-1;controller->source_failure_observed_by=observation;}
  if(reader->joined_by&&reader->joined_by->interpretation_returned&&!reader->joined_by->interpretation_result&&atomic_load_explicit(&forward->forwarding_finished,memory_order_acquire)&&
     !atomic_load_explicit(&forward->release_attempt_original,memory_order_acquire)){
   observation->forward_releases[index].raw_join_original=reader->joined_by;
   sl_linux_release_forward_source(&observation->forward_releases[index]);
  }
  if(atomic_load_explicit(&forward->release_attempt_original,memory_order_acquire)&&
     forward->create_returned&&!forward->create_result&&!forward->joined_by)
   sl_linux_observe_forward_join(&observation->forward_joins[index]);
  if(forward->joined_by&&(!forward->joined_by->interpretation_returned||forward->joined_by->interpretation_result)&&!controller->source_failure_observed_by){observation->forward_failure_original=forward->joined_by;observation->source_result=forward->joined_by->interpretation_result?forward->joined_by->interpretation_result:-1;controller->source_failure_observed_by=observation;}
 }
 /* The collector is independent of root readiness and every raw reader.
  * Consume fresh preowned rows even during preexec observation; no blocking
  * join and no interpretation of body-return publication as native exit. */
 if(controller->collector_create_returned&&!controller->collector_create_result&&!controller->collector_joined_by){
  if(!controller->collector_join_roster||controller->collector_join_index>=controller->collector_join_capacity){
   observation->collector_exhausted=1;observation->collector_result=-1;
  }else{
   sl_linux_collector_join *join=&controller->collector_join_roster[controller->collector_join_index++];
   observation->collector_original=join;
   if(join->controller_original!=controller||join->attempted||join->returned)observation->collector_result=-1;
   else observation->collector_result=sl_linux_observe_collector_join(join);
  }
 }
 if((observation->collector_exhausted||(observation->collector_result&&observation->collector_result!=EBUSY))&&
    !controller->collector_failure_observed_by)controller->collector_failure_observed_by=observation;
 /* At most one active actual family reader. A blocked read is retained in its
  * own worker while this controller continues all independent sources. */
 if(controller->family_active&&!controller->family_active->joined_by){
  observation->family_join.worker_original=controller->family_active;
  observation->family_result=sl_linux_observe_family_join(&observation->family_join);
  if(controller->family_active->joined_by&&!observation->family_result){
   sl_linux_family_worker *worker=controller->family_active;
   controller->family_latest_joined=worker;
   /* This sample was dispatched after the actual root terminal observation.
    * Its real native join precedes dispatch stop. No body publication, prior
    * zero or root-only observation supplies this relation. */
   if(worker->root_terminal_original&&worker->root_terminal_original==controller->root_terminal_original&&
      worker->observation.returned&&worker->observation.framing_complete&&worker->observation.eof&&
      !worker->observation.result&&!worker->observation.populated)
    controller->family_dispatch_stopped_by=worker->joined_by;
  }
  if(observation->family_result&&observation->family_result!=EBUSY&&!controller->family_failure_observed_by)controller->family_failure_observed_by=observation;
 }
 if(!controller->family_dispatch_stopped_by&&!controller->family_failure_observed_by&&(!controller->family_active||controller->family_active->joined_by)&&observation->family.observation.target_original){
  observation->family.root_terminal_original=controller->root_terminal_original;
  controller->family_active=&observation->family;
  observation->family_result=sl_linux_start_family_worker(&observation->family);
  if(observation->family_result&&!controller->family_failure_observed_by)controller->family_failure_observed_by=observation;
 }
 observation->returned=1;
 observation->source_cut_result=sl_linux_observe_source_cut(&observation->source_cut,observation);
 return observation->root_result;
}
sl_linux_service_observation *sl_linux_service_next(sl_linux_public_controller *controller){
 if(!controller||!controller->service_roster||controller->service_index>=controller->service_capacity)return 0;
 sl_linux_service_observation *o=&controller->service_roster[controller->service_index];
 if(!o->prepared||o->attempted||o->controller_original!=controller)return 0;
 unsigned long index=controller->service_index++;
 /* Actual scheduler observation, not a timing/liveness grant or sleep default.
  * Every next occurrence is separately preowned before any worker dispatch. */
 if(index)o->yield_result=sl_linux_call6(&o->schedule_yield,"controller.service.sched-yield",SYS_sched_yield,0,0,0,0,0,0);
 if(o->schedule_original)o->schedule_observe_result=sl_linux_schedule_observe(o->schedule_original);
 sl_linux_service_sources(o);return o;
}
long sl_linux_service_until_authority(sl_linux_public_controller *controller){
 if(!controller||controller->authority_observed_by)return -1;
 for(;;){
  sl_linux_service_observation *o=sl_linux_service_next(controller);
  if(!o)return -1;
  if(controller->source_failure_observed_by){long failure=controller->source_failure_observed_by->source_result;return failure<0?failure:-1;}
  if(controller->collector_failure_observed_by){
   long failure=controller->collector_failure_observed_by->collector_result;
   /* pthread failures are positive errno values, not child PIDs/success.
    * Preserve that exact original row; project only this caller's denial. */
   return failure<0?failure:-1;
  }
  if(controller->family_failure_observed_by){long failure=controller->family_failure_observed_by->family_result;return failure<0?failure:-1;}
  if(o->yield_result<0)return o->yield_result;
  if(o->authority_result<0)return o->authority_result;
  if(o->selinux_result<0)return o->selinux_result;
  if(controller->root_terminal_original)return -ECHILD;
  if(!o->authority_result&&!o->selinux_result){controller->authority_observed_by=o;return 0;}
  if(o->schedule_original){
   if(o->schedule_observe_result)return o->schedule_observe_result;
   o->schedule_wait_result=sl_linux_schedule_block(o->schedule_original);
   /* Every returned wait outcome advances to a fresh service occurrence.
    * Raw EINTR/EAGAIN/timeout/error stays in the SAME once-owned wait cell.
    * Worker join must still settle late notification syscall originals. */
  }
  /* Not-ready is not failure/completion. Continue with the next real owned
   * occurrence; finite roster exhaustion denies without overwriting history. */
 }
}
long sl_linux_continue_failed_sources(sl_linux_public_controller *c){
 if(!c||!c->construction_body_returned||!c->construction_result||c->birth_result<=0||c->failure_continuation_attempted)return -EINVAL;
 c->failure_continuation_attempted=1;c->failure_continuation_result=c->construction_result;
 /* Keep the original constructor failure primary. Fresh rows still service
  * independently eligible producers/joins; no source failure grants exit. */
 while(c->service_index<c->service_capacity){
  sl_linux_service_observation *o=sl_linux_service_next(c);
  if(!o){c->failure_continuation_result=-EINVAL;break;}
  c->failure_continuation_last=o;
  if(!o->schedule_original||o->schedule_observe_result){c->failure_continuation_result=o->schedule_observe_result?o->schedule_observe_result:-EINVAL;break;}
  o->schedule_wait_result=sl_linux_schedule_block(o->schedule_original);
  /* A returned error/not-ready/timeout is retained in this exact row. The
   * next pass uses a new row; no uncertain operation or wait is retried. */
 }
 if(c->service_index==c->service_capacity)c->failure_continuation_exhausted=1;
 c->failure_continuation_returned=1;return c->failure_continuation_result;
 /* Owning public-main terminal retention is a separate required continuation;
  * this finite service return is never permission to release arena or exit. */
}
/* A fresh cut retains the actual source graph. It neither releases that graph
 * nor grants FINAL persistence, independent ACK or public success. */
long sl_linux_observe_source_cut(sl_linux_source_cut *cut,sl_linux_service_observation *service){
 sl_linux_public_controller *c;long result=1;
 if(!cut||cut->attempted||!service||!service->returned||!service->controller_original||cut!=&service->source_cut)return -EINVAL;
 cut->attempted=1;cut->service_original=service;c=service->controller_original;cut->controller_original=c;
 cut->root_original=c->root_terminal_original;cut->family_original=c->family_dispatch_stopped_by;
 cut->collector_original=c->collector_joined_by;cut->readers_original=c->readers;cut->forwarders_original=c->forwarders;cut->reader_count=c->reader_count;
 if(c->source_failure_observed_by){cut->failure_original=c->source_failure_observed_by;result=c->source_failure_observed_by->source_result;goto failed;}
 if(c->collector_failure_observed_by){cut->failure_original=c->collector_failure_observed_by;result=c->collector_failure_observed_by->collector_result;goto failed;}
 if(c->family_failure_observed_by){cut->failure_original=c->family_failure_observed_by;result=c->family_failure_observed_by->family_result;goto failed;}
 if(!cut->root_original||!cut->family_original||!cut->collector_original)goto finished;
 if(!c->readers||!c->forwarders||!c->reader_count){cut->failure_original=c;result=-EINVAL;goto finished;}
 sl_linux_root_observation *root=cut->root_original;
 sl_linux_family_join *family=cut->family_original;sl_linux_family_worker *worker=family->worker_original;
 sl_linux_collector_join *collector=cut->collector_original;
 if(root->birth_original!=&c->entry.birth||!atomic_load_explicit(&root->wait.returned,memory_order_acquire)||root->wait.result||root->information.si_pid<=0||
    (root->information.si_code!=CLD_EXITED&&root->information.si_code!=CLD_KILLED&&root->information.si_code!=CLD_DUMPED)){
  cut->failure_original=root;result=-EINVAL;goto finished;
 }
 if(!worker||c->family_latest_joined!=worker||c->family_active!=worker||worker->joined_by!=family||!family->returned||family->native_result||family->interpretation_result||
    worker->root_terminal_original!=root||!worker->observation.returned||!worker->observation.framing_complete||!worker->observation.eof||worker->observation.result||worker->observation.populated||
    !c->startup_original||worker->observation.actual_target!=&c->startup_original->candidate_target){
  cut->failure_original=family;result=-EINVAL;goto finished;
 }
 if(collector->controller_original!=c||!collector->returned||collector->result||!collector->interpretation_returned||collector->interpretation_result||
    collector->channel_original!=&c->channel||collector->collection_original!=&c->collection||collector->channel_result!=8||collector->collection_result!=8){
  cut->failure_original=collector;result=-EINVAL;goto finished;
 }
 /* Native collector join settles the request producer before these reads.
  * Walk every prepared root: returned channel.count is not custody frontier. */
 cut->requests_original=c->channel.prepared_requests;cut->request_capacity=c->channel.prepared_capacity;
 if(!cut->requests_original||!cut->request_capacity||c->channel.count>cut->request_capacity){cut->failure_original=&c->channel;result=-EINVAL;goto finished;}
 for(unsigned long i=0;i<cut->request_capacity;i++){
  sl_linux_terminal_request *request=&cut->requests_original[i];
  sl_linux_input_worker *input=&request->input;sl_linux_mode_episode *mode=&request->mode;
  if(input->start_attempted){
   sl_linux_input_stop *stop=input->stop_original;
   unsigned same_stop=0;for(unsigned long j=0;j<cut->request_capacity;j++)if(stop==&cut->requests_original[j].input_stop)same_stop=1;
   if(input->owner_original!=request||input->mode_original!=mode||mode->input_original!=input||!input->start_returned||input->start_result||!input->create_entered||!input->create_returned||input->create_result||!same_stop||stop->worker_original!=input||
      !stop->attempted||!stop->send_completed||!stop->join_entered||!stop->join_returned||stop->join_result||stop->joined_result!=input||!stop->interpretation_returned||stop->interpretation_result||!atomic_load_explicit(&input->returned,memory_order_acquire)||
      (input->outcome!=SL_INPUT_STOPPED&&input->outcome!=SL_INPUT_EOF)){
    cut->failure_original=input;result=-EINVAL;goto finished;
   }
  }
  if(atomic_load_explicit(&mode->change.entered,memory_order_acquire)){
   sl_linux_mode_restore *restore=mode->restore_original;
   unsigned same_restore=0;for(unsigned long j=0;j<cut->request_capacity;j++)if(restore==&cut->requests_original[j].restoration)same_restore=1;
   if(mode->terminal_original!=&c->terminal||!atomic_load_explicit(&mode->change.returned,memory_order_acquire)||mode->change.result||!same_restore||restore->mode_original!=mode||
      !atomic_load_explicit(&restore->restore.returned,memory_order_acquire)||restore->restore.result){
    cut->failure_original=mode;result=-EINVAL;goto finished;
   }
  }
  cut->checked_requests++;
 }
 if(c->terminal.active_mode){cut->failure_original=c->terminal.active_mode;result=-EINVAL;goto finished;}
 for(unsigned long i=0;i<cut->reader_count;i++){
  sl_linux_capture_worker *reader=&cut->readers_original[i];sl_linux_forward_worker *forward=&cut->forwarders_original[i];
  sl_linux_reader_join *raw=reader->joined_by;sl_linux_forward_join *joined=forward->joined_by;
  if(!raw||!joined)goto finished;
  if(reader->controller_original!=c||raw->worker_original!=reader||!raw->returned||raw->result||!raw->interpretation_returned||raw->interpretation_result||raw->joined_result!=reader||reader->outcome!=SL_READER_READ_EOF){
   cut->failure_original=raw;result=-EINVAL;goto finished;
  }
  if(forward->reader_original!=reader||joined->worker_original!=forward||!joined->returned||joined->result||!joined->interpretation_returned||joined->interpretation_result||joined->joined_result!=forward||
     forward->result||forward->forwarding_result||!forward->release_original||forward->release_original->raw_join_original!=raw){
   cut->failure_original=joined;result=-EINVAL;goto finished;
  }
  cut->checked_readers++;
 }
 result=0;goto finished;
failed:
 if(result>=0)result=-EINVAL;
finished:
 cut->result=result;cut->returned=1;return result;
}
long sl_linux_drain_sources(sl_linux_public_controller *c){
 if(!c||!c->construction_returned||c->construction_result||c->birth_result<=0||c->drain_attempted)return -EINVAL;
 c->drain_attempted=1;c->drain_result=1;
 while(c->service_index<c->service_capacity){
  sl_linux_service_observation *o=sl_linux_service_next(c);
  if(!o){c->drain_result=-EINVAL;break;}
  c->drain_last=o;c->drain_result=o->source_cut_result;
  if(!c->drain_result){c->drained_by=&o->source_cut;break;}
  if(c->drain_result<0)break;
  if(!o->schedule_original||o->schedule_observe_result){c->drain_result=o->schedule_observe_result?o->schedule_observe_result:-EINVAL;break;}
  o->schedule_wait_result=sl_linux_schedule_block(o->schedule_original);
 }
 if(!c->drained_by&&c->service_index==c->service_capacity){c->drain_exhausted=1;if(c->drain_result>0)c->drain_result=-ENOSPC;}
 c->drain_returned=1;return c->drain_result;
 /* A held source cut is a FINAL input, never whole owner disposition. */
}
#endif
