/* UNRUN synthetic parent-return/wait originals. No clone, pidfd or genuine
 * process terminal evidence; actual production observer body is exercised. */
#include "../outer_terminal_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/wait.h>
#include <errno.h>
static unsigned mode,calls;static sl_linux_outer_terminal *current;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 calls++;assert(c==&current->wait&&number==SYS_waitid&&a==P_PIDFD&&b==42&&
  d==(long)&current->information&&e==(WEXITED|WNOWAIT|WNOHANG)&&f==(long)&current->usage&&!g);
 c->source=source;c->number=number;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof(args));atomic_store(&c->entered,1);
 if(mode!=1){current->information.si_pid=mode==4?200:100;current->information.si_signo=mode==5?SIGUSR1:SIGCHLD;
  current->information.si_code=mode==2?CLD_KILLED:mode==3?CLD_DUMPED:mode==6?CLD_STOPPED:CLD_EXITED;}
 c->result=mode==7?-ECHILD:0;atomic_store(&c->returned,1);return c->result;
}
#include "../outer_terminal_linux.c"
int main(void){
 static struct {sl_linux_outer_birth birth;sl_linux_outer_terminal observation;} cases[14];
 for(mode=0;mode<14;mode++){
  sl_linux_outer_birth *b=&cases[mode].birth;sl_linux_outer_terminal *o=current=&cases[mode].observation;
  b->prepared=b->attempted=1;b->raw_parent=100;b->pidfd=42;atomic_store(&b->entered,1);atomic_store(&b->parent_captured,1);
  b->arguments.flags=CLONE_PIDFD;b->arguments.pidfd=(uint64_t)(uintptr_t)&b->pidfd;b->arguments.exit_signal=SIGCHLD;o->birth_original=b;
  if(mode==8)b->raw_parent=-ENOMEM; /* numeric pidfd output must not be adopted */
  if(mode==9)atomic_store(&b->parent_captured,0);
  if(mode==10)b->pidfd=-1;
  if(mode==11)atomic_store(&b->entered,0);
  if(mode==12)b->arguments.flags=0;
  if(mode==13)o->raw_wait=17;
  unsigned char original_birth[sizeof(*b)],original_output[sizeof(*o)];memcpy(original_birth,b,sizeof(*b));memcpy(original_output,o,sizeof(*o));
  unsigned before=calls;long result=sl_linux_observe_outer_terminal(o);
  assert(!memcmp(original_birth,b,sizeof(*b)));
  if(mode>=8){assert(result<0&&calls==before&&!memcmp(original_output,o,sizeof(*o)));continue;}
  assert(calls==before+1&&o->attempted&&o->returned&&o->actual_birth==b);
  if(mode==0||mode==2||mode==3)assert(!result&&o->terminal);
  else if(mode==1)assert(result==1&&!o->terminal&&!o->information.si_pid);
  else assert(result<0&&!o->terminal);
  unsigned char saved[sizeof(*o)];memcpy(saved,o,sizeof(saved));before=calls;
  assert(sl_linux_observe_outer_terminal(o)<0&&calls==before&&!memcmp(saved,o,sizeof(saved)));
 }
 return 0;
}
