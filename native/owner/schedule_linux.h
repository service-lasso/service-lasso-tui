#ifndef SERVICE_LASSO_SCHEDULE_LINUX_H
#define SERVICE_LASSO_SCHEDULE_LINUX_H
#include "owner_linux.h"
#include <time.h>
struct sl_linux_public_controller;
struct sl_linux_schedule;
typedef struct sl_linux_schedule_wake {
 struct sl_linux_schedule *schedule_original;void *source_original;
 _Atomic unsigned claimed,returned;
 uint32_t generation_before,generation_after;sl_linux_call call;long result;
} sl_linux_schedule_wake;
typedef struct sl_linux_schedule_wait {
 struct sl_linux_schedule *schedule_original;_Atomic unsigned claimed;
 uint32_t generation_before,generation_after;sl_linux_call call;long result;
 unsigned observed,returned;
} sl_linux_schedule_wait;
typedef struct sl_linux_schedule {
 struct sl_linux_public_controller *controller_original;
 _Atomic uint32_t generation;
 sl_linux_schedule_wake *wakes,*actual_wakes;unsigned long wake_capacity,actual_wake_capacity;
 sl_linux_schedule_wait *waits,*actual_waits;unsigned long wait_capacity,actual_wait_capacity;
 unsigned long interval_ns,actual_interval_ns;struct timespec interval;
 unsigned prepare_attempted,prepared;
} sl_linux_schedule;
long sl_linux_prepare_schedule(sl_linux_schedule *root);
long sl_linux_schedule_notify(sl_linux_schedule_wake *root);
long sl_linux_schedule_joined_notification(sl_linux_schedule_wake *root,const void *source);
long sl_linux_schedule_observe(sl_linux_schedule_wait *root);
long sl_linux_schedule_block(sl_linux_schedule_wait *root);
#endif