#ifdef _WIN32
#include "duplex_windows.h"
#include <aclapi.h>

static void duplex_enter(sl_win_call *c,const char *source,void *subject){c->source=source;c->subject=subject;c->entered=1;}
static void duplex_return(sl_win_call *c,ULONG_PTR value,DWORD error){c->result=value;c->last_error=error;c->returned=1;}
static int duplex_bytes_fresh(const void *value,SIZE_T size){const unsigned char *p=value;for(SIZE_T i=0;i<size;i++)if(p[i])return 0;return 1;}
static int duplex_call_fresh(const sl_win_call *c){return !c->source&&!c->subject&&!c->result&&!c->last_error&&!c->entered&&!c->returned;}
static int duplex_overlap(const void *a,SIZE_T an,const void *b,SIZE_T bn){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;if(!a||!b||x>~(ULONG_PTR)0-an||y>~(ULONG_PTR)0-bn)return 1;return x<y+bn&&y<x+an;}
static int duplex_name_put(sl_win_duplex *o,WCHAR c){if(o->name_units>=o->actual_name_capacity)return 0;o->actual_name[o->name_units++]=c;return 1;}
static int duplex_name_literal(sl_win_duplex *o,const WCHAR *s){while(*s)if(!duplex_name_put(o,*s++))return 0;return 1;}
static int duplex_name_decimal(sl_win_duplex *o,ULONGLONG value){WCHAR reverse[sizeof(ULONGLONG)*3];SIZE_T n=0;do{if(n==sizeof(reverse)/sizeof(reverse[0]))return 0;reverse[n++]=(WCHAR)(L'0'+value%10);value/=10;}while(value);while(n)if(!duplex_name_put(o,reverse[--n]))return 0;return 1;}
static int duplex_construct_body(sl_win_duplex *o){
 BOOL ok;DWORD error;SIZE_T sizes[5];void *regions[5];
 if((ULONG_PTR)o->name%__alignof(WCHAR)||(ULONG_PTR)o->token_body%__alignof(TOKEN_USER)||(ULONG_PTR)o->acl_body%__alignof(ACL)||(ULONG_PTR)o->completions%__alignof(sl_win_duplex_completion)||!o->name||!o->name_capacity||o->name_capacity>~(SIZE_T)0/sizeof(WCHAR)||!o->token_body||o->token_capacity<sizeof(TOKEN_USER)||!o->acl_body||o->acl_capacity<sizeof(ACL)||!o->completions||!o->completion_capacity||o->completion_capacity>~(SIZE_T)0/sizeof(*o->completions)||!o->settlements||!o->settlement_capacity||o->settlement_capacity>~(SIZE_T)0/sizeof(*o->settlements)||(ULONG_PTR)o->settlements%__alignof(sl_win_duplex_settlement)||!o->input_bytes||!o->output_bytes)return 0;
 regions[0]=o->name;sizes[0]=o->name_capacity*sizeof(WCHAR);regions[1]=o->token_body;sizes[1]=o->token_capacity;regions[2]=o->acl_body;sizes[2]=o->acl_capacity;regions[3]=o->completions;sizes[3]=o->completion_capacity*sizeof(*o->completions);regions[4]=o->settlements;sizes[4]=o->settlement_capacity*sizeof(*o->settlements);
 for(unsigned i=0;i<5;i++){if(duplex_overlap(regions[i],sizes[i],o,sizeof(*o)))return 0;for(unsigned j=0;j<i;j++)if(duplex_overlap(regions[i],sizes[i],regions[j],sizes[j]))return 0;}
 for(SIZE_T i=0;i<o->completion_capacity;i++){sl_win_duplex_completion *v=&o->completions[i];if(v->owner_original||v->attempted||v->returned||v->transferred||v->result||!duplex_call_fresh(&v->query))return 0;}
 for(SIZE_T i=0;i<o->settlement_capacity;i++){sl_win_duplex_settlement *v=&o->settlements[i];if(!duplex_bytes_fresh(v,sizeof(*v)))return 0;}
 o->actual_settlements=o->settlements;o->actual_settlement_capacity=o->settlement_capacity;
 for(SIZE_T i=0;i<o->actual_settlement_capacity;i++)o->actual_settlements[i].owner_original=o;
 o->actual_name=o->name;o->actual_name_capacity=o->name_capacity;o->actual_token_body=o->token_body;o->actual_token_capacity=o->token_capacity;o->actual_acl_body=o->acl_body;o->actual_acl_capacity=o->acl_capacity;o->actual_completions=o->completions;o->actual_completion_capacity=o->completion_capacity;
 for(SIZE_T i=0;i<o->actual_completion_capacity;i++)o->actual_completions[i].owner_original=o;
 duplex_enter(&o->process_query,"GetCurrentProcess.duplex",o);o->process=GetCurrentProcess();error=GetLastError();duplex_return(&o->process_query,(ULONG_PTR)o->process,error);
 duplex_enter(&o->token_open,"OpenProcessToken.duplex.query",o->process);ok=OpenProcessToken(o->process,TOKEN_QUERY,&o->token);error=GetLastError();duplex_return(&o->token_open,ok,error);if(!ok||!o->token)return 0;
 duplex_enter(&o->user_query,"GetTokenInformation.duplex.TokenUser",o->token);ok=GetTokenInformation(o->token,TokenUser,o->actual_token_body,o->actual_token_capacity,&o->token_required);error=GetLastError();duplex_return(&o->user_query,ok,error);if(!ok||o->token_required<sizeof(TOKEN_USER)||o->token_required>o->actual_token_capacity)return 0;
 o->user_sid=((TOKEN_USER*)o->actual_token_body)->User.Sid;
 {ULONG_PTR sid=(ULONG_PTR)o->user_sid,begin=(ULONG_PTR)o->actual_token_body;if(begin>~(ULONG_PTR)0-o->token_required||sid<begin||sid>begin+o->token_required||begin+o->token_required-sid<FIELD_OFFSET(SID,SubAuthority))return 0; SIZE_T sid_extent=FIELD_OFFSET(SID,SubAuthority)+(SIZE_T)((SID*)o->user_sid)->SubAuthorityCount*sizeof(DWORD);if(sid_extent>begin+o->token_required-sid)return 0;}
 duplex_enter(&o->sid_valid,"IsValidSid.duplex",o->user_sid);ok=IsValidSid(o->user_sid);error=GetLastError();duplex_return(&o->sid_valid,ok,error);if(!ok)return 0;
 duplex_enter(&o->sid_length,"GetLengthSid.duplex",o->user_sid);o->sid_bytes=GetLengthSid(o->user_sid);error=GetLastError();duplex_return(&o->sid_length,o->sid_bytes,error);
 {ULONG_PTR sid=(ULONG_PTR)o->user_sid,begin=(ULONG_PTR)o->actual_token_body;if(!o->sid_bytes||o->sid_bytes>begin+o->token_required-sid)return 0;}
 if(o->sid_bytes>MAXDWORD-sizeof(ACL)-(sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)))return 0;
 o->acl_bytes=sizeof(ACL)+sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+o->sid_bytes;if(o->acl_bytes>o->actual_acl_capacity)return 0;
 duplex_enter(&o->acl_initialize,"InitializeAcl.duplex",o->actual_acl_body);ok=InitializeAcl(o->actual_acl_body,o->acl_bytes,ACL_REVISION);error=GetLastError();duplex_return(&o->acl_initialize,ok,error);if(!ok)return 0;
 /* Explicit server construction rights. Client open below excludes the pipe
  * instance bit. This SID construction never proves supplier exclusion. */
 duplex_enter(&o->acl_add,"AddAccessAllowedAce.duplex",o->actual_acl_body);ok=AddAccessAllowedAce(o->actual_acl_body,ACL_REVISION,FILE_READ_DATA|FILE_READ_EA|FILE_WRITE_DATA|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|FILE_CREATE_PIPE_INSTANCE|READ_CONTROL|SYNCHRONIZE,o->user_sid);error=GetLastError();duplex_return(&o->acl_add,ok,error);if(!ok)return 0;
 duplex_enter(&o->descriptor_initialize,"InitializeSecurityDescriptor.duplex",&o->descriptor);ok=InitializeSecurityDescriptor(&o->descriptor,SECURITY_DESCRIPTOR_REVISION);error=GetLastError();duplex_return(&o->descriptor_initialize,ok,error);if(!ok)return 0;
 duplex_enter(&o->descriptor_dacl,"SetSecurityDescriptorDacl.duplex",&o->descriptor);ok=SetSecurityDescriptorDacl(&o->descriptor,TRUE,o->actual_acl_body,FALSE);error=GetLastError();duplex_return(&o->descriptor_dacl,ok,error);if(!ok)return 0;
 duplex_enter(&o->descriptor_control,"SetSecurityDescriptorControl.duplex",&o->descriptor);ok=SetSecurityDescriptorControl(&o->descriptor,SE_DACL_PROTECTED,SE_DACL_PROTECTED);error=GetLastError();duplex_return(&o->descriptor_control,ok,error);if(!ok)return 0;
 o->attributes.nLength=sizeof(o->attributes);o->attributes.lpSecurityDescriptor=&o->descriptor;o->attributes.bInheritHandle=FALSE;
 duplex_enter(&o->pid_query,"GetCurrentProcessId.duplex",o);o->pid=GetCurrentProcessId();error=GetLastError();duplex_return(&o->pid_query,o->pid,error);
 duplex_enter(&o->tick_query,"GetTickCount64.duplex",o);o->tick=GetTickCount64();error=GetLastError();duplex_return(&o->tick_query,(ULONG_PTR)o->tick,error);
 if(!duplex_name_literal(o,L"\\\\.\\pipe\\service-lasso-tui-")||!duplex_name_decimal(o,o->pid)||!duplex_name_put(o,L'-')||!duplex_name_decimal(o,o->tick)||!duplex_name_put(o,0)){o->exhausted=1;return 0;}
 duplex_enter(&o->server_create,"CreateNamedPipeW.duplex.first-instance",o->actual_name);o->server=CreateNamedPipeW(o->actual_name,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,o->output_bytes,o->input_bytes,0,&o->attributes);error=GetLastError();duplex_return(&o->server_create,(ULONG_PTR)o->server,error);if(o->server==INVALID_HANDLE_VALUE||!o->server)return 0;
 duplex_enter(&o->event_create,"CreateEventW.duplex.connect",o);o->event=CreateEventW(NULL,TRUE,FALSE,NULL);error=GetLastError();duplex_return(&o->event_create,(ULONG_PTR)o->event,error);if(!o->event)return 0;o->connect.hEvent=o->event;
 duplex_enter(&o->connect_begin,"ConnectNamedPipe.duplex.overlapped",o->server);ok=ConnectNamedPipe(o->server,&o->connect);error=GetLastError();duplex_return(&o->connect_begin,ok,error);
 if(ok||error==ERROR_PIPE_CONNECTED)o->connected=1;else if(error==ERROR_IO_PENDING)o->pending=1;else return 0;
 /* Explicit documented GENERIC_READ mapping for duplex pipe observation,
  * plus existing data write. Never request GENERIC_WRITE/instance creation. */
 duplex_enter(&o->client_open,"CreateFileW.duplex.client",o->actual_name);o->client=CreateFileW(o->actual_name,FILE_READ_DATA|FILE_READ_EA|FILE_READ_ATTRIBUTES|FILE_WRITE_DATA|READ_CONTROL|SYNCHRONIZE,0,NULL,OPEN_EXISTING,0,NULL);error=GetLastError();duplex_return(&o->client_open,(ULONG_PTR)o->client,error);if(!o->client||o->client==INVALID_HANDLE_VALUE)return 0;
 duplex_enter(&o->client_duplicate,"DuplicateHandle.duplex.inherited-client",o->client);ok=DuplicateHandle(o->process,o->client,o->process,&o->inherited_client,0,TRUE,DUPLICATE_SAME_ACCESS);error=GetLastError();duplex_return(&o->client_duplicate,ok,error);if(!ok||!o->inherited_client)return 0;
 return o->connected?1:2;
}
int sl_win_construct_duplex(sl_win_duplex *o){
 if(!o||o->attempted||o->returned||o->name_units||o->completion_count||o->process||o->token||o->server||o->event||o->client||o->inherited_client||o->pending||o->connected||o->completion_failed||o->connect.Internal||o->connect.InternalHigh||o->connect.Offset||o->connect.OffsetHigh||o->connect.hEvent)return 0;
 sl_win_call *calls[]={&o->process_query,&o->token_open,&o->user_query,&o->sid_valid,&o->sid_length,&o->acl_initialize,&o->acl_add,&o->descriptor_initialize,&o->descriptor_dacl,&o->descriptor_control,&o->pid_query,&o->tick_query,&o->server_create,&o->event_create,&o->connect_begin,&o->client_open,&o->client_duplicate};
 if(o->actual_name||o->actual_name_capacity||o->actual_token_body||o->actual_token_capacity||o->token_required||o->actual_acl_body||o->actual_acl_capacity||o->actual_completions||o->actual_completion_capacity||o->completed_by||o->user_sid||o->sid_bytes||o->acl_bytes||o->pid||o->tick||o->exhausted||o->result||o->actual_settlements||o->actual_settlement_capacity||o->settled_by||o->settlement_count||o->cancel_attempted||o->cancel_returned||o->operation_settled||!duplex_call_fresh(&o->cancel_request))return 0;
 if(!duplex_bytes_fresh(&o->descriptor,sizeof(o->descriptor))||!duplex_bytes_fresh(&o->attributes,sizeof(o->attributes)))return 0;
 for(unsigned i=0;i<sizeof(calls)/sizeof(calls[0]);i++)if(!duplex_call_fresh(calls[i]))return 0;
 o->attempted=1;o->result=duplex_construct_body(o);o->returned=1;return o->result;
}
int sl_win_observe_duplex_completion(sl_win_duplex *o){
 sl_win_duplex_completion *v;BOOL ok;DWORD error;
 if(!o||!o->attempted||!o->returned||!o->pending||o->connected||o->completion_failed||!o->actual_completions)return 0;
 if(o->completion_count==o->actual_completion_capacity){o->exhausted=1;return 0;}if(o->completion_count>o->actual_completion_capacity)return 0;
 v=&o->actual_completions[o->completion_count];if(v->owner_original!=o||v->attempted||v->query.entered||v->query.returned)return 0;
 v->attempted=1;o->completion_count++;duplex_enter(&v->query,"GetOverlappedResult.duplex.connect",o->server);
 ok=GetOverlappedResult(o->server,&o->connect,&v->transferred,FALSE);error=GetLastError();duplex_return(&v->query,ok,error);
 /* transferred is retained raw but undefined for connect; never byte proof. */
 v->result=ok?1:(error==ERROR_IO_INCOMPLETE?2:0);v->returned=1;
 if(ok){o->connected=1;o->pending=0;o->completed_by=v;}else if(error!=ERROR_IO_INCOMPLETE){o->completion_failed=1;}
 /* Failed/unknown completion remains pending custody, with no new query. */
 return v->result;
}
int sl_win_request_duplex_cancel(sl_win_duplex *o){
 BOOL ok;DWORD error;
 if(!o||!o->attempted||!o->returned||!o->pending||o->operation_settled||o->cancel_attempted||!duplex_call_fresh(&o->cancel_request)||!o->connect_begin.returned||o->connect_begin.result||o->connect_begin.last_error!=ERROR_IO_PENDING)return 0;
 o->cancel_attempted=1;duplex_enter(&o->cancel_request,"CancelIoEx.duplex.original-connect",o->server);
 ok=CancelIoEx(o->server,&o->connect);error=GetLastError();duplex_return(&o->cancel_request,ok,error);o->cancel_returned=1;
 /* TRUE and ERROR_NOT_FOUND are request observations, never completion. */
 return ok!=FALSE;
}
int sl_win_observe_duplex_settlement(sl_win_duplex *o){
 sl_win_duplex_settlement *v;BOOL ok;DWORD error;
 if(!o||!o->cancel_returned||!o->cancel_request.returned||!o->pending||o->operation_settled||!o->actual_settlements)return 0;
 if(o->settlement_count==o->actual_settlement_capacity){o->exhausted=1;return 0;}if(o->settlement_count>o->actual_settlement_capacity)return 0;
 v=&o->actual_settlements[o->settlement_count];if(v->owner_original!=o||v->attempted||v->returned||!duplex_call_fresh(&v->query))return 0;
 v->attempted=1;o->settlement_count++;
 /* Only SAME operation whose actual begin returned ERROR_IO_PENDING. SDK
  * macro evaluates DWORD Internal != STATUS_PENDING; no event-alone proof. */
 v->internal_before=o->connect.Internal;v->macro_before=HasOverlappedIoCompleted(&o->connect)!=0;
 duplex_enter(&v->query,"GetOverlappedResult.duplex.settlement",o->server);
 ok=GetOverlappedResult(o->server,&o->connect,&v->transferred,FALSE);error=GetLastError();duplex_return(&v->query,ok,error);
 v->internal_after=o->connect.Internal;v->macro_after=HasOverlappedIoCompleted(&o->connect)!=0;
 v->outcome=ok?1:(error==ERROR_OPERATION_ABORTED?2:(error==ERROR_IO_INCOMPLETE?4:3));
 v->completed=v->macro_after&&v->outcome!=4;v->returned=1;
 if(v->completed){o->operation_settled=1;o->pending=0;o->settled_by=v;}
 /* Kernel borrower retirement only: no connection success, failure erasure,
  * descriptor custody/disposition or owner lifetime grant. */
 return v->completed?1:2;
}
#endif
