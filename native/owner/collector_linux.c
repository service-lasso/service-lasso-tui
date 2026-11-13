#if defined(__linux__) && defined(__x86_64__)
#include "public_controller_linux.h"
/* Native join completion is distinct from the published channel-body return.
 * Every invocation owns a fresh row; EBUSY never consumes thread ownership. */
static int collector_join_body(sl_linux_collector_join *observation) {
 sl_linux_public_controller *controller;
 if(!observation||observation->attempted||!observation->controller_original)return -1;
 observation->attempted=1;controller=observation->controller_original;
 if(!controller->collector_create_returned||controller->collector_create_result||controller->collector_joined_by){
  observation->interpretation_result=-1;return -1;
 }
 observation->result=pthread_tryjoin_np(controller->collector_thread,&observation->joined_result);
 observation->returned=1;
 if(observation->result){observation->interpretation_result=observation->result;return observation->result;}
 /* Actual successful native join is absorbing even if body/return validation
  * fails. Never retry pthread_join on this same original native thread. */
 controller->collector_joined_by=observation;
 if(observation->joined_result!=&controller->collection||
    !atomic_load_explicit(&controller->collector_returned,memory_order_acquire)){
  observation->interpretation_result=-1;return -1;
 }
 if(controller->collector_wake){long result=sl_linux_schedule_joined_notification(controller->collector_wake,&controller->channel);if(result){observation->interpretation_result=result;return result;}}
 observation->channel_original=&controller->channel;
 observation->collection_original=&controller->collection;
 observation->channel_result=controller->channel.result;
 observation->collection_result=controller->collection.result;
 /* The body returns this address even on channel failure. A native join is
  * not a successful LOCAL representation exchange or custody authorization. */
 if(!controller->channel.returned){observation->interpretation_result=-1;return -1;}
 if(controller->channel.result<0){observation->interpretation_result=controller->channel.result;return controller->channel.result;}
 if(controller->channel.result!=8||!controller->channel.local_prefix_original||
    controller->channel.prepared_collection!=&controller->collection||
    !controller->collection.returned||controller->collection.result!=8||
    !controller->collection.readback_equal||
    controller->collection.header_original!=controller->channel.local_prefix_original){
  observation->interpretation_result=-1;return -1;
 }
 observation->interpretation_result=0;return 0;
}
/* Actual synchronous interpreter frontier, distinct from native pthread
 * returned. No duplicate entry changes either prior result or frontier. */
int sl_linux_observe_collector_join(sl_linux_collector_join *observation){
 if(!observation||observation->attempted||observation->interpretation_returned||!observation->controller_original)return -1;
 int result=collector_join_body(observation);
 observation->interpretation_returned=1;
 return result;
}
#endif