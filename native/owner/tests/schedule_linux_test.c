/* Source-only controlled syscall fixture; UNRUN. Not native futex evidence. */
#include <assert.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include "../schedule_linux.h"
static unsigned calls;static long last_operation,last_expected;
long sl_linux_call6(sl_linux_call *o,const char *name,long number,long a,long b,long c,long d,long e,long f){
 (void)a;(void)d;(void)e;(void)f;assert(number==SYS_futex);o->source=name;o->number=number;
 atomic_store(&o->entered,1);calls++;last_operation=b;last_expected=c;
 o->result=b==FUTEX_WAIT_PRIVATE?-EAGAIN:1;atomic_store(&o->returned,1);return o->result;
}
#include "../schedule_linux.c"
int main(void){
 sl_linux_schedule root={0};sl_linux_schedule_wake wake[1]={0};sl_linux_schedule_wait wait[2]={0};int controller,source;
 root.controller_original=(struct sl_linux_public_controller*)&controller;root.wakes=wake;root.wake_capacity=1;root.waits=wait;root.wait_capacity=2;root.interval_ns=1;
 wake[0].schedule_original=&root;wake[0].source_original=&source;
 wait[0].schedule_original=&root;wait[1].schedule_original=&root;
 assert(!sl_linux_prepare_schedule(&root));assert(sl_linux_schedule_joined_notification(&wake[0],&source)==-EINVAL);assert(!sl_linux_schedule_observe(&wait[0]));assert(wait[0].generation_before==0);
 assert(sl_linux_schedule_notify(&wake[0])==1);assert(atomic_load(&wake[0].returned));assert(root.generation==1);assert(!sl_linux_schedule_joined_notification(&wake[0],&source));assert(sl_linux_schedule_joined_notification(&wake[0],&controller)==-EINVAL);
 assert(sl_linux_schedule_block(&wait[0])==-EAGAIN);assert(last_operation==FUTEX_WAIT_PRIVATE&&last_expected==0);assert(wait[0].generation_after==1);
 unsigned before=calls;assert(sl_linux_schedule_notify(&wake[0])==-EINVAL);assert(sl_linux_schedule_block(&wait[0])==-EINVAL);assert(calls==before);
 assert(!sl_linux_schedule_observe(&wait[1]));assert(wait[1].generation_before==1);assert(sl_linux_schedule_block(&wait[1])==-EAGAIN);assert(last_expected==1);
 return 0;
}