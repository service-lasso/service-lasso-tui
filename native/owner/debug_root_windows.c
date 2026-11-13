#ifdef _WIN32
#include "debug_root_windows.h"
#ifndef SL_WINDOWS_ROOT_RVA
#error exact payload-first private root header required
#endif

static int debug_fresh(const void *p,SIZE_T size){const unsigned char *b=p;for(SIZE_T i=0;i<size;i++)if(b[i])return 0;return 1;}
static int debug_disjoint(const void *a,SIZE_T an,const void *b,SIZE_T bn){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;return x&&y&&an&&bn&&x<=~(ULONG_PTR)0-an&&y<=~(ULONG_PTR)0-bn&&!(x<y+bn&&y<x+an);}
static void debug_enter(sl_win_call *c,const char *source,void *subject){c->source=source;c->subject=subject;c->entered=1;}
static void debug_return(sl_win_call *c,ULONG_PTR result,DWORD error){c->result=result;c->last_error=error;c->returned=1;}

int sl_win_observe_debug_event(sl_win_debug_event_observation *o){
 BOOL ok;DWORD error,status;
 if(!o||o->attempted||o->returned||o->actual_birth||o->actual_thread_id||o->held_thread_id||o->actual_timeout_ms||o->result||o->create_event_matches_birth||
    !debug_fresh(&o->thread_query,sizeof(o->thread_query))||!debug_fresh(&o->held_thread_query,sizeof(o->held_thread_query))||
    !debug_fresh(&o->held_thread_wait,sizeof(o->held_thread_wait))||!debug_fresh(&o->wait,sizeof(o->wait))||!debug_fresh(&o->event,sizeof(o->event)))return 0;
 if(!o->birth_original||!debug_disjoint(o,sizeof(*o),o->birth_original,sizeof(*o->birth_original)))return 0;
 o->attempted=1;o->actual_birth=o->birth_original;
 if(!o->actual_birth||!o->actual_birth->create_process.returned||!o->actual_birth->create_process.result||
    !(o->actual_birth->creation_flags&DEBUG_ONLY_THIS_PROCESS)||!(o->actual_birth->creation_flags&CREATE_SUSPENDED)||
    !o->actual_birth->creator_thread_query.returned||!o->actual_birth->creator_thread_id||
    !o->actual_birth->creator_thread_duplicate.returned||!o->actual_birth->creator_thread_duplicate.result||
    !o->actual_birth->held_creator_thread)goto done;
 debug_enter(&o->thread_query,"GetCurrentThreadId.debug.wait",o->actual_birth);
 o->actual_thread_id=GetCurrentThreadId();error=GetLastError();debug_return(&o->thread_query,o->actual_thread_id,error);
 if(o->actual_thread_id!=o->actual_birth->creator_thread_id)goto done;
 debug_enter(&o->held_thread_query,"GetThreadId.debug.creator-original",o->actual_birth->held_creator_thread);
 o->held_thread_id=GetThreadId(o->actual_birth->held_creator_thread);error=GetLastError();debug_return(&o->held_thread_query,o->held_thread_id,error);
 if(!o->held_thread_id||o->held_thread_id!=o->actual_thread_id)goto done;
 debug_enter(&o->held_thread_wait,"WaitForSingleObject.debug.creator-original.zero",o->actual_birth->held_creator_thread);
 status=WaitForSingleObject(o->actual_birth->held_creator_thread,0);error=GetLastError();debug_return(&o->held_thread_wait,status,error);
 if(status!=WAIT_TIMEOUT)goto done;
 /* Caller must derive timeout from SAME original deadline/clock schedule.
  * This native attempt itself neither proves D nor renews a deadline. */
 o->actual_timeout_ms=o->timeout_ms;
 debug_enter(&o->wait,"WaitForDebugEvent.original",o->actual_birth);
 ok=WaitForDebugEvent(&o->event,o->actual_timeout_ms);error=GetLastError();debug_return(&o->wait,ok,error);
 if(!ok)goto done;
 /* All actual event unions/handles remain retained even when not matching.
  * The caller cannot treat this observation as authority to continue them. */
 o->create_event_matches_birth=o->event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT&&
   o->event.dwProcessId==o->actual_birth->process.dwProcessId&&o->event.dwThreadId==o->actual_birth->process.dwThreadId;
 o->result=1;
done:o->returned=1;return o->result;
}

