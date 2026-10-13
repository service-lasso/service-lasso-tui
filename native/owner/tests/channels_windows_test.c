#ifdef _WIN32
#include "../channels_windows.h"
#include <assert.h>
static unsigned mode,pipes,duplicates,processes;
static HANDLE fixture_process(void){processes++;return (HANDLE)(ULONG_PTR)90;}
static BOOL fixture_pipe(PHANDLE read,PHANDLE write,LPSECURITY_ATTRIBUTES attributes,DWORD size){
 unsigned index=pipes++;assert(attributes==NULL&&size==64+index);
 *read=(HANDLE)(ULONG_PTR)(100+2*index);*write=(HANDLE)(ULONG_PTR)(101+2*index);
 if(mode==index+1){*write=(HANDLE)(ULONG_PTR)777;return FALSE;}return TRUE;
}
static BOOL fixture_duplicate(HANDLE source_process,HANDLE source,HANDLE target_process,LPHANDLE target,DWORD access,BOOL inherit,DWORD options){
 duplicates++;assert(source_process==(HANDLE)(ULONG_PTR)90&&target_process==source_process&&source==(HANDLE)(ULONG_PTR)100&&access==0&&inherit&&options==DUPLICATE_SAME_ACCESS);
 *target=(HANDLE)(ULONG_PTR)(mode==4?778:200);return mode==4?FALSE:TRUE;
}
static DWORD fixture_error(void){return 55;}
#define GetCurrentProcess fixture_process
#define CreatePipe fixture_pipe
#define DuplicateHandle fixture_duplicate
#define GetLastError fixture_error
#include "../channels_windows.c"
int main(void){
 sl_win_channels cases[6]={0};
 for(mode=0;mode<6;mode++){
  sl_win_channels *o=&cases[mode];for(unsigned i=0;i<3;i++)o->suggested_bytes[i]=64+i;
  if(mode==5)o->create[1].last_error=99;
  pipes=duplicates=processes=0;int result=sl_win_construct_channels(o);
  if(mode==0){assert(result&&o->returned&&o->created_count==3&&o->inherited_gate==(HANDLE)(ULONG_PTR)200&&pipes==3&&duplicates==1);}
  else if(mode==5){assert(!result&&!o->attempted&&!pipes&&!processes&&o->create[1].last_error==99);}
  else if(mode<4){unsigned index=mode-1;assert(!result&&o->returned&&o->created_count==index&&pipes==mode&&!duplicates&&o->write[index]==(HANDLE)(ULONG_PTR)777&&o->create[index].returned&&!o->create[index].result&&o->create[index].last_error==55);}
  else{assert(!result&&o->returned&&o->created_count==3&&o->inherited_gate==(HANDLE)(ULONG_PTR)778&&o->gate_duplicate.returned&&!o->gate_duplicate.result);}
  {unsigned old=pipes+duplicates+processes;assert(!sl_win_construct_channels(o)&&old==pipes+duplicates+processes);}
 }
 return 0;
}
#endif
