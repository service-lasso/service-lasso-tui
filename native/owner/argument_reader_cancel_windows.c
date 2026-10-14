#ifdef _WIN32
#include "argument_reader_cancel_windows.h"
static int reader_cancel_zero(const void *p,SIZE_T n){const unsigned char *b=p;for(SIZE_T i=0;i<n;i++)if(b[i])return 0;return 1;}
static int reader_cancel_disjoint(const void *a,SIZE_T an,const void *b,SIZE_T bn){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;return x&&y&&an&&bn&&x<=~(ULONG_PTR)0-an&&y<=~(ULONG_PTR)0-bn&&!(x<y+bn&&y<x+an);}
static void reader_cancel_enter(sl_win_call *c,const char *s,void *p){c->source=s;c->subject=p;c->entered=1;}
static void reader_cancel_return(sl_win_call *c,ULONG_PTR r,DWORD e){c->result=r;c->last_error=e;c->returned=1;}
int sl_win_cancel_argument_reader(sl_win_argument_reader_cancel *o){
 sl_win_public_startup *s;const sl_win_debug_release *r;const sl_win_root_write *w;
 const sl_win_mapped_root_observation *m;const sl_win_debug_event_observation *e;
 sl_win_birth *b;sl_win_argument_send_sequence *q;BOOL ok;DWORD error;
 if(!o||o->attempted||o->returned||o->actual_startup||o->actual_release||o->actual_server||o->actual_primary||o->primary_id||o->wait_status||o->dispatch_stopped||o->disconnected||o->writer_result||o->result||!reader_cancel_zero(&o->writer_cancellation,sizeof(o->writer_cancellation))||
    !reader_cancel_zero(&o->disconnect,sizeof(o->disconnect))||!reader_cancel_zero(&o->thread_query,sizeof(o->thread_query))||!reader_cancel_zero(&o->thread_wait,sizeof(o->thread_wait))||!reader_cancel_zero(&o->cancel,sizeof(o->cancel)))return 0;
 s=o->startup_original;r=o->release_original;
 if(!s||!r||!reader_cancel_disjoint(o,sizeof(*o),s,sizeof(*s))||!sl_win_public_startup_factory_disjoint(s,o,sizeof(*o))||!reader_cancel_disjoint(o,sizeof(*o),r,sizeof(*r)))return 0;
 q=&s->argument_sequence;
 if(!q->actual_turns||!q->actual_turn_capacity||q->actual_turn_capacity>~(SIZE_T)0/sizeof(*q->actual_turns)||!reader_cancel_disjoint(o,sizeof(*o),q->actual_turns,q->actual_turn_capacity*sizeof(*q->actual_turns)))return 0;
 if(!s->argument_sender.span_count||s->argument_sender.span_count>4)return 0;
 for(SIZE_T i=0;i<s->argument_sender.span_count;i++)if(!reader_cancel_disjoint(o,sizeof(*o),s->argument_sender.spans[i].bytes,s->argument_sender.spans[i].extent))return 0;
 w=r->actual_write;if(!w||!reader_cancel_disjoint(o,sizeof(*o),w,sizeof(*w)))return 0;
 m=w->actual_before;if(!m||!w->actual_source||!reader_cancel_disjoint(o,sizeof(*o),m,sizeof(*m))||!reader_cancel_disjoint(o,sizeof(*o),w->actual_source,sizeof(*w->actual_source)))return 0;
 e=m->actual_event;if(!e||!reader_cancel_disjoint(o,sizeof(*o),e,sizeof(*e)))return 0;
 b=e->actual_birth;if(!b||!reader_cancel_disjoint(o,sizeof(*o),b,sizeof(*b)))return 0;
 o->attempted=1;o->actual_startup=s;o->actual_release=r;
 if(!s->returned||!s->result||!q->prepared||q->actual_sender!=&s->argument_sender||!r->returned||!r->result||!r->resume_primary.returned||r->previous_suspend_count!=1||b!=&s->controller.entry.birth)goto done;
 o->actual_server=s->duplex.server;o->actual_primary=b->process.hThread;
 if(!o->actual_server||o->actual_server==INVALID_HANDLE_VALUE||!o->actual_primary||o->actual_primary==INVALID_HANDLE_VALUE)goto done;
 /* Existing owner alone schedules this sequence. Do not consume live writer
  * outputs; stopping future dispatch is independent of native settlement. */
 q->cancelled=1;o->dispatch_stopped=1;
 reader_cancel_enter(&o->disconnect,"DisconnectNamedPipe.argument.original-D-failure",o->actual_server);
 ok=DisconnectNamedPipe(o->actual_server);error=GetLastError();reader_cancel_return(&o->disconnect,ok,error);o->disconnected=ok!=FALSE;
 reader_cancel_enter(&o->thread_query,"GetThreadId.argument.issuing-primary",o->actual_primary);
 o->primary_id=GetThreadId(o->actual_primary);error=GetLastError();reader_cancel_return(&o->thread_query,o->primary_id,error);
 if(!o->primary_id||o->primary_id!=b->process.dwThreadId)goto done;
 reader_cancel_enter(&o->thread_wait,"WaitForSingleObject.argument.issuing-primary.zero",o->actual_primary);
 o->wait_status=WaitForSingleObject(o->actual_primary,0);error=GetLastError();reader_cancel_return(&o->thread_wait,o->wait_status,error);
 if(o->wait_status!=WAIT_TIMEOUT)goto done;
 reader_cancel_enter(&o->cancel,"CancelSynchronousIo.argument.issuing-primary",o->actual_primary);
 ok=CancelSynchronousIo(o->actual_primary);error=GetLastError();reader_cancel_return(&o->cancel,ok,error);
 /* Result means only these two native requests reported success. Neither a
  * request nor ERROR_NOT_FOUND establishes any operation completion. */
 o->result=o->disconnected&&ok;
done:
 /* Preserve active-write cancellation even when disconnect/thread/request
  * observations failed. Existing sender path never consumes live outputs. */
 if(o->dispatch_stopped){
  o->writer_cancellation.sequence_original=q;
  o->writer_result=sl_win_cancel_argument_send(&o->writer_cancellation);
 }
 o->returned=1;return o->result;
}
#endif