int sl_win_observe_mapped_root(sl_win_mapped_root_observation *o){
 BOOL ok;DWORD error;ULONG_PTR region_begin,region_end;sl_win_birth *birth;
 if(!o||o->attempted||o->returned||o->actual_event||o->mapped_base||o->root_address||o->region_returned||o->bytes_read||o->result||
    !debug_fresh(&o->region_query,sizeof(o->region_query))||!debug_fresh(&o->read,sizeof(o->read))||
    !debug_fresh(&o->region,sizeof(o->region))||!debug_fresh(o->bytes,sizeof(o->bytes)))return 0;
 if(!o->event_original||!debug_disjoint(o,sizeof(*o),o->event_original,sizeof(*o->event_original))||
    !o->event_original->actual_birth||!debug_disjoint(o,sizeof(*o),o->event_original->actual_birth,sizeof(*o->event_original->actual_birth)))return 0;
 o->attempted=1;o->actual_event=o->event_original;
 if(!o->actual_event||!o->actual_event->returned||!o->actual_event->result||!o->actual_event->create_event_matches_birth||
    !o->actual_event->wait.returned||!o->actual_event->wait.result||!o->actual_event->actual_birth)goto mapped_done;
 birth=o->actual_event->actual_birth;
 if(!birth->process.hProcess||SL_WINDOWS_ROOT_EXTENT!=sizeof(o->bytes)||
    SL_WINDOWS_ROOT_RVA>SL_WINDOWS_PAYLOAD_IMAGE_SIZE||SL_WINDOWS_ROOT_EXTENT>SL_WINDOWS_PAYLOAD_IMAGE_SIZE-SL_WINDOWS_ROOT_RVA)goto mapped_done;
 o->mapped_base=(ULONG_PTR)o->actual_event->event.u.CreateProcessInfo.lpBaseOfImage;
 if(!o->mapped_base||o->mapped_base>~(ULONG_PTR)0-SL_WINDOWS_ROOT_RVA)goto mapped_done;
 o->root_address=o->mapped_base+SL_WINDOWS_ROOT_RVA;
 if(o->root_address>~(ULONG_PTR)0-sizeof(o->bytes))goto mapped_done;
 debug_enter(&o->region_query,"VirtualQueryEx.debug.exact-root",birth->process.hProcess);
 o->region_returned=VirtualQueryEx(birth->process.hProcess,(LPCVOID)o->root_address,&o->region,sizeof(o->region));
 error=GetLastError();debug_return(&o->region_query,o->region_returned,error);
 if(o->region_returned!=sizeof(o->region))goto mapped_done;
 region_begin=(ULONG_PTR)o->region.BaseAddress;
 if(region_begin>~(ULONG_PTR)0-o->region.RegionSize)goto mapped_done;
 region_end=region_begin+o->region.RegionSize;
 if(o->region.AllocationBase!=(PVOID)o->mapped_base||o->region.State!=MEM_COMMIT||o->region.Type!=MEM_IMAGE||
    (o->region.Protect!=PAGE_READWRITE&&o->region.Protect!=PAGE_WRITECOPY)||
    o->root_address<region_begin||o->root_address>region_end||sizeof(o->bytes)>region_end-o->root_address)goto mapped_done;
 debug_enter(&o->read,"ReadProcessMemory.debug.exact-root",birth->process.hProcess);
 ok=ReadProcessMemory(birth->process.hProcess,(LPCVOID)o->root_address,o->bytes,sizeof(o->bytes),&o->bytes_read);
 error=GetLastError();debug_return(&o->read,ok,error);
 /* Preserve BOOL and partial output separately. Full read is acquisition only. */
 o->result=ok&&o->bytes_read==sizeof(o->bytes);
mapped_done:o->returned=1;return o->result;
}

