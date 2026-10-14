#ifdef _WIN32
#include "owner_windows.h"
static int token_call(sl_win_call *c,const char *source,HANDLE token,TOKEN_INFORMATION_CLASS kind,void *body,DWORD capacity,DWORD *required){
 c->source=source;c->subject=token;c->entered=1;
 BOOL result=GetTokenInformation(token,kind,body,capacity,required);DWORD error=GetLastError();
 c->result=(ULONG_PTR)result;c->last_error=error;c->returned=1;return result!=FALSE;
}
int sl_win_observe_process_token(sl_win_token_observation *o,HANDLE process){
 if(!o||o->attempted)return 0;o->attempted=1;o->process_original=process;o->actual_backing=o->backing;o->actual_capacity=o->capacity;
 if(!process||!o->actual_backing||o->actual_capacity<sizeof(TOKEN_ACCESS_INFORMATION))goto done;
 o->open.source="OpenProcessToken.query-only";o->open.subject=process;o->open.entered=1;
 BOOL opened=OpenProcessToken(process,TOKEN_QUERY,&o->token);DWORD error=GetLastError();
 o->open.result=(ULONG_PTR)opened;o->open.last_error=error;o->open.returned=1;
 if(!opened||!o->token)goto done;
 if(!token_call(&o->before_query,"GetTokenInformation.statistics-before",o->token,TokenStatistics,&o->before,sizeof(o->before),&o->before_required)||o->before_required!=sizeof(o->before))goto after;
 if(token_call(&o->information_query,"GetTokenInformation.access-information",o->token,TokenAccessInformation,o->actual_backing,o->actual_capacity,&o->required)&&o->required>=sizeof(TOKEN_ACCESS_INFORMATION)&&o->required<=o->actual_capacity)o->primary_result=1;
 after:o->after_result=token_call(&o->after_query,"GetTokenInformation.statistics-after",o->token,TokenStatistics,&o->after,sizeof(o->after),&o->after_required);
 if(o->after_required!=sizeof(o->after))o->after_result=0;
 o->result=o->primary_result&&o->after_result;
 done:o->returned=1;return o->result;
 /* These are raw observations. Token/ModifiedId correspondence, embedded
  * SID/hash/pointer ranges and authority remain actual consumer obligations. */
}
#endif