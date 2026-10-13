/* UNRUN controlled syscall regression with actual pthread scheduling/join.
 * Link input_workers_linux.c with this fixture and pthread. No real terminal
 * IO/birth/native authority: the syscall function is explicitly substituted. */
#if defined(__linux__) && defined(__x86_64__)
#include "../input_workers_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <fcntl.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static unsigned write_seen,stop_sent,reads,writes,sends;
static long socket_failure,lift_failure;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 call->source=source;call->number=number;call->arguments[0]=a0;call->arguments[1]=a1;call->arguments[2]=a2;call->arguments[3]=a3;call->arguments[4]=a4;call->arguments[5]=a5;
 atomic_store(&call->entered,1);
 if(number==SYS_socketpair){if(socket_failure)call->result=socket_failure;else{int *fds=(int*)a3;fds[0]=70;fds[1]=71;call->result=0;}}
 else if(number==SYS_poll){
  struct pollfd *fds=(struct pollfd*)a0;assert(a1==2);pthread_mutex_lock(&lock);
  if(fds[0].fd==11&&write_seen){while(!stop_sent)pthread_cond_wait(&changed,&lock);}
  if(stop_sent){fds[1].revents=POLLIN;fds[0].revents=0;}else{fds[0].revents=fds[0].events;fds[1].revents=0;}
  pthread_mutex_unlock(&lock);call->result=1;
 }
 else if(number==SYS_read){assert(a0==99);((unsigned char*)a1)[0]='q';reads++;call->result=1;}
 else if(number==SYS_write){assert(a0==11&&a2==1);pthread_mutex_lock(&lock);writes++;write_seen=1;pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);call->result=-EAGAIN;}
 else if(number==SYS_sendto){assert(a0==71&&a2==1&&a3==MSG_NOSIGNAL);pthread_mutex_lock(&lock);sends++;stop_sent=1;pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);call->result=1;}
 else{assert(0);call->result=-1;}
 atomic_store(&call->returned,1);return call->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->original_fd=fd;lift->held_fd=lift_failure?-1:fd;lift->attempted=1;return lift_failure?lift_failure:fd;}
static void check_prelaunch_failure(long socket_error,long lift_error){
 sl_linux_input_worker worker={0};sl_linux_input_frame frames[1]={0};sl_linux_input_step steps[1]={0};
 sl_linux_input_stop stop={0};sl_linux_mode_episode mode={0};sl_linux_terminal_views terminal={0};sl_linux_call nonblocking={0};int owner;
 worker.owner_original=&owner;worker.frames=frames;worker.capacity=1;frames[0].steps=steps;frames[0].capacity=1;
 assert(sl_linux_prepare_terminal_input(&worker)==0);
 mode.terminal_original=&terminal;mode.input_fd=99;terminal.active_mode=&mode;atomic_init(&mode.change.returned,1);mode.change.result=0;
 nonblocking.number=SYS_fcntl;nonblocking.arguments[0]=11;nonblocking.arguments[1]=F_SETFL;nonblocking.arguments[2]=O_NONBLOCK;atomic_init(&nonblocking.returned,1);
 socket_failure=socket_error;lift_failure=lift_error;
 assert(sl_linux_start_terminal_input(&worker,&mode,11,&nonblocking)==(socket_error?socket_error:lift_error));
 assert(worker.start_returned&&!worker.create_entered&&!worker.create_returned&&mode.input_original==&worker);
 assert(sl_linux_stop_join_terminal_input(&stop,&worker)==0&&!stop.join_entered&&!stop.send_completed);
 assert(stop.interpretation_returned&&!stop.interpretation_result);
 assert(worker.control_pair.result==socket_error&&worker.stop_original==&stop);
 socket_failure=0;lift_failure=0;
}
int main(void){
 check_prelaunch_failure(-EACCES,0);check_prelaunch_failure(0,-EIO);
 sl_linux_input_worker worker={0};sl_linux_input_frame frames[1]={0};sl_linux_input_step steps[5]={0};
 sl_linux_input_stop stop={0},repeat={0};sl_linux_mode_episode mode={0};sl_linux_terminal_views terminal={0};sl_linux_call nonblocking={0};int owner;
 worker.owner_original=&owner;worker.frames=frames;worker.capacity=1;frames[0].steps=steps;frames[0].capacity=5;
 assert(sl_linux_prepare_terminal_input(&worker)==0);
 /* Changing exported proposal fields cannot change the prepared backing. */
 worker.frames=0;worker.capacity=0;frames[0].steps=0;frames[0].capacity=0;
 mode.terminal_original=&terminal;mode.input_fd=99;terminal.active_mode=&mode;atomic_init(&mode.change.returned,1);mode.change.result=0;
 nonblocking.number=SYS_fcntl;nonblocking.arguments[0]=11;nonblocking.arguments[1]=F_SETFL;nonblocking.arguments[2]=O_NONBLOCK;nonblocking.result=0;atomic_init(&nonblocking.returned,1);
 assert(sl_linux_start_terminal_input(&worker,&mode,11,&nonblocking)==0&&worker.start_returned);
 pthread_mutex_lock(&lock);while(!write_seen)pthread_cond_wait(&changed,&lock);pthread_mutex_unlock(&lock);
 assert(sl_linux_stop_join_terminal_input(&stop,&worker)==0);
 assert(stop.join_returned&&!stop.join_result&&stop.joined_result==&worker&&worker.outcome==SL_INPUT_STOPPED);
 assert(stop.interpretation_returned&&!stop.interpretation_result);
 assert(sl_linux_stop_join_terminal_input(&stop,&worker)==-1&&stop.interpretation_returned&&!stop.interpretation_result&&sends==1);
 assert(frames[0].raw_count==1&&frames[0].written==0&&frames[0].bytes[0]=='q'&&frames[0].count==5);
 assert(steps[3].call.result==-EAGAIN&&atomic_load(&steps[3].call.returned));
 assert(reads==1&&writes==1&&sends==1&&mode.input_original==&worker&&worker.stop_original==&stop);
 assert(sl_linux_stop_join_terminal_input(&repeat,&worker)==-1&&sends==1);
 assert(repeat.interpretation_returned&&repeat.interpretation_result==-1&&worker.stop_original==&stop);
 return 0;
}
#endif
