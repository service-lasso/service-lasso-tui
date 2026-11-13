#if defined(__linux__) && defined(__x86_64__)
#include "stream_routes_linux.h"
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
long sl_linux_prepare_private_output(sl_linux_call *query,sl_linux_call *setting,struct termios *state,struct termios *derived,int slave){
 long result;
 if (!query||!setting||!state||!derived||slave<3) return -1;
 atomic_init(&query->entered,0);atomic_init(&query->returned,0);
 atomic_init(&setting->entered,0);atomic_init(&setting->returned,0);
 result=sl_linux_call6(query,"stream.private.pty.tcgets",SYS_ioctl,slave,TCGETS,(long)state,0,0,0);
 if(result<0)return result;
 *derived=*state;derived->c_oflag&=~OPOST;
 return sl_linux_call6(setting,"stream.private.pty.output-untranslated",SYS_ioctl,slave,TCSETS,(long)derived,0,0,0);
}
long sl_linux_construct_stream_route(sl_linux_stream_route *route,int controlling_master,int controlling_slave){
 long result;int index;
 if(!route||route->attempted||!route->public_original||route->stream_index<0||route->stream_index>2)return -1;
 route->attempted=1;route->owner_fd=-1;route->child_fd=-1;
 route->raw_endpoints[0]=-1;route->raw_endpoints[1]=-1;
 atomic_init(&route->query.entered,0);atomic_init(&route->query.returned,0);
 atomic_init(&route->pipe.entered,0);atomic_init(&route->pipe.returned,0);
 atomic_init(&route->pty_open.entered,0);atomic_init(&route->pty_open.returned,0);
 atomic_init(&route->pty_unlock.entered,0);atomic_init(&route->pty_unlock.returned,0);
 atomic_init(&route->pty_peer.entered,0);atomic_init(&route->pty_peer.returned,0);
 atomic_init(&route->close_child.entered,0);atomic_init(&route->close_child.returned,0);
 if(!atomic_load_explicit(&route->public_original->original_flags.returned,memory_order_acquire))return -1;
 route->observed_absent=route->public_original->observed_absent;
 if(route->observed_absent)return route->public_original->original_flags.result==-EBADF?0:-1;
 if(!atomic_load_explicit(&route->public_original->duplicate.returned,memory_order_acquire)||route->public_original->held_fd<3||route->public_original->duplicate.result!=route->public_original->held_fd)return -1;
 result=sl_linux_call6(&route->query,"stream.public.capability.tcgets",SYS_ioctl,route->public_original->held_fd,TCGETS,(long)&route->public_state,0,0,0);
 if(result!=0&&result!=-ENOTTY)return result<0?result:-1;
 route->is_terminal=result==0;
 if(route->is_terminal&&route->stream_index<2){
  if(controlling_master<3||controlling_slave<3)return -1;
  route->shares_controlling=1;route->owner_fd=controlling_master;route->child_fd=controlling_slave;return 0;
 }
 if(!route->is_terminal){
  result=sl_linux_call6(&route->pipe,"stream.private.pipe",SYS_pipe2,(long)route->raw_endpoints,O_CLOEXEC,0,0,0,0);
  if(result<0)return result;
  for(index=0;index<2;index++){result=sl_linux_lift_internal_fd(&route->lifts[index],route->raw_endpoints[index]);if(result<0)return result;}
  if(route->stream_index==0){route->child_fd=route->lifts[0].held_fd;route->owner_fd=route->lifts[1].held_fd;}
  else{route->owner_fd=route->lifts[0].held_fd;route->child_fd=route->lifts[1].held_fd;}
  return 0;
 }
 result=sl_linux_call6(&route->pty_open,"stream.stderr.pty.open",SYS_openat,AT_FDCWD,(long)"/dev/ptmx",O_RDWR|O_NOCTTY|O_CLOEXEC,0,0,0);
 if(result<0||result>2147483647L)return result<0?result:-1;
 route->raw_endpoints[0]=(int)result;
 result=sl_linux_lift_internal_fd(&route->lifts[0],route->raw_endpoints[0]);if(result<0)return result;route->owner_fd=(int)result;
 route->unlock_value=0;
 result=sl_linux_call6(&route->pty_unlock,"stream.stderr.pty.unlock",SYS_ioctl,route->owner_fd,TIOCSPTLCK,(long)&route->unlock_value,0,0,0);if(result<0)return result;
 result=sl_linux_call6(&route->pty_peer,"stream.stderr.pty.peer",SYS_ioctl,route->owner_fd,TIOCGPTPEER,O_RDWR|O_NOCTTY|O_CLOEXEC,0,0,0);
 if(result<0||result>2147483647L)return result<0?result:-1;
 route->raw_endpoints[1]=(int)result;
 result=sl_linux_lift_internal_fd(&route->lifts[1],route->raw_endpoints[1]);if(result<0)return result;route->child_fd=(int)result;
 return sl_linux_prepare_private_output(&route->private_query,&route->private_set,&route->private_state,&route->private_output_state,route->child_fd);
}
long sl_linux_close_parent_child_view(sl_linux_stream_route *route){
 if(!route||!route->attempted||route->close_attempted||route->child_fd<3)return -1;
 route->close_attempted=1;
 return sl_linux_call6(&route->close_child,"stream.parent.child-view.close-once",SYS_close,route->child_fd,0,0,0,0,0);
}
#endif