#ifdef _WIN32
#include "../duplex_windows.h"
#include <assert.h>
#include <string.h>
static unsigned mode,completion_calls,client_calls,duplicate_calls,process_calls,cancel_calls;
static DWORD last;
static HANDLE fake_process(void){process_calls++;return (HANDLE)(LONG_PTR)-1;}
static BOOL fake_token(HANDLE p,DWORD access,PHANDLE token){assert(p==(HANDLE)(LONG_PTR)-1&&access==TOKEN_QUERY);*token=(HANDLE)(ULONG_PTR)10;return TRUE;}
static BOOL fake_user(HANDLE token,TOKEN_INFORMATION_CLASS kind,LPVOID buffer,DWORD bytes,PDWORD required){
 assert(token==(HANDLE)(ULONG_PTR)10&&kind==TokenUser);*required=sizeof(TOKEN_USER)+12;
 if(mode==3){last=ERROR_INSUFFICIENT_BUFFER;*required=bytes+1;return FALSE;}
 assert(bytes>=*required);TOKEN_USER *u=buffer;SID *sid=(SID*)((unsigned char*)buffer+sizeof(TOKEN_USER));memset(sid,0,12);sid->Revision=SID_REVISION;sid->SubAuthorityCount=1;u->User.Sid=sid;return TRUE;
}
static BOOL fake_valid(PSID sid){assert(((SID*)sid)->SubAuthorityCount==1);return TRUE;}
static DWORD fake_sid_length(PSID sid){(void)sid;return 12;}
static BOOL fake_acl(PACL p,DWORD n,DWORD revision){assert(p&&n>=sizeof(ACL)&&revision==ACL_REVISION);return TRUE;}
static BOOL fake_ace(PACL p,DWORD revision,DWORD access,PSID sid){assert(p&&sid&&revision==ACL_REVISION&&(access&FILE_CREATE_PIPE_INSTANCE));return TRUE;}
static BOOL fake_sd(PSECURITY_DESCRIPTOR d,DWORD revision){assert(d&&revision==SECURITY_DESCRIPTOR_REVISION);return TRUE;}
static BOOL fake_dacl(PSECURITY_DESCRIPTOR d,BOOL present,PACL acl,BOOL defaulted){assert(d&&present&&acl&&!defaulted);return TRUE;}
static BOOL fake_control(PSECURITY_DESCRIPTOR d,SECURITY_DESCRIPTOR_CONTROL mask,SECURITY_DESCRIPTOR_CONTROL bits){assert(d&&mask==SE_DACL_PROTECTED&&bits==SE_DACL_PROTECTED);return TRUE;}
static DWORD fake_pid(void){return 42;}
static ULONGLONG fake_tick(void){return 987;}
static HANDLE fake_server(LPCWSTR name,DWORD access,DWORD pipe,DWORD instances,DWORD output,DWORD input,DWORD timeout,LPSECURITY_ATTRIBUTES sa){assert(name&&instances==1&&output==32&&input==32&&timeout==0&&sa&&!sa->bInheritHandle&&(access&FILE_FLAG_OVERLAPPED)&&(access&FILE_FLAG_FIRST_PIPE_INSTANCE)&&(pipe&PIPE_REJECT_REMOTE_CLIENTS));return (HANDLE)(ULONG_PTR)20;}
static HANDLE fake_event(LPSECURITY_ATTRIBUTES sa,BOOL manual,BOOL initial,LPCWSTR name){assert(!sa&&manual&&!initial&&!name);return (HANDLE)(ULONG_PTR)30;}
static BOOL fake_connect(HANDLE server,LPOVERLAPPED ov){assert(server==(HANDLE)(ULONG_PTR)20&&ov->hEvent==(HANDLE)(ULONG_PTR)30);ov->Internal=STATUS_PENDING;last=ERROR_IO_PENDING;return FALSE;}
static HANDLE fake_client(LPCWSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD disposition,DWORD flags,HANDLE template_handle){assert(name&&access==(FILE_READ_DATA|FILE_READ_EA|FILE_READ_ATTRIBUTES|READ_CONTROL|FILE_WRITE_DATA|SYNCHRONIZE)&&!(access&FILE_CREATE_PIPE_INSTANCE)&&!share&&!sa&&disposition==OPEN_EXISTING&&!flags&&!template_handle);client_calls++;if(mode==2){last=ERROR_ACCESS_DENIED;return INVALID_HANDLE_VALUE;}return (HANDLE)(ULONG_PTR)40;}
static BOOL fake_duplicate(HANDLE source,HANDLE client,HANDLE target,LPHANDLE out,DWORD access,BOOL inherit,DWORD options){assert(source==target&&client==(HANDLE)(ULONG_PTR)40&&!access&&inherit&&options==DUPLICATE_SAME_ACCESS);duplicate_calls++;*out=(HANDLE)(ULONG_PTR)50;return TRUE;}
static BOOL fake_completion(HANDLE server,LPOVERLAPPED ov,LPDWORD transferred,BOOL wait){assert(server==(HANDLE)(ULONG_PTR)20&&ov->hEvent==(HANDLE)(ULONG_PTR)30&&!wait);completion_calls++;*transferred=0xabc;if(mode==1){if(cancel_calls){ov->Internal=0xc0000120;last=ERROR_OPERATION_ABORTED;}else last=ERROR_INVALID_HANDLE;return FALSE;}if(completion_calls==1){last=ERROR_IO_INCOMPLETE;return FALSE;}ov->Internal=0;return TRUE;}
static DWORD fake_error(void){return last;}
#define GetCurrentProcess fake_process
#define OpenProcessToken fake_token
#define GetTokenInformation fake_user
#define IsValidSid fake_valid
#define GetLengthSid fake_sid_length
#define InitializeAcl fake_acl
#define AddAccessAllowedAce fake_ace
#define InitializeSecurityDescriptor fake_sd
#define SetSecurityDescriptorDacl fake_dacl
#define SetSecurityDescriptorControl fake_control
#define GetCurrentProcessId fake_pid
#define GetTickCount64 fake_tick
#define CreateNamedPipeW fake_server
#define CreateEventW fake_event
#define ConnectNamedPipe fake_connect
#define CreateFileW fake_client
#define DuplicateHandle fake_duplicate
#define GetOverlappedResult fake_completion
static BOOL fake_cancel(HANDLE server,LPOVERLAPPED ov){assert(server==(HANDLE)(ULONG_PTR)20&&ov->Internal==STATUS_PENDING);cancel_calls++;last=ERROR_NOT_FOUND;return FALSE;}
#define CancelIoEx fake_cancel
#define GetLastError fake_error
#include "../duplex_windows.c"
int main(void){
 sl_win_duplex owners[6]={0};WCHAR names[6][128]={{0}};
 union {max_align_t align;unsigned char bytes[256];} tokens[6]={0},acls[6]={0};sl_win_duplex_completion rows[6][3]={0};sl_win_duplex_settlement settlements[6][3]={0};
 for(mode=0;mode<6;mode++){
  sl_win_duplex *o=&owners[mode];o->name=names[mode];o->name_capacity=128;o->token_body=tokens[mode].bytes;o->token_capacity=256;o->acl_body=acls[mode].bytes;o->acl_capacity=256;o->completions=rows[mode];o->completion_capacity=3;o->settlements=settlements[mode];o->settlement_capacity=3;o->input_bytes=32;o->output_bytes=32;
  completion_calls=client_calls=duplicate_calls=process_calls=cancel_calls=0;last=0;
  if(mode==4)o->user_query.result=77;if(mode==5)o->name=(WCHAR*)((unsigned char*)names[mode]+1);int result=sl_win_construct_duplex(o);
  if(mode>=4){assert(!result&&!process_calls&&!client_calls);if(mode==4)assert(o->user_query.result==77&&!o->attempted);continue;}
  if(mode==3){assert(!result&&o->user_query.returned&&o->token_required==257&&!client_calls&&!o->server);continue;}
  if(mode==2){assert(!result&&o->pending&&o->client==INVALID_HANDLE_VALUE&&o->client_open.returned&&!duplicate_calls);continue;}
  assert(result==2&&o->pending&&o->inherited_client==(HANDLE)(ULONG_PTR)50&&client_calls==1&&duplicate_calls==1);
  if(mode==1){assert(!sl_win_observe_duplex_completion(o)&&o->completion_failed&&o->pending&&rows[mode][0].returned);assert(!sl_win_observe_duplex_completion(o)&&completion_calls==1);assert(!sl_win_request_duplex_cancel(o)&&o->cancel_returned&&o->pending&&!o->operation_settled&&o->cancel_request.last_error==ERROR_NOT_FOUND);assert(sl_win_observe_duplex_settlement(o)==1&&o->operation_settled&&!o->pending&&settlements[mode][0].outcome==2&&settlements[mode][0].macro_after&&o->completion_failed);}
  else{assert(sl_win_observe_duplex_completion(o)==2&&!o->connected&&rows[mode][0].query.last_error==ERROR_IO_INCOMPLETE);assert(sl_win_observe_duplex_completion(o)==1&&o->connected&&!o->pending&&o->completed_by==&rows[mode][1]);assert(rows[mode][1].transferred==0xabc);}
  {unsigned calls=client_calls;assert(!sl_win_construct_duplex(o)&&client_calls==calls);}
 }
 return 0;
}
#endif
