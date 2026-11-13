#ifdef _WIN32
#include "argument_sender_windows.h"
_Static_assert(offsetof(sl_win_argument_write,cancellation_requested)%sizeof(LONG)==0,"interlocked cancellation alignment");

static int sender_zero(const void *p,SIZE_T n){const unsigned char *s=p;for(SIZE_T i=0;i<n;i++)if(s[i])return 0;return 1;}
static int sender_extent(const void *p,SIZE_T n,ULONG_PTR *begin,ULONG_PTR *end){
 ULONG_PTR x=(ULONG_PTR)p;if(!x||!n||x>~(ULONG_PTR)0-n)return 0;*begin=x;*end=x+n;return 1;
}
static int sender_disjoint(const void *a,SIZE_T an,const void *b,SIZE_T bn){
 ULONG_PTR x,xe,y,ye;if(!sender_extent(a,an,&x,&xe)||!sender_extent(b,bn,&y,&ye))return 0;return !(x<ye&&y<xe);
}
static int sender_add(sl_win_argument_sender *o,const void *p,SIZE_T bytes,unsigned part){
 ULONG_PTR begin,end;SIZE_T offset=0;
 if(!sender_extent(p,bytes,&begin,&end))return 0;
 while(bytes){
  DWORD extent=bytes>MAXDWORD?MAXDWORD:(DWORD)bytes;
  sl_win_argument_span *span;
  if(o->span_count>=sizeof(o->spans)/sizeof(o->spans[0]))return 0;
  span=&o->spans[o->span_count++];span->bytes=(const unsigned char *)(begin+offset);
  span->extent=extent;span->source_part=part;span->source_offset=offset;
  offset+=extent;bytes-=extent;
 }
 return 1;
}
int sl_win_prepare_argument_sender(sl_win_argument_sender *o){
 const sl_win_private_arguments *a;const sl_win_duplex *d;SIZE_T raw_bytes,module_bytes;
 if(!o||o->attempted||o->returned||o->actual_arguments||o->actual_duplex||o->actual_server||o->span_count||o->body_extent||o->result||!sender_zero(o->spans,sizeof(o->spans)))return 0;
 a=o->arguments_original;d=o->duplex_original;
 if(!a||!d||!sender_disjoint(o,sizeof(*o),a,sizeof(*a))||!sender_disjoint(o,sizeof(*o),d,sizeof(*d))||!sender_disjoint(a,sizeof(*a),d,sizeof(*d)))return 0;
 if(a->transport_raw_units>~(SIZE_T)0/sizeof(WCHAR)||a->transport_module_units>~(SIZE_T)0/sizeof(WCHAR))return 0;
 if(a->transport_raw_units&&!sender_disjoint(o,sizeof(*o),a->transport_raw,a->transport_raw_units*sizeof(WCHAR)))return 0;
 if(a->transport_module_units&&!sender_disjoint(o,sizeof(*o),a->transport_module,a->transport_module_units*sizeof(WCHAR)))return 0;
 o->attempted=1;o->actual_arguments=o->arguments_original;o->actual_duplex=o->duplex_original;
 a=o->actual_arguments;d=o->actual_duplex;
 if(!a||!d||!a->attempted||!a->returned||a->result||!a->transport_prepared||!d->attempted||!d->returned||!d->connected||!d->server||d->server==INVALID_HANDLE_VALUE||!d->inherited_client||a->endpoint_original!=d->inherited_client)goto done;
 // These bounds come from the actual original GetCommandLine and DWORD module
 // query constructor domains, not an invented module path or memory budget.
 if(!a->transport_raw||!a->transport_raw_units||a->transport_raw_units>32767||a->transport_module_units>MAXDWORD||a->transport_raw_units>~(SIZE_T)0/sizeof(WCHAR)||a->transport_module_units>~(SIZE_T)0/sizeof(WCHAR))goto done;
 raw_bytes=a->transport_raw_units*sizeof(WCHAR);module_bytes=a->transport_module_units*sizeof(WCHAR);
 if(raw_bytes>~(SIZE_T)0-module_bytes||a->transport_body_bytes!=raw_bytes+module_bytes||(!module_bytes&&a->transport_module)||(module_bytes&&!a->transport_module))goto done;
 if(!sender_disjoint(o,sizeof(*o),a,sizeof(*a))||!sender_disjoint(o,sizeof(*o),d,sizeof(*d))||!sender_disjoint(a,sizeof(*a),d,sizeof(*d))||!sender_disjoint(o,sizeof(*o),a->transport_raw,raw_bytes)||!sender_disjoint(a,sizeof(*a),a->transport_raw,raw_bytes)||!sender_disjoint(d,sizeof(*d),a->transport_raw,raw_bytes))goto done;
 if(module_bytes&&(!sender_disjoint(o,sizeof(*o),a->transport_module,module_bytes)||!sender_disjoint(a,sizeof(*a),a->transport_module,module_bytes)||!sender_disjoint(d,sizeof(*d),a->transport_module,module_bytes)||!sender_disjoint(a->transport_raw,raw_bytes,a->transport_module,module_bytes)))goto done;
 o->actual_server=d->server;o->body_extent=a->transport_body_bytes;
 if(!sender_add(o,a->transport_header,sizeof(a->transport_header),0)||!sender_add(o,a->transport_raw,raw_bytes,1)||(module_bytes&&!sender_add(o,a->transport_module,module_bytes,2)))goto done;
 o->result=1;
done:o->returned=1;return o->result;
}

