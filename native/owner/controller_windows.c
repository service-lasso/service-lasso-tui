#ifdef _WIN32
#include "owner_windows.h"
#include <string.h>
/* Root, family population and raw EOF are distinct original observations.
 * No process wait result grants natural-family-zero or terminal disposition. */
int sl_win_observe_root_exit(sl_win_root_observation *observation) {
    DWORD wait,error;
    BOOL result;
    if (!observation || observation->attempted || !observation->birth_original ||
        !observation->birth_original->process.hProcess) return 0;
    observation->attempted=1;
    observation->wait.source="controller.root.WaitForSingleObject.zero";
    observation->wait.subject=observation->birth_original;observation->wait.entered=1;
    wait=WaitForSingleObject(observation->birth_original->process.hProcess,0);
    error=GetLastError();
    observation->wait.result=wait;observation->wait.last_error=error;observation->wait.returned=1;
    if (wait!=WAIT_OBJECT_0) return 0;
    observation->exit_code.source="controller.root.GetExitCodeProcess";
    observation->exit_code.subject=observation->birth_original;observation->exit_code.entered=1;
    result=GetExitCodeProcess(observation->birth_original->process.hProcess,&observation->code);
    error=GetLastError();
    observation->exit_code.result=(ULONG_PTR)result;observation->exit_code.last_error=error;
    observation->exit_code.returned=1;
    /* A real signaled process may have exit code259; never reinterpret that
     * actual completed code as STILL_ACTIVE after its genuine signaled wait. */
    return result!=FALSE;
}
int sl_win_observe_family(sl_win_family_observation *observation) {
    BOOL result;DWORD error;
    if (!observation || observation->attempted || !observation->birth_original ||
        !observation->birth_original->job) return 0;
    observation->attempted=1;
    observation->query.source="controller.family.QueryInformationJobObject.accounting";
    observation->query.subject=observation->birth_original;observation->query.entered=1;
    result=QueryInformationJobObject(observation->birth_original->job,JobObjectBasicAccountingInformation,
                                    &observation->accounting,sizeof(observation->accounting),&observation->returned_length);
    error=GetLastError();
    observation->query.result=(ULONG_PTR)result;observation->query.last_error=error;observation->query.returned=1;
    /* Successful query retains actual population; no result here grants a
     * complete nonbreakaway/ancestry/rights/profile proof or stream closure. */
    return result!=FALSE;
}
int sl_win_service_sources(sl_win_service_observation *o){
 if(!o||o->attempted)return 0;
 o->attempted=1;o->result=0;o->actual_birth=o->birth_original;
 if(!o->actual_birth||o->root.attempted||o->family.attempted||
    o->root.birth_original||o->family.birth_original||o->root.wait.entered||o->root.wait.returned||o->root.exit_code.entered||o->root.exit_code.returned||o->family.query.entered||o->family.query.returned)goto done;
 o->root.birth_original=o->actual_birth;o->family.birth_original=o->actual_birth;
 /* Native not-ready and failed root observation cannot suppress family IO. */
 o->root_result=sl_win_observe_root_exit(&o->root);
 o->family_result=sl_win_observe_family(&o->family);
 o->root_ready=o->root_result!=0&&o->root.wait.entered&&o->root.wait.returned&&
  o->root.wait.result==WAIT_OBJECT_0&&o->root.exit_code.entered&&
  o->root.exit_code.returned&&o->root.exit_code.result!=FALSE;
 o->family_observed=o->family_result!=0&&o->family.query.entered&&
  o->family.query.returned&&o->family.query.result!=FALSE;
 /* Successful servicing is distinct from root termination or zero family. */
 o->result=o->family_observed&&o->root.wait.entered&&o->root.wait.returned&&
  (o->root.wait.result==WAIT_TIMEOUT||o->root_ready);
done:o->returned=1;return o->result;
}
static int sl_win_prepared_streams_source(sl_win_entry *e){
 if(!e->public_streams_attempted||!e->public_streams_prepared||!e->current_process_query.entered||!e->current_process_query.returned||e->current_process_query.subject!=e||e->current_process_query.result!=(ULONG_PTR)e->current_process)return 0;
 const DWORD selectors[]={STD_INPUT_HANDLE,STD_OUTPUT_HANDLE,STD_ERROR_HANDLE};
 for(unsigned i=0;i<3;i++){
  sl_win_public_stream *s=&e->public_streams[i];
  if(s->selector!=selectors[i]||!s->get_handle.entered||!s->get_handle.returned||s->get_handle.subject!=s||!s->get_handle.source||strcmp(s->get_handle.source,"public.entry.GetStdHandle")||s->get_handle.result!=(ULONG_PTR)s->original)return 0;
  if(s->observed_absent||s->observed_invalid){
   if(s->observed_absent!=(s->original==NULL)||s->observed_invalid!=(s->original==INVALID_HANDLE_VALUE)||
      s->held||s->duplicate_failed||s->duplicate.entered||s->duplicate.returned)return 0;
  }else{
   if(!s->original||s->original==INVALID_HANDLE_VALUE||!s->duplicate.entered||!s->duplicate.returned||s->duplicate.subject!=s||!s->duplicate.source||strcmp(s->duplicate.source,"public.entry.DuplicateHandle.standard-view"))return 0;
   if(s->duplicate_failed){if(s->duplicate.result)return 0;/* Raw output is indeterminate: no adoption/query. */}
   else if(!s->duplicate.result||!s->held||s->held==INVALID_HANDLE_VALUE)return 0;
  }
 }
 return 1;
}
static int sl_win_construct_public_controller_source(sl_win_public_controller *o,unsigned prepared){
 if(!o||o->attempted)return 0;
 o->attempted=1;o->construction_result=0;
 o->actual_services=o->services;o->actual_service_capacity=o->service_capacity;
 ULONG_PTR base=(ULONG_PTR)o->actual_services,owner=(ULONG_PTR)o;SIZE_T bytes;
 if(!base||!o->actual_service_capacity||o->actual_service_capacity>~(SIZE_T)0/sizeof(*o->actual_services)||o->service_count||(!prepared&&o->entry.public_streams_attempted)||o->entry.birth.attempted)goto done;
 bytes=o->actual_service_capacity*sizeof(*o->actual_services);
 if(base>~(ULONG_PTR)0-bytes||owner>~(ULONG_PTR)0-sizeof(*o)||(base<owner+sizeof(*o)&&owner<base+bytes))goto done;
 for(SIZE_T i=0;i<o->actual_service_capacity;i++){
  sl_win_service_observation *v=&o->actual_services[i];
  if(v->attempted||v->returned||v->birth_original||v->actual_birth||v->root.attempted||v->family.attempted||v->root.birth_original||v->family.birth_original||v->root.wait.entered||v->root.wait.returned||v->root.exit_code.entered||v->root.exit_code.returned||v->family.query.entered||v->family.query.returned)goto done;
 }
 for(SIZE_T i=0;i<o->actual_service_capacity;i++)o->actual_services[i].birth_original=&o->entry.birth;
 o->services_prepared=1;
 o->streams_result=prepared?sl_win_prepared_streams_source(&o->entry):sl_win_prepare_public_streams(&o->entry);if(!o->streams_result)goto done;
 o->birth_result=sl_win_prepare_entry_birth(&o->entry);
 o->construction_result=o->birth_result!=0;
done:o->returned=1;return o->construction_result;
}
int sl_win_construct_public_controller(sl_win_public_controller *o){return sl_win_construct_public_controller_source(o,0);}
int sl_win_construct_public_controller_from_prepared_streams(sl_win_public_controller *o){return sl_win_construct_public_controller_source(o,1);}
int sl_win_service_public_controller(sl_win_public_controller *o){
 if(!o||!o->attempted||!o->returned||!o->services_prepared||!o->actual_services)return 0;
 if(o->service_count==o->actual_service_capacity){o->service_exhausted=1;return 0;}
 if(o->service_count>o->actual_service_capacity)return 0;
 sl_win_service_observation *v=&o->actual_services[o->service_count];
 if(v->attempted||v->birth_original!=&o->entry.birth)return 0;
 o->last_service=v;o->service_count++;
 o->last_service_result=sl_win_service_sources(v);
 return o->last_service_result;
}
#endif
