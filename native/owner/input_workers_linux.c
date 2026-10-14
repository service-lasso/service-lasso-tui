#if defined(__linux__) && defined(__x86_64__)
#include "schedule_linux.h"
#include "input_workers_linux.h"
#include <sys/syscall.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
_Static_assert(sizeof(struct pollfd)==8,"Linux pollfd ABI");
static sl_linux_input_step *sl_input_step(sl_linux_input_frame *frame,unsigned kind){
 sl_linux_input_step *step;if(frame->count>=frame->prepared_capacity)return 0;
 step=&frame->prepared_steps[frame->count++];step->kind=kind;return step;
}
static long sl_input_wait(sl_linux_input_worker *worker,sl_linux_input_step *step,int fd,short events){
 long result;
 step->readiness[0]=(struct pollfd){.fd=fd,.events=events};
 step->readiness[1]=(struct pollfd){.fd=worker->control[0],.events=POLLIN};
 result=sl_linux_call6(&step->call,"input.poll.source-and-stop",SYS_poll,(long)step->readiness,2,-1,0,0,0);
 if(result<=0)return -1;
 if(step->readiness[1].revents&POLLIN)return 1;
 if(step->readiness[1].revents&(POLLERR|POLLHUP|POLLNVAL))return -1;
 if(step->readiness[0].revents&(events|POLLHUP))return 0;
 return -1;
}
static void *sl_linux_terminal_input(void *argument){
 sl_linux_input_worker *worker=argument;unsigned long index;long result;
 atomic_store_explicit(&worker->entered,1,memory_order_release);
 for(index=0;index<worker->prepared_capacity;index++){
  sl_linux_input_frame *frame=&worker->prepared_frames[index];sl_linux_input_step *step;
  worker->count=index+1;step=sl_input_step(frame,1);if(!step)goto exhausted;
  result=sl_input_wait(worker,step,worker->input_fd,POLLIN);
  if(result==1)goto stopped;if(result<0)goto failed;
  step=sl_input_step(frame,2);if(!step)goto exhausted;
  frame->raw_count=sl_linux_call6(&step->call,"input.public.read",SYS_read,worker->input_fd,(long)frame->bytes,sizeof(frame->bytes),0,0,0);
  if(frame->raw_count<0||(unsigned long)frame->raw_count>sizeof(frame->bytes))goto failed;
  if(!frame->raw_count){worker->outcome=SL_INPUT_EOF;goto finished;}
  while(frame->written<(unsigned long)frame->raw_count){
   step=sl_input_step(frame,3);if(!step)goto exhausted;
   result=sl_input_wait(worker,step,worker->master_fd,POLLOUT);
   if(result==1)goto stopped;if(result<0)goto failed;
   step=sl_input_step(frame,4);if(!step)goto exhausted;
   result=sl_linux_call6(&step->call,"input.private-master.write",SYS_write,worker->master_fd,(long)(frame->bytes+frame->written),(unsigned long)frame->raw_count-frame->written,0,0,0);
   if(result==-EAGAIN)continue; /* fresh no-progress observation, never reused */
   if(result<=0||(unsigned long)result>(unsigned long)frame->raw_count-frame->written)goto failed;
   frame->written+=(unsigned long)result;
  }
 }
exhausted:worker->outcome=SL_INPUT_EXHAUSTED;goto finished;
stopped:worker->outcome=SL_INPUT_STOPPED;goto finished;
failed:worker->outcome=SL_INPUT_FAILURE;
finished:worker->worker_result=worker;atomic_store_explicit(&worker->returned,1,memory_order_release);if(worker->terminal_wake)sl_linux_schedule_notify(worker->terminal_wake);return worker;
}
long sl_linux_prepare_terminal_input(sl_linux_input_worker *worker){
 unsigned long index,other,step_index;uintptr_t start,end;
 if(!worker||worker->prepare_attempted||!worker->owner_original)return -1;
 worker->prepare_attempted=1;
 if(!worker->frames||!worker->capacity||worker->capacity>UINTPTR_MAX/sizeof(*worker->frames)||
    worker->capacity*sizeof(*worker->frames)>UINTPTR_MAX-(uintptr_t)worker->frames)return -1;
 start=(uintptr_t)worker->frames;end=start+worker->capacity*sizeof(*worker->frames);
 if((uintptr_t)worker>UINTPTR_MAX-sizeof(*worker)||(start<(uintptr_t)worker+sizeof(*worker)&&(uintptr_t)worker<end))return -1;
 for(index=0;index<worker->capacity;index++){
  sl_linux_input_frame *frame=&worker->frames[index];uintptr_t a=(uintptr_t)frame->steps,b;
  if(!frame->steps||!frame->capacity||frame->count||frame->written||frame->raw_count||frame->capacity>UINTPTR_MAX/sizeof(*frame->steps)||frame->capacity*sizeof(*frame->steps)>UINTPTR_MAX-a)return -1;
  b=a+frame->capacity*sizeof(*frame->steps);
  if(a<end&&start<b)return -1;
  if(a<(uintptr_t)worker+sizeof(*worker)&&(uintptr_t)worker<b)return -1;
  for(other=0;other<index;other++){uintptr_t c=(uintptr_t)worker->frames[other].steps,d=c+worker->frames[other].capacity*sizeof(*frame->steps);if(a<d&&c<b)return -1;}
  frame->prepared_steps=frame->steps;frame->prepared_capacity=frame->capacity;
  for(step_index=0;step_index<frame->capacity;step_index++){atomic_init(&frame->steps[step_index].call.entered,0);atomic_init(&frame->steps[step_index].call.returned,0);}
 }
 atomic_init(&worker->control_pair.entered,0);atomic_init(&worker->control_pair.returned,0);
 atomic_init(&worker->entered,0);atomic_init(&worker->returned,0);
 worker->prepared_frames=worker->frames;worker->prepared_capacity=worker->capacity;
 worker->prepared=1;return 0;
}
static long sl_linux_start_terminal_input_enter(sl_linux_input_worker *worker,sl_linux_mode_episode *mode,int master_fd,const sl_linux_call *nonblocking){
 long result;unsigned index;
 if(!worker||!worker->prepared||!mode||!mode->terminal_original||master_fd<3)return -1;
 worker->master_nonblocking_original=nonblocking;
 if(!nonblocking||!atomic_load_explicit(&nonblocking->returned,memory_order_acquire)||nonblocking->result!=0||nonblocking->number!=SYS_fcntl||nonblocking->arguments[0]!=master_fd||nonblocking->arguments[1]!=F_SETFL||!(nonblocking->arguments[2]&O_NONBLOCK))return -1;
 worker->mode_original=mode;worker->master_fd=master_fd;worker->input_fd=mode->input_fd;
 if(worker->input_fd<3||mode->input_original||!atomic_load_explicit(&mode->change.returned,memory_order_acquire)||mode->change.result!=0||mode->terminal_original->active_mode!=mode)return -1;
 mode->input_original=worker;worker->raw_control[0]=-1;worker->raw_control[1]=-1;worker->control[0]=-1;worker->control[1]=-1;
 result=sl_linux_call6(&worker->control_pair,"input.stop.socketpair",SYS_socketpair,AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0,(long)worker->raw_control,0,0);
 if(result!=0)return result<0?result:-1;
 for(index=0;index<2;index++){result=sl_linux_lift_internal_fd(&worker->control_lifts[index],worker->raw_control[index]);if(result<0)return result;worker->control[index]=(int)result;}
 worker->create_entered=1;worker->create_result=pthread_create(&worker->thread,0,sl_linux_terminal_input,worker);worker->create_returned=1;
 return worker->create_result?-1:0;
}
long sl_linux_start_terminal_input(sl_linux_input_worker *worker,sl_linux_mode_episode *mode,int master_fd,const sl_linux_call *nonblocking){
 long result;if(!worker||worker->start_attempted)return -1;
 worker->start_attempted=1;worker->mode_original=mode;worker->master_fd=master_fd;worker->master_nonblocking_original=nonblocking;
 result=sl_linux_start_terminal_input_enter(worker,mode,master_fd,nonblocking);
 worker->start_result=result;worker->start_returned=1;return result;
}
static long stop_join_terminal_input_body(sl_linux_input_stop *stop,sl_linux_input_worker *worker){
 long result;
 if(!stop||stop->attempted||!worker||worker->stop_original)return -1;
 stop->attempted=1;stop->worker_original=worker;worker->stop_original=stop;
 if(!worker->create_returned){if(worker->start_returned&&!worker->create_entered)return 0;return -1;}
 if(worker->create_result)return 0; /* authentic pthread rejection: no worker */
 stop->byte=1;atomic_init(&stop->send.entered,0);atomic_init(&stop->send.returned,0);
 result=sl_linux_call6(&stop->send,"input.stop.send-once",SYS_sendto,worker->control[1],(long)&stop->byte,1,MSG_NOSIGNAL,0,0);
 if(result!=1)return result<0?result:-1;stop->send_completed=1;
 /* Channel worker is independent of the live public deadline/controller. */
 stop->join_entered=1;stop->join_result=pthread_join(worker->thread,&stop->joined_result);stop->join_returned=1;
 if(stop->join_result||stop->joined_result!=worker||!atomic_load_explicit(&worker->returned,memory_order_acquire))return -1;
 if(worker->terminal_wake){long result=sl_linux_schedule_joined_notification(worker->terminal_wake,worker);if(result)return result;}
 return 0;
}
long sl_linux_stop_join_terminal_input(sl_linux_input_stop *stop,sl_linux_input_worker *worker){
 if(!stop||stop->attempted||stop->interpretation_returned)return -1;
 long result=stop_join_terminal_input_body(stop,worker);
 stop->interpretation_result=result;stop->interpretation_returned=1;return result;
}
#endif