static void sender_enter(sl_win_call *o,const char *source,void *subject){o->source=source;o->subject=subject;o->entered=1;}
static void sender_return(sl_win_call *o,ULONG_PTR result,DWORD error){o->result=result;o->last_error=error;o->returned=1;}
static int sender_output_disjoint(sl_win_argument_sender *s,const void *output,SIZE_T extent){
 const sl_win_private_arguments *a;
 if(!s||!s->returned||!s->result||!s->actual_arguments||!s->actual_duplex||s->span_count>4)return 0;
 a=s->actual_arguments;
 if(!sender_disjoint(output,extent,s,sizeof(*s))||!sender_disjoint(output,extent,a,sizeof(*a))||!sender_disjoint(output,extent,s->actual_duplex,sizeof(*s->actual_duplex)))return 0;
 for(SIZE_T i=0;i<s->span_count;i++)if(!sender_disjoint(output,extent,s->spans[i].bytes,s->spans[i].extent))return 0;
 return 1;
}
static int sender_write_fresh(sl_win_argument_write *o){
 return o&&!o->attempted&&!o->returned&&!o->actual_sender&&sender_zero(&o->actual_span,sizeof(o->actual_span))&&!o->actual_server&&!o->event&&sender_zero(&o->overlap,sizeof(o->overlap))&&sender_zero(&o->thread_query,sizeof(o->thread_query))&&sender_zero(&o->event_create,sizeof(o->event_create))&&sender_zero(&o->write,sizeof(o->write))&&sender_zero(&o->worker_query,sizeof(o->worker_query))&&sender_zero(&o->suspension,sizeof(o->suspension))&&sender_zero(&o->cancellation_check,sizeof(o->cancellation_check))&&sender_zero(&o->worker_cancel,sizeof(o->worker_cancel))&&!o->cancellation_requested&&!o->worker_cancellation_observed&&!o->worker_internal_before&&!o->worker_internal_after&&!o->worker_transferred&&!o->worker_macro_before&&!o->worker_macro_after&&!o->worker_settled&&!o->issuing_thread_id&&!o->initiation_supported&&!o->operation_settled&&!o->delivered&&!o->result;
}
static int sender_write_begin(sl_win_argument_write *o){
 sl_win_argument_sender *s;BOOL ok;DWORD error;
 if(!o||o->attempted||o->returned||!sender_zero(&o->thread_query,sizeof(o->thread_query))||!sender_zero(&o->write,sizeof(o->write))||o->issuing_thread_id||o->initiation_supported||o->operation_settled||o->delivered||o->result)return 0;
 if(!sender_output_disjoint(o->sender_original,o,sizeof(*o))||!o->worker_original||!sender_disjoint(o,sizeof(*o),o->worker_original,sizeof(*o->worker_original)))return 0;
 o->attempted=1;s=o->actual_sender;
 if(!sender_output_disjoint(s,o,sizeof(*o))||s!=o->sender_original||o->span_index>=s->span_count||!o->worker_original||o->worker_original->actual_write!=o||!o->worker_original->create.returned||!o->worker_original->create.result||!o->worker_original->thread)goto write_done;
 if(!o->actual_server||o->actual_server==INVALID_HANDLE_VALUE||o->actual_server!=s->actual_server||o->actual_span.bytes!=s->spans[o->span_index].bytes||o->actual_span.extent!=s->spans[o->span_index].extent||!o->event||!o->event_create.returned||o->event_create.result!=(ULONG_PTR)o->event)goto write_done;
 sender_enter(&o->thread_query,"GetCurrentThreadId.argument-write",o);
 o->issuing_thread_id=GetCurrentThreadId();error=GetLastError();sender_return(&o->thread_query,o->issuing_thread_id,error);
 if(!o->issuing_thread_id||o->issuing_thread_id!=o->worker_original->thread_id)goto write_done;
 sender_enter(&o->write,"WriteFile.argument-original-span",o->actual_server);
 // NULL avoids the documented erroneous asynchronous bytes-written output.
 // GetOverlappedResult supplies a separate genuine transferred-count original.
 ok=WriteFile(o->actual_server,o->actual_span.bytes,o->actual_span.extent,NULL,&o->overlap);
 error=GetLastError();sender_return(&o->write,ok,error);
 // Even synchronous TRUE is not promoted to a copied completion/count. The
 // independent completion observer below still joins SAME OVERLAPPED originals.
 if(ok||error==ERROR_IO_PENDING)o->initiation_supported=1;
 else goto write_done; // native initiation failed; no pending operation to wait for
 // Publish-before-request in the owner and this postsubmission atomic read
 // jointly cover cancellation that raced before WriteFile submitted anything.
 sender_enter(&o->cancellation_check,"InterlockedCompareExchange.argument-writer.cancel-observe",o);
 o->worker_cancellation_observed=InterlockedCompareExchange(&o->cancellation_requested,0,0);
 sender_return(&o->cancellation_check,(ULONG_PTR)(DWORD)o->worker_cancellation_observed,0);
 if(o->worker_cancellation_observed==1){
  sender_enter(&o->worker_cancel,"CancelIoEx.argument-writer.postsubmission",o);
  ok=CancelIoEx(o->actual_server,&o->overlap);error=GetLastError();sender_return(&o->worker_cancel,ok,error);
 }
 // The issuing thread remains alive through its own native settlement query.
 o->worker_internal_before=o->overlap.Internal;o->worker_macro_before=HasOverlappedIoCompleted(&o->overlap)?1:0;
 sender_enter(&o->worker_query,"GetOverlappedResult.argument-writer.wait",o);
 ok=GetOverlappedResult(o->actual_server,&o->overlap,&o->worker_transferred,TRUE);
 error=GetLastError();sender_return(&o->worker_query,ok,error);
 o->worker_internal_after=o->overlap.Internal;o->worker_macro_after=HasOverlappedIoCompleted(&o->overlap)?1:0;
 if(ok||(error!=ERROR_IO_INCOMPLETE&&o->worker_macro_after&&o->worker_internal_after!=(ULONG_PTR)STATUS_PENDING)){o->worker_settled=1;o->result=o->initiation_supported?1:0;}
 else {
  // Supported Sleep(INFINITE) retains this live issuing thread and all originals.
  // An unexpected VOID return is a provider-contract violation, never normal exit.
  sender_enter(&o->suspension,"Sleep.argument-writer.unsettled",o);Sleep(INFINITE);
  sender_return(&o->suspension,0,GetLastError());return -1;
 }
write_done:o->returned=1;return o->result;
}
DWORD WINAPI sl_win_argument_write_worker(LPVOID original){
 int result=sender_write_begin((sl_win_argument_write *)original);
 // Normal return occurs only before submission or after native settlement.
 // An unsupported suspension return has a distinct abnormal exit code; no cleanup,
 // close or IO completion is inferred; the held worker must be joined later.
 return result<0?1:0;
}
int sl_win_start_argument_write_worker(sl_win_argument_worker *o){
 DWORD error;sl_win_argument_write *w;sl_win_argument_sender *s;
 if(!o||o->attempted||o->returned||o->actual_write||o->actual_stack_reservation||o->thread||o->thread_id||o->previous_suspend_count||o->result||!sender_zero(&o->create,sizeof(o->create))||!sender_zero(&o->resume,sizeof(o->resume)))return 0;
 w=o->write_original;
 if(!w||!sender_disjoint(o,sizeof(*o),w,sizeof(*w))||!sender_output_disjoint(w->sender_original,o,sizeof(*o))||!sender_output_disjoint(w->sender_original,w,sizeof(*w)))return 0;
 o->attempted=1;o->actual_write=o->write_original;o->actual_stack_reservation=o->stack_reservation;w=o->actual_write;
 if(!w||w->worker_original||!sender_write_fresh(w)||!o->actual_stack_reservation||!sender_disjoint(o,sizeof(*o),w,sizeof(*w))||!sender_output_disjoint(w->sender_original,o,sizeof(*o))||!sender_output_disjoint(w->sender_original,w,sizeof(*w)))goto worker_done;
 w->worker_original=o; // once-only, immutable before thread birth
 s=w->sender_original;w->actual_sender=s;
 if(w->span_index>=s->span_count)goto worker_done;
 w->actual_server=s->actual_server;w->actual_span=s->spans[w->span_index];
 if(!w->actual_server||w->actual_server==INVALID_HANDLE_VALUE||!w->actual_span.bytes||!w->actual_span.extent)goto worker_done;
 // Complete event/OVERLAPPED application initialization before thread birth.
 // Cancellation then cannot race application writes to OVERLAPPED fields.
 sender_enter(&w->event_create,"CreateEventW.argument-write",w);
 w->event=CreateEventW(NULL,TRUE,FALSE,NULL);error=GetLastError();sender_return(&w->event_create,(ULONG_PTR)w->event,error);
 if(!w->event||w->event==INVALID_HANDLE_VALUE||w->event==s->actual_server||w->event==s->actual_duplex->client||w->event==s->actual_duplex->inherited_client)goto worker_done;
 w->overlap.hEvent=w->event;
 // The exact source-bound stack reservation/commit/native-thread/TLS/provider
 // and aggregate containing reservation must already be genuinely financed.
 sender_enter(&o->create,"CreateThread.argument-writer.suspended",w);
 o->thread=CreateThread(NULL,o->actual_stack_reservation,sl_win_argument_write_worker,w,CREATE_SUSPENDED|STACK_SIZE_PARAM_IS_A_RESERVATION,&o->thread_id);
 error=GetLastError();sender_return(&o->create,(ULONG_PTR)o->thread,error);
 if(!o->thread||o->thread==INVALID_HANDLE_VALUE||!o->thread_id||o->thread==w->event||o->thread==s->actual_server||o->thread==s->actual_duplex->client||o->thread==s->actual_duplex->inherited_client)goto worker_done;
 sender_enter(&o->resume,"ResumeThread.argument-writer",o->thread);
 o->previous_suspend_count=ResumeThread(o->thread);error=GetLastError();sender_return(&o->resume,o->previous_suspend_count,error);
 if(o->previous_suspend_count!=1)goto worker_done;
 o->result=1;
worker_done:o->returned=1;return o->result;
}
int sl_win_observe_argument_worker_join(sl_win_argument_worker_join *o){
 sl_win_argument_worker *k;DWORD error;
 if(!o||o->attempted||o->returned||o->actual_worker||!sender_zero(&o->wait,sizeof(o->wait))||!sender_zero(&o->exit_query,sizeof(o->exit_query))||o->status||o->exit_code||o->terminal||o->completed)return 0;
 k=o->worker_original;
 if(!k||!k->actual_write||!sender_disjoint(o,sizeof(*o),k,sizeof(*k))||!sender_disjoint(o,sizeof(*o),k->actual_write,sizeof(*k->actual_write))||!sender_output_disjoint(k->actual_write->sender_original,o,sizeof(*o)))return 0;
 o->attempted=1;o->actual_worker=o->worker_original;k=o->actual_worker;
 if(!k||!k->returned||!k->create.returned||!k->create.result||!k->thread||k->create.result!=(ULONG_PTR)k->thread||!k->actual_write||!sender_disjoint(o,sizeof(*o),k,sizeof(*k))||!sender_disjoint(o,sizeof(*o),k->actual_write,sizeof(*k->actual_write))||!sender_output_disjoint(k->actual_write->sender_original,o,sizeof(*o)))goto join_done;
 // Do not read any worker-written field before this genuine terminal wait.
 // Windows wait functions supply the documented synchronization barrier.
 sender_enter(&o->wait,"WaitForSingleObject.argument-writer.zero",k->thread);
 o->status=WaitForSingleObject(k->thread,0);error=GetLastError();sender_return(&o->wait,o->status,error);
 if(o->status==WAIT_OBJECT_0){
  BOOL ok;o->terminal=1;sender_enter(&o->exit_query,"GetExitCodeThread.argument-writer",k->thread);
  ok=GetExitCodeThread(k->thread,&o->exit_code);error=GetLastError();sender_return(&o->exit_query,ok,error);
  if(ok&&o->exit_code==0)o->completed=1;
 }
join_done:o->returned=1;return o->completed?1:0;
}
int sl_win_observe_argument_write(sl_win_argument_write_observation *o){
 sl_win_argument_write *w;sl_win_argument_worker_join *j;BOOL ok;DWORD error;
 if(!o||o->attempted||o->returned||o->actual_write||o->actual_join||!sender_zero(&o->query,sizeof(o->query))||o->internal_before||o->internal_after||o->transferred||o->macro_before||o->macro_after||o->settled||o->delivered||o->result)return 0;
 w=o->write_original;j=o->join_original;
 if(!w||!j||!j->actual_worker||!sender_disjoint(o,sizeof(*o),w,sizeof(*w))||!sender_disjoint(o,sizeof(*o),j,sizeof(*j))||!sender_disjoint(o,sizeof(*o),j->actual_worker,sizeof(*j->actual_worker))||!sender_output_disjoint(w->sender_original,o,sizeof(*o)))return 0;
 o->attempted=1;o->actual_write=o->write_original;o->actual_join=o->join_original;w=o->actual_write;j=o->actual_join;
 if(!w||!j||!j->returned||!j->completed||!j->wait.returned||j->wait.result!=WAIT_OBJECT_0||!j->actual_worker||j->actual_worker!=w->worker_original||j->actual_worker->actual_write!=w||!sender_disjoint(o,sizeof(*o),j,sizeof(*j))||!sender_disjoint(o,sizeof(*o),j->actual_worker,sizeof(*j->actual_worker)))goto observation_done;
 // Only after the actual joined thread lifetime may non-atomic writer output
 // fields be read. Returned alone is not a worker synchronization primitive.
 if(!w->returned||!w->worker_settled||!w->worker_query.returned||!w->initiation_supported||!w->write.entered||!w->write.returned||w->issuing_thread_id!=j->actual_worker->thread_id||w->operation_settled||!sender_disjoint(o,sizeof(*o),w,sizeof(*w))||!sender_output_disjoint(w->actual_sender,o,sizeof(*o)))goto observation_done;
 o->internal_before=w->overlap.Internal;o->macro_before=HasOverlappedIoCompleted(&w->overlap)?1:0;
 sender_enter(&o->query,"GetOverlappedResult.argument-write",w);
 ok=GetOverlappedResult(w->actual_server,&w->overlap,&o->transferred,FALSE);
 error=GetLastError();sender_return(&o->query,ok,error);
 o->internal_after=w->overlap.Internal;o->macro_after=HasOverlappedIoCompleted(&w->overlap)?1:0;
 if(ok){
  o->settled=1;w->operation_settled=1;
  if(o->transferred==w->actual_span.extent){o->delivered=1;w->delivered=1;o->result=1;}
 }else if(error!=ERROR_IO_INCOMPLETE&&o->macro_after&&o->internal_after!=(ULONG_PTR)STATUS_PENDING){
  // Failed/aborted completed kernel operation is settlement, never delivery.
  o->settled=1;w->operation_settled=1;
 }
observation_done:o->returned=1;return o->result;
}
int sl_win_request_argument_write_cancel(sl_win_argument_write_cancel *o){
 BOOL ok;DWORD error;sl_win_argument_worker *k;sl_win_argument_write *w;sl_win_argument_sender *s;
 if(!o||o->attempted||o->returned||o->actual_worker||!sender_zero(&o->publication,sizeof(o->publication))||!sender_zero(&o->request,sizeof(o->request))||o->previous_cancellation||o->result)return 0;
 k=o->worker_original;
 if(!k||!k->actual_write||!sender_disjoint(o,sizeof(*o),k,sizeof(*k))||!sender_disjoint(o,sizeof(*o),k->actual_write,sizeof(*k->actual_write))||!sender_output_disjoint(k->actual_write->sender_original,o,sizeof(*o)))return 0;
 o->attempted=1;o->actual_worker=o->worker_original;k=o->actual_worker;
 if(!k||!k->returned||!k->create.returned||!k->create.result||!k->actual_write)goto cancel_done;
 w=k->actual_write;s=w->sender_original; // immutable caller configuration
 if(w->worker_original!=k)goto cancel_done;
 if(!sender_disjoint(o,sizeof(*o),k,sizeof(*k))||!sender_disjoint(o,sizeof(*o),w,sizeof(*w))||!sender_output_disjoint(s,o,sizeof(*o)))goto cancel_done;
 // Do not read worker-written flags/handle/event/OVERLAPPED fields while a
 // synchronous WriteFile may still be running. Use immutable plan handle and
 // exact preowned OVERLAPPED address; cancellation may race initial submission.
 sender_enter(&o->publication,"InterlockedExchange.argument-writer.cancel-publish",w);
 o->previous_cancellation=InterlockedExchange(&w->cancellation_requested,1);
 sender_return(&o->publication,(ULONG_PTR)(DWORD)o->previous_cancellation,0);
 sender_enter(&o->request,"CancelIoEx.argument-write.request",w);
 ok=CancelIoEx(s->actual_server,&w->overlap);error=GetLastError();sender_return(&o->request,ok,error);o->result=ok?1:0;
 // Request result does not alter pending/settled/delivered, OVERLAPPED or buffers.
cancel_done:o->returned=1;return o->result;
}
static int sender_turn_fresh(const sl_win_argument_send_turn *o){
 return o&&!o->attempted&&!o->returned&&!o->actual_sequence&&!o->span&&sender_zero(&o->join,sizeof(o->join))&&sender_zero(&o->completion,sizeof(o->completion))&&!o->start_result&&!o->join_result&&!o->completion_result&&!o->advanced&&!o->delivered&&!o->failed&&!o->settled;
}
int sl_win_prepare_argument_send_sequence(sl_win_argument_send_sequence *o){
 SIZE_T bytes;
 if(!o||o->attempted||o->returned||o->actual_sender||o->actual_stack_reservation||o->span_count||o->active_span||o->actual_turns||o->actual_turn_capacity||o->turn_index||o->turn_exhausted||o->prepared||o->cancelled||o->failed||o->delivered||o->active_settled||!sender_zero(o->writes,sizeof(o->writes))||!sender_zero(o->workers,sizeof(o->workers)))return 0;
 if(!sender_output_disjoint(o->sender_original,o,sizeof(*o)))return 0;
 if(!o->turns||!o->turn_capacity||o->turn_capacity>~(SIZE_T)0/sizeof(*o->turns))return 0;
 bytes=o->turn_capacity*sizeof(*o->turns);
 if(!sender_disjoint(o,sizeof(*o),o->turns,bytes)||!sender_output_disjoint(o->sender_original,o->turns,bytes))return 0;
 for(SIZE_T i=0;i<o->turn_capacity;i++)if(o->turns[i].sequence_original!=o||!sender_turn_fresh(&o->turns[i]))return 0;
 o->attempted=1;o->actual_sender=o->sender_original;o->actual_stack_reservation=o->stack_reservation;
 o->actual_turns=o->turns;o->actual_turn_capacity=o->turn_capacity;
 if(!o->actual_stack_reservation||!sender_output_disjoint(o->actual_sender,o,sizeof(*o))||!o->actual_sender->span_count||o->actual_sender->span_count>4)goto sequence_done;
 o->span_count=o->actual_sender->span_count;
 for(SIZE_T i=0;i<o->span_count;i++){
  o->writes[i].sender_original=o->actual_sender;o->writes[i].span_index=i;
  o->workers[i].write_original=&o->writes[i];o->workers[i].stack_reservation=o->actual_stack_reservation;
 }
 o->prepared=1;
sequence_done:o->returned=1;return o->prepared?1:0;
}
int sl_win_service_argument_send(sl_win_argument_send_turn *o){
 sl_win_argument_send_sequence *s;sl_win_argument_worker *k;
 if(!o||o->attempted||o->returned||o->actual_sequence||o->span||!sender_zero(&o->join,sizeof(o->join))||!sender_zero(&o->completion,sizeof(o->completion))||o->start_result||o->join_result||o->completion_result||o->advanced||o->delivered||o->failed||o->settled)return 0;
 s=o->sequence_original;
 if(!s||!s->returned||!s->prepared||!sender_disjoint(o,sizeof(*o),s,sizeof(*s))||!sender_output_disjoint(s->actual_sender,o,sizeof(*o)))return 0;
 if(!s->actual_turns||s->turn_index>=s->actual_turn_capacity||o!=&s->actual_turns[s->turn_index])return 0;
 s->turn_index++; // consume exact preowned row before any native effect
 o->attempted=1;o->actual_sequence=o->sequence_original;s=o->actual_sequence;
 if(!s||!s->returned||!s->prepared||!s->span_count||s->span_count>4||s->active_span>s->span_count||!sender_disjoint(o,sizeof(*o),s,sizeof(*s))||!sender_output_disjoint(s->actual_sender,o,sizeof(*o)))goto turn_done;
 if(s->delivered||s->active_settled)goto turn_done;
 if(s->active_span==s->span_count){s->failed=1;o->failed=1;goto turn_done;}
 o->span=s->active_span;k=&s->workers[o->span];
 if(!k->attempted){
  if(s->cancelled||s->failed)goto turn_done;
  o->start_result=sl_win_start_argument_write_worker(k);
  if(!o->start_result){s->failed=1;o->failed=1;goto turn_done;}
 }
 o->join.worker_original=k;o->join_result=sl_win_observe_argument_worker_join(&o->join);
 if(!o->join_result){
  if(!o->join.wait.returned||o->join.status!=WAIT_TIMEOUT){s->failed=1;o->failed=1;}
  goto turn_done;
 }
 o->completion.write_original=&s->writes[o->span];o->completion.join_original=&o->join;
 o->completion_result=sl_win_observe_argument_write(&o->completion);
 if(o->completion.settled){s->active_settled=1;o->settled=1;}
 if(!o->completion_result){s->failed=1;o->failed=1;goto turn_done;}
 if(s->cancelled||s->failed)goto turn_done;
 // A different preowned occurrence owns every span; never reset/reuse a write.
 s->active_span++;s->active_settled=0;o->advanced=1;
 if(s->active_span==s->span_count){s->delivered=1;o->delivered=1;}
turn_done:o->returned=1;return o->delivered?1:0;
}
int sl_win_service_next_argument_send(sl_win_argument_send_sequence *s){
 if(!s||!s->returned||!s->prepared||!s->actual_turns||s->turn_index>s->actual_turn_capacity)return 0;
 if(s->turn_index==s->actual_turn_capacity){s->turn_exhausted=1;return 0;}
 return sl_win_service_argument_send(&s->actual_turns[s->turn_index]);
}
int sl_win_cancel_argument_send(sl_win_argument_send_cancellation *o){
 sl_win_argument_send_sequence *s;sl_win_argument_worker *k;
 if(!o||o->attempted||o->returned||o->actual_sequence||o->span||!sender_zero(&o->cancellation,sizeof(o->cancellation))||o->dispatch_stopped||o->request_result)return 0;
 s=o->sequence_original;
 if(!s||!s->returned||!s->prepared||!sender_disjoint(o,sizeof(*o),s,sizeof(*s))||!sender_output_disjoint(s->actual_sender,o,sizeof(*o)))return 0;
 if(!s->actual_turns||!s->actual_turn_capacity||s->actual_turn_capacity>~(SIZE_T)0/sizeof(*s->actual_turns)||!sender_disjoint(o,sizeof(*o),s->actual_turns,s->actual_turn_capacity*sizeof(*s->actual_turns)))return 0;
 o->attempted=1;o->actual_sequence=o->sequence_original;s=o->actual_sequence;
 if(!s||!s->returned||!s->prepared||!s->span_count||s->span_count>4||s->active_span>s->span_count||!sender_disjoint(o,sizeof(*o),s,sizeof(*s))||!sender_output_disjoint(s->actual_sender,o,sizeof(*o)))goto sequence_cancel_done;
 // Same owner thread serializes service and cancellation. Stop future birth
 // before native cancellation publication; no worker output is inspected.
 s->cancelled=1;o->dispatch_stopped=1;o->span=s->active_span;
 if(o->span==s->span_count)goto sequence_cancel_done;
 k=&s->workers[o->span];
 if(!k->attempted||!k->returned||!k->create.returned||!k->create.result)goto sequence_cancel_done;
 o->cancellation.worker_original=k;
 o->request_result=sl_win_request_argument_write_cancel(&o->cancellation);
sequence_cancel_done:o->returned=1;return o->dispatch_stopped?1:0;
}
#endif
