/* UNRUN source observation only; substituted token APIs grant no rights. */
#ifdef _WIN32
#include "../owner_windows.h"
#include <assert.h>
static unsigned mode,opens,queries,statistics;static DWORD error;
static BOOL fixture_open(HANDLE process,DWORD access,PHANDLE token){assert(process==(HANDLE)(LONG_PTR)-1&&access==TOKEN_QUERY);opens++;*token=(HANDLE)(ULONG_PTR)40;error=mode==1?ERROR_ACCESS_DENIED:0;return mode!=1;}
static BOOL fixture_info(HANDLE token,TOKEN_INFORMATION_CLASS kind,LPVOID body,DWORD capacity,PDWORD required){assert(token==(HANDLE)(ULONG_PTR)40);queries++;if(kind==TokenStatistics){statistics++;assert(capacity==sizeof(TOKEN_STATISTICS));*required=sizeof(TOKEN_STATISTICS);((TOKEN_STATISTICS*)body)->ModifiedId.LowPart=statistics;error=(mode==2&&statistics==1)||(mode==4&&statistics==2)?ERROR_ACCESS_DENIED:0;if(mode==5&&statistics==1)*required=1;return !error;}assert(kind==TokenAccessInformation);*required=mode==3?capacity+1:sizeof(TOKEN_ACCESS_INFORMATION);error=mode==3?ERROR_INSUFFICIENT_BUFFER:0;return mode!=3;}
static DWORD fixture_error(void){return error;}
#define OpenProcessToken fixture_open
#define GetTokenInformation fixture_info
#define GetLastError fixture_error
#include "../token_windows.c"
typedef struct {sl_win_token_observation owner;unsigned char bytes[sizeof(TOKEN_ACCESS_INFORMATION)+64];} fixture;
int main(void){fixture cases[6]={0};for(mode=0;mode<6;mode++){fixture *f=&cases[mode];sl_win_token_observation *o=&f->owner;o->backing=f->bytes;o->capacity=sizeof(f->bytes);opens=queries=statistics=0;int r=sl_win_observe_process_token(o,(HANDLE)(LONG_PTR)-1);assert(o->returned&&o->actual_backing==f->bytes&&o->token==(HANDLE)(ULONG_PTR)40&&opens==1);if(!mode)assert(r&&queries==3&&o->before.ModifiedId.LowPart==1&&o->after.ModifiedId.LowPart==2);else {assert(!r);if(mode==1)assert(!queries&&o->open.last_error==ERROR_ACCESS_DENIED);else assert(statistics==2&&o->after_query.returned);if(mode==3)assert(o->required==sizeof(f->bytes)+1&&o->information_query.last_error==ERROR_INSUFFICIENT_BUFFER&&o->after_result);if(mode==4)assert(o->primary_result&&!o->after_result);}
 unsigned prior=queries;o->backing=0;assert(!sl_win_observe_process_token(o,(HANDLE)0)&&queries==prior&&o->actual_backing==f->bytes);
 }return 0;}
#endif