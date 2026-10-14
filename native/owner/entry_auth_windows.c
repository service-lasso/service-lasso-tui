#ifdef _WIN32
#include "entry_auth_windows.h"

static int entry_call_fresh(const sl_win_call *c){return !c->source&&!c->subject&&!c->result&&!c->last_error&&!c->entered&&!c->returned;}
static void entry_enter(sl_win_call *c,const char *source,void *subject){c->source=source;c->subject=subject;c->entered=1;}
static void entry_return(sl_win_call *c,BOOL result,DWORD error){c->result=(ULONG_PTR)result;c->last_error=error;c->returned=1;}

int sl_win_observe_entry_endpoint(sl_win_endpoint_observation *o){
 BOOL ok;DWORD error;ULONG_PTR begin,owner;
 if(!o||o->attempted||o->returned||o->actual_endpoint||o->server_process_id||o->flags||o->outbound_bytes||o->inbound_bytes||o->maximum_instances||o->result||
    !entry_call_fresh(&o->server_process_query)||!entry_call_fresh(&o->information_query)||
    o->security.attempted||o->security.returned||o->security.actual_backing||o->security.actual_capacity||o->security.object_original||
    o->security.required||o->security.requested||o->security.result||!entry_call_fresh(&o->security.query)||
    !o->security.backing||!o->security.capacity)return 0;
 begin=(ULONG_PTR)o->security.backing;owner=(ULONG_PTR)o;
 if(begin%__alignof(DWORD)||begin>~(ULONG_PTR)0-o->security.capacity||owner>~(ULONG_PTR)0-sizeof(*o)||
    (begin<owner+sizeof(*o)&&owner<begin+o->security.capacity))return 0;
 o->attempted=1;o->actual_endpoint=o->endpoint_original;
 if(!o->actual_endpoint||o->actual_endpoint==INVALID_HANDLE_VALUE)goto done;
 entry_enter(&o->server_process_query,"GetNamedPipeServerProcessId.entry.same-client",o->actual_endpoint);
 ok=GetNamedPipeServerProcessId(o->actual_endpoint,&o->server_process_id);error=GetLastError();entry_return(&o->server_process_query,ok,error);
 if(!ok)goto done;
 entry_enter(&o->information_query,"GetNamedPipeInfo.entry.same-client",o->actual_endpoint);
 ok=GetNamedPipeInfo(o->actual_endpoint,&o->flags,&o->outbound_bytes,&o->inbound_bytes,&o->maximum_instances);
 error=GetLastError();entry_return(&o->information_query,ok,error);
 if(!ok)goto done;
 if(!sl_win_observe_object_security(&o->security,o->actual_endpoint))goto done;
 /* Query success retains raw descriptor/flags, not a ready bit. Exact client,
  * byte mode, single instance, rights/principal and creator conjunction are
  * independently interpreted from these SAME originals by their owner. */
 o->result=1;
done:o->returned=1;return o->result;
}
#endif