int sl_win_write_exact_root(sl_win_root_write *o){
 BOOL ok;DWORD error;ULONG_PTR source,owner,before;sl_win_birth *birth;
 if(!o||o->attempted||o->returned||o->actual_before||o->actual_source||o->bytes_written||o->result||
    !debug_fresh(&o->write,sizeof(o->write))||!debug_fresh(&o->readback,sizeof(o->readback)))return 0;
 if(!o->source_original||!o->before_original)return 0;
 source=(ULONG_PTR)o->source_original;owner=(ULONG_PTR)o;before=(ULONG_PTR)o->before_original;
 if(source%__alignof(sl_windows_native_root_storage)||before%__alignof(sl_win_mapped_root_observation)||source>~(ULONG_PTR)0-sizeof(*o->source_original)||
    owner>~(ULONG_PTR)0-sizeof(*o)||before>~(ULONG_PTR)0-sizeof(*o->before_original)||
    (source<owner+sizeof(*o)&&owner<source+sizeof(*o->source_original))||
    (before<owner+sizeof(*o)&&owner<before+sizeof(*o->before_original))||
    (source<before+sizeof(*o->before_original)&&before<source+sizeof(*o->source_original)))return 0;
 /* Protect transitive originals before even recording this output attempt. */
 if(!o->before_original->actual_event||
    !debug_disjoint(o,sizeof(*o),o->before_original->actual_event,sizeof(*o->before_original->actual_event))||
    !o->before_original->actual_event->actual_birth||
    !debug_disjoint(o,sizeof(*o),o->before_original->actual_event->actual_birth,sizeof(*o->before_original->actual_event->actual_birth)))return 0;
 o->attempted=1;o->actual_before=o->before_original;o->actual_source=o->source_original;
 if(!o->actual_before||!o->actual_source||!o->actual_before->returned||!o->actual_before->result||
    !o->actual_before->read.returned||!o->actual_before->read.result||o->actual_before->bytes_read!=sizeof(*o->actual_source)||
    !o->actual_before->actual_event||!o->actual_before->actual_event->actual_birth)goto write_done;
 birth=o->actual_before->actual_event->actual_birth;
 if(o->actual_source->source_extent!=sizeof(*o->actual_source)||
    o->actual_source->endpoint!=(uint64_t)(ULONG_PTR)birth->inherited_child_endpoint||
    o->actual_source->entry_gate!=(uint64_t)(ULONG_PTR)birth->inherited_entry_gate)goto write_done;
 /* Initial compiler BSS observation must remain separate from the write and
  * independent readback. Zero bytes alone do not authenticate any writer. */
 if(!debug_fresh(o->actual_before->bytes,sizeof(o->actual_before->bytes)))goto write_done;
 debug_enter(&o->write,"WriteProcessMemory.debug.exact-root",birth->process.hProcess);
 ok=WriteProcessMemory(birth->process.hProcess,(LPVOID)o->actual_before->root_address,
   o->actual_source,sizeof(*o->actual_source),&o->bytes_written);
 error=GetLastError();debug_return(&o->write,ok,error);
 if(!ok||o->bytes_written!=sizeof(*o->actual_source))goto write_done;
 o->readback.event_original=o->actual_before->actual_event;
 if(!sl_win_observe_mapped_root(&o->readback)||o->readback.root_address!=o->actual_before->root_address)goto write_done;
 {const unsigned char *expected=(const unsigned char*)o->actual_source;
  for(SIZE_T i=0;i<sizeof(*o->actual_source);i++)if(o->readback.bytes[i]!=expected[i])goto write_done;}
 o->result=1;
write_done:o->returned=1;return o->result;
}

