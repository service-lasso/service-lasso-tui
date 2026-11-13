/* UNRUN controlled actual Windows controller bodies; no native qualification. */
#ifdef _WIN32
#include <assert.h>
#include <string.h>
#include "../owner_windows.h"
static DWORD selected_wait,selected_error,selected_code;static BOOL selected_family;static unsigned waits,exits,families;
static DWORD WINAPI controlled_wait(HANDLE h,DWORD timeout){assert(h==(HANDLE)1&&!timeout);waits++;return selected_wait;}
static BOOL WINAPI controlled_exit(HANDLE h,LPDWORD code){assert(h==(HANDLE)1);exits++;*code=selected_code;return TRUE;}
static BOOL WINAPI controlled_family(HANDLE h,JOBOBJECTINFOCLASS kind,LPVOID body,DWORD bytes,LPDWORD returned){assert(h==(HANDLE)2&&kind==JobObjectBasicAccountingInformation&&bytes==sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION));families++;((JOBOBJECT_BASIC_ACCOUNTING_INFORMATION*)body)->ActiveProcesses=1;*returned=bytes;return selected_family;}
static DWORD WINAPI controlled_error(void){return selected_error;}
#define WaitForSingleObject controlled_wait
#define GetExitCodeProcess controlled_exit
#define QueryInformationJobObject controlled_family
#define GetLastError controlled_error
static int unused_stream_constructor(sl_win_entry *entry){(void)entry;assert(0);return 0;}
static int unused_birth_constructor(sl_win_entry *entry){(void)entry;assert(0);return 0;}
#define sl_win_prepare_public_streams unused_stream_constructor
#define sl_win_prepare_entry_birth unused_birth_constructor
#include "../controller_windows.c"
int main(void){
 static sl_win_service_observation observations[5];static sl_win_birth births[5];
 for(unsigned mode=0;mode<5;mode++){
  sl_win_service_observation *o=&observations[mode];births[mode].process.hProcess=(HANDLE)1;births[mode].job=(HANDLE)2;o->birth_original=&births[mode];
  waits=exits=families=0;selected_wait=mode==1?WAIT_FAILED:mode==2?WAIT_OBJECT_0:WAIT_TIMEOUT;selected_family=mode!=3;selected_error=87;selected_code=259;if(mode==4)births[mode].process.hProcess=NULL;
  int result=sl_win_service_sources(o);assert(o->attempted&&o->returned&&o->actual_birth==&births[mode]&&families==1);
  assert(result==(mode==0||mode==2));assert(o->family_observed==(mode!=3));assert(o->root_ready==(mode==2));
  if(mode==1)assert(o->root.wait.result==WAIT_FAILED&&o->root.wait.last_error==87&&!exits);
  if(mode==2)assert(o->root.code==259&&o->family.accounting.ActiveProcesses==1);
  if(mode==4)assert(!waits&&!o->root.attempted&&o->family.query.returned);
  sl_win_service_observation original;memcpy(&original,o,sizeof(original));unsigned old_waits=waits,old_families=families;assert(!sl_win_service_sources(o)&&!memcmp(&original,o,sizeof(original))&&waits==old_waits&&families==old_families);
 }
 return 0;
}
#endif