/* UNRUN controlled FD-map source case: substitutes native observations and
 * shared mapping only; does not prove procfs/closed roster/native admission. */
#if defined(__linux__) && defined(__x86_64__)
#include "../owner_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <fcntl.h>
static sl_linux_fd_action shared_actions[32];static unsigned mappings;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 c->source=s;c->number=n;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);atomic_store(&c->entered,1);
 long result=-1;if(n==SYS_fcntl&&b==F_GETFD)result=a==13?FD_CLOEXEC:0;
 if(n==SYS_mmap){assert(b<=sizeof shared_actions);mappings++;result=(long)shared_actions;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../socket_peer_linux.c"
#include "../socket_state_linux.c"
#include "../owner_linux_entry.c"
int main(void){
 sl_linux_entry entry={0};sl_linux_bootstrap bootstrap={0};sl_linux_fd_map map={0};
 sl_linux_fd_view views[11]={0};sl_linux_fd_action actions[15]={0};
 int fds[11]={0,1,2,10,11,12,13,14,15,16,17};
 entry.birth.bootstrap=&bootstrap;entry.birth.payload_fd=13;entry.birth.entry_gate_fd=16;entry.custody_endpoints[1]=14;
 map.views=views;map.count=11;map.actions=actions;map.action_capacity=15;map.payload_fd=13;map.custody_fd=14;map.pty_slave_fd=15;
 for(unsigned i=0;i<11;i++){views[i].fd=fds[i];views[i].original=&views[i];}
 for(unsigned i=0;i<3;i++)map.standard_fds[i]=10+i;
 assert(sl_linux_build_child_fd_map(&entry,&map)==0&&mappings==1);
 unsigned unwanted=0;
 for(unsigned long i=0;i<bootstrap.fd_action_count;i++){
  assert(bootstrap.fd_actions[i].source_fd!=16);
  if(bootstrap.fd_actions[i].source_fd==17)unwanted++;
 }
 assert(unwanted==1&&bootstrap.controlling_terminal_fd==15);
 unsigned prior=mappings;assert(sl_linux_build_child_fd_map(&entry,&map)<0&&mappings==prior);
 /* Missing gate observation or a gate alias with payload denies before any
  * action backing acquisition; no fabricated close/dup is emitted. */
 for(unsigned mode=0;mode<2;mode++){
  memset(&entry,0,sizeof entry);memset(&bootstrap,0,sizeof bootstrap);memset(&map,0,sizeof map);memset(actions,0,sizeof actions);
  entry.birth.bootstrap=&bootstrap;entry.birth.payload_fd=13;entry.birth.entry_gate_fd=mode?13:99;entry.custody_endpoints[1]=14;
  map.views=views;map.count=11;map.actions=actions;map.action_capacity=15;map.payload_fd=13;map.custody_fd=14;map.pty_slave_fd=15;
  for(unsigned i=0;i<3;i++)map.standard_fds[i]=10+i;
  assert(sl_linux_build_child_fd_map(&entry,&map)<0&&mappings==prior&&!map.action_count);
 }
 return 0;
}
#endif