int sl_win_release_debug_root(sl_win_debug_release *o){
 BOOL ok;DWORD error,status;sl_win_birth *birth;sl_win_debug_event_observation *event;
 if(!o||o->attempted||o->returned||o->actual_write||o->current_thread_id||o->held_thread_id||o->previous_suspend_count||o->event_handles_unknown||o->result||
    !debug_fresh(&o->current_thread_query,sizeof(o->current_thread_query))||!debug_fresh(&o->held_thread_query,sizeof(o->held_thread_query))||
    !debug_fresh(&o->held_thread_wait,sizeof(o->held_thread_wait))||!debug_fresh(&o->continue_event,sizeof(o->continue_event))||
    !debug_fresh(&o->stop_debug,sizeof(o->stop_debug))||!debug_fresh(&o->resume_primary,sizeof(o->resume_primary))||
    !debug_fresh(o->independent_handles,sizeof(o->independent_handles))||!debug_fresh(o->event_handles,sizeof(o->event_handles)))return 0;
 if(!o->write_original||(ULONG_PTR)o->write_original%__alignof(sl_win_root_write)||
    !debug_disjoint(o,sizeof(*o),o->write_original,sizeof(*o->write_original)))return 0;
 if(!o->write_original->actual_before||!o->write_original->actual_source||
    !debug_disjoint(o,sizeof(*o),o->write_original->actual_before,sizeof(*o->write_original->actual_before))||
    !debug_disjoint(o,sizeof(*o),o->write_original->actual_source,sizeof(*o->write_original->actual_source))||
    !o->write_original->actual_before->actual_event||
    !debug_disjoint(o,sizeof(*o),o->write_original->actual_before->actual_event,sizeof(*o->write_original->actual_before->actual_event))||
    !o->write_original->actual_before->actual_event->actual_birth||
    !debug_disjoint(o,sizeof(*o),o->write_original->actual_before->actual_event->actual_birth,sizeof(*o->write_original->actual_before->actual_event->actual_birth)))return 0;
 o->attempted=1;o->actual_write=o->write_original;
 if(!o->actual_write||!o->actual_write->returned||!o->actual_write->result||!o->actual_write->write.returned||!o->actual_write->write.result||
    !o->actual_write->actual_before||!o->actual_write->actual_before->actual_event)goto release_done;
 event=o->actual_write->actual_before->actual_event;birth=event->actual_birth;
 if(!birth||!event->create_event_matches_birth||!event->wait.returned||!event->wait.result||
    !birth->creator_thread_duplicate.returned||!birth->creator_thread_duplicate.result||!birth->held_creator_thread||
    !birth->process.hProcess||!birth->process.hThread)goto release_done;
 o->independent_handles[0]=birth->process.hProcess;o->independent_handles[1]=birth->process.hThread;o->independent_handles[2]=birth->held_creator_thread;
 o->event_handles[0]=event->event.u.CreateProcessInfo.hProcess;o->event_handles[1]=event->event.u.CreateProcessInfo.hThread;o->event_handles[2]=event->event.u.CreateProcessInfo.hFile;
 /* Distinct fields/origins alone do not prove distinct handle-table entries.
  * Retain/check actual native values before any Continue/Stop. If an event
  * occurrence aliases a later-used independent handle, do not relabel it PI. */
 for(unsigned i=0;i<3;i++){
  if(!o->independent_handles[i]||o->independent_handles[i]==INVALID_HANDLE_VALUE)goto release_done;
  for(unsigned j=0;j<i;j++)if(o->independent_handles[i]==o->independent_handles[j])goto release_done;
  for(unsigned j=0;j<3;j++)if(o->event_handles[j]&&o->independent_handles[i]==o->event_handles[j])goto release_done;
 }
 debug_enter(&o->current_thread_query,"GetCurrentThreadId.debug.release",birth);
 o->current_thread_id=GetCurrentThreadId();error=GetLastError();debug_return(&o->current_thread_query,o->current_thread_id,error);
 debug_enter(&o->held_thread_query,"GetThreadId.debug.release.creator-original",birth->held_creator_thread);
 o->held_thread_id=GetThreadId(birth->held_creator_thread);error=GetLastError();debug_return(&o->held_thread_query,o->held_thread_id,error);
 if(!o->current_thread_id||o->current_thread_id!=birth->creator_thread_id||o->held_thread_id!=o->current_thread_id)goto release_done;
 debug_enter(&o->held_thread_wait,"WaitForSingleObject.debug.release.creator-original.zero",birth->held_creator_thread);
 status=WaitForSingleObject(birth->held_creator_thread,0);error=GetLastError();debug_return(&o->held_thread_wait,status,error);
 if(status!=WAIT_TIMEOUT)goto release_done;
 debug_enter(&o->continue_event,"ContinueDebugEvent.original.create",event);
 ok=ContinueDebugEvent(event->event.dwProcessId,event->event.dwThreadId,DBG_CONTINUE);error=GetLastError();debug_return(&o->continue_event,ok,error);
 if(!ok)goto release_done;
 debug_enter(&o->stop_debug,"DebugActiveProcessStop.original",birth);
 o->event_handles_unknown=1;
 ok=DebugActiveProcessStop(birth->process.dwProcessId);error=GetLastError();debug_return(&o->stop_debug,ok,error);
 if(!ok)goto release_done;
 /* Do not use/query/close debug-event handles after Stop, including failure.
  * Keep their occurrences charged to creator/outer terminal settlement. */
 debug_enter(&o->resume_primary,"ResumeThread.original.PI-primary",birth->process.hThread);
 o->previous_suspend_count=ResumeThread(birth->process.hThread);error=GetLastError();debug_return(&o->resume_primary,o->previous_suspend_count,error);
 /* Exactly the independently created primary suspend remains. A mismatch is
  * retained failure, never fixed by a new Suspend/Resume or forced cleanup. */
 if(o->previous_suspend_count!=1)goto release_done;
 o->result=1;
release_done:o->returned=1;return o->result;
}
#endif
