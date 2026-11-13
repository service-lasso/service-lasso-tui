#ifdef _WIN32
#include "channels_windows.h"
static int channels_fresh(const sl_win_call *c){return !c->source&&!c->subject&&!c->result&&!c->last_error&&!c->entered&&!c->returned;}
static void channels_enter(sl_win_call *c,const char *source,void *subject){c->source=source;c->subject=subject;c->entered=1;}
static int channels_body(sl_win_channels *o){
 BOOL result;DWORD error;unsigned i;
 channels_enter(&o->process_query,"GetCurrentProcess.channels",o);o->process=GetCurrentProcess();
 o->process_query.result=(ULONG_PTR)o->process;o->process_query.returned=1;
 if(!o->process)return 0;
 for(i=0;i<3;i++){
  o->failed_index=i;channels_enter(&o->create[i],"CreatePipe",&o->read[i]);
  result=CreatePipe(&o->read[i],&o->write[i],NULL,o->actual_suggested_bytes[i]);error=GetLastError();
  o->create[i].result=(ULONG_PTR)result;o->create[i].last_error=error;o->create[i].returned=1;
  if(!result)return 0;
  if(!o->read[i]||o->read[i]==INVALID_HANDLE_VALUE||!o->write[i]||o->write[i]==INVALID_HANDLE_VALUE||o->read[i]==o->write[i])return 0;
  for(unsigned j=0;j<i;j++)if(o->read[i]==o->read[j]||o->read[i]==o->write[j]||o->write[i]==o->read[j]||o->write[i]==o->write[j])return 0;
  o->created_count=i+1;
 }
 o->failed_index=3;channels_enter(&o->gate_duplicate,"DuplicateHandle.channels-child-gate",o->read[0]);
 result=DuplicateHandle(o->process,o->read[0],o->process,&o->inherited_gate,0,TRUE,DUPLICATE_SAME_ACCESS);error=GetLastError();
 o->gate_duplicate.result=(ULONG_PTR)result;o->gate_duplicate.last_error=error;o->gate_duplicate.returned=1;
 if(!result||!o->inherited_gate||o->inherited_gate==INVALID_HANDLE_VALUE)return 0;
 for(i=0;i<3;i++)if(o->inherited_gate==o->read[i]||o->inherited_gate==o->write[i])return 0;
 o->failed_index=0;return 1;
}
int sl_win_construct_channels(sl_win_channels *o){
 unsigned i;
 if(!o||o->attempted||o->returned||o->process||o->inherited_gate||o->created_count||o->failed_index||o->result||!channels_fresh(&o->process_query)||!channels_fresh(&o->gate_duplicate))return 0;
 for(i=0;i<3;i++)if(o->read[i]||o->write[i]||o->actual_suggested_bytes[i]||!o->suggested_bytes[i]||!channels_fresh(&o->create[i]))return 0;
 // nSize is only a kernel buffering suggestion, never a physical-fit receipt.
 for(i=0;i<3;i++)o->actual_suggested_bytes[i]=o->suggested_bytes[i];
 o->attempted=1;o->result=channels_body(o);o->returned=1;return o->result;
}
#endif
