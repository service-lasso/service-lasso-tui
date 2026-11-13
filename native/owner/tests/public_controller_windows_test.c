/* UNRUN actual Windows owning caller with explicitly controlled birth/native IO. */
#ifdef _WIN32
#include <assert.h>
#include <string.h>
#include "../owner_windows.h"
static unsigned mode,stream_calls,birth_calls,wait_calls,family_calls;
static int controlled_streams(sl_win_entry *e){stream_calls++;e->public_streams_attempted=1;e->public_streams_prepared=mode!=1;return mode!=1;}
static int controlled_birth(sl_win_entry *e){assert(e->public_streams_prepared);birth_calls++;e->birth.attempted=1;e->birth.process.hProcess=(HANDLE)1;e->birth.job=(HANDLE)2;return mode!=2;}
static DWORD WINAPI controlled_wait(HANDLE h,DWORD ms){assert(h==(HANDLE)1&&!ms);wait_calls++;return WAIT_TIMEOUT;}
static BOOL WINAPI controlled_exit(HANDLE h,LPDWORD code){(void)h;(void)code;assert(0);return FALSE;}
static BOOL WINAPI controlled_family(HANDLE h,JOBOBJECTINFOCLASS kind,LPVOID body,DWORD bytes,LPDWORD returned){assert(h==(HANDLE)2&&kind==JobObjectBasicAccountingInformation);family_calls++;memset(body,0,bytes);*returned=bytes;return TRUE;}
static DWORD WINAPI controlled_error(void){return 87;}
#define sl_win_prepare_public_streams controlled_streams
#define sl_win_prepare_entry_birth controlled_birth
#define WaitForSingleObject controlled_wait
#define GetExitCodeProcess controlled_exit
#define QueryInformationJobObject controlled_family
#define GetLastError controlled_error
#include "../controller_windows.c"
typedef struct controller_case {sl_win_public_controller owner;sl_win_service_observation rows[2],replacement[2];} controller_case;
int main(void){
 static controller_case cases[7];
 for(mode=0;mode<7;mode++){
  controller_case *e=&cases[mode];sl_win_public_controller *o=&e->owner;o->services=e->rows;o->service_capacity=2;stream_calls=birth_calls=wait_calls=family_calls=0;
  if(mode==3)e->rows[1].attempted=1;if(mode==6)e->rows[1].root.wait.entered=1;if(mode==4)o->services=(sl_win_service_observation*)o;
  int result=sl_win_construct_public_controller(o);assert(o->attempted&&o->returned&&result==(mode==0||mode==5));
  if(mode>=3&&mode!=5){if(mode==6)assert(e->rows[1].root.wait.entered==1&&!e->rows[1].root.wait.returned);assert(!stream_calls&&!birth_calls&&!o->services_prepared);assert(!sl_win_service_public_controller(o)&&!wait_calls&&!family_calls);continue;}
  assert(o->services_prepared&&stream_calls==1&&birth_calls==(mode!=1));
  if(mode==5){o->services=e->replacement;o->service_capacity=1;}
  for(unsigned i=0;i<2;i++){int serviced=sl_win_service_public_controller(o);assert(serviced==(mode!=1));assert(o->last_service==&e->rows[i]&&e->rows[i].returned&&e->rows[i].actual_birth==&o->entry.birth);}
  assert(o->service_count==2&&!sl_win_service_public_controller(o)&&o->service_exhausted);assert(wait_calls==(mode==1?0:2)&&family_calls==(mode==1?0:2));
  sl_win_public_controller original;memcpy(&original,o,sizeof(original));assert(!sl_win_construct_public_controller(o)&&!memcmp(&original,o,sizeof(original))&&stream_calls==1);
 }
 return 0;
}
#endif