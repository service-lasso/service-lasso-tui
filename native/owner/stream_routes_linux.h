#ifndef SERVICE_LASSO_STREAM_ROUTES_LINUX_H
#define SERVICE_LASSO_STREAM_ROUTES_LINUX_H
#include "terminal_linux.h"
typedef struct sl_linux_stream_route {
 const sl_linux_public_stream *public_original;
 sl_linux_call query,pipe,pty_open,pty_unlock,pty_peer,private_query,private_set,close_child;
 struct termios public_state,private_state,private_output_state;
 sl_linux_fd_lift lifts[2];
 int raw_endpoints[2],owner_fd,child_fd,unlock_value,stream_index;
 unsigned attempted,is_terminal,shares_controlling,observed_absent,close_attempted;
} sl_linux_stream_route;
long sl_linux_prepare_private_output(sl_linux_call *query,sl_linux_call *setting,struct termios *state,struct termios *derived,int slave);
long sl_linux_construct_stream_route(sl_linux_stream_route *route,int controlling_master,int controlling_slave);
long sl_linux_close_parent_child_view(sl_linux_stream_route *route);
#endif