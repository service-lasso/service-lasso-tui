#ifdef _WIN32
#include "../private_arguments_windows.h"
#include <assert.h>
#include <string.h>
static const WCHAR *fixture_raw;
static const WCHAR *fixture_module=L"public module.exe";
static unsigned command_calls,module_calls;
static DWORD fixture_error,module_error;
static LPWSTR fixture_command(void){command_calls++;return (LPWSTR)fixture_raw;}
static DWORD fixture_module_name(HMODULE handle,LPWSTR out,DWORD capacity){
 SIZE_T n=0;assert(handle==NULL);module_calls++;
 if(module_error)return 0;
 while(fixture_module[n])n++;
 if(n>=capacity){fixture_error=ERROR_INSUFFICIENT_BUFFER;return capacity;}
 fixture_error=0;
 memcpy(out,fixture_module,(n+1)*sizeof(WCHAR));return (DWORD)n;
}
static DWORD fixture_last_error(void){return fixture_error;}
#define GetCommandLineW fixture_command
#define GetModuleFileNameW fixture_module_name
#define GetLastError fixture_last_error
#include "../private_arguments_windows.c"
int main(void){
 static const WCHAR ordinary[]={L'"',L'p',L' ',L'q',L'"',L' ',0xd800,L'"',0xdc00,L'"',0};
 static const WCHAR empty[]={0},spaces[]={L' ',L'\t',0};
 sl_win_private_arguments owners[12]={0};WCHAR outputs[12][512]={{0}},modules[12][3072]={{0}};sl_win_module_query queries[12][2]={0};
 for(unsigned i=0;i<12;i++){
  sl_win_private_arguments *o=&owners[i];o->output=outputs[i];o->output_capacity=512;o->module=modules[i];o->module_capacity=3072;o->module_queries=queries[i];o->module_query_capacity=2;o->original_capacity=64;
  o->endpoint_original=(HANDLE)(ULONG_PTR)42;o->security_label_bytes=128;o->terminal_calls=4;o->representation_bytes=256;
 }
 fixture_raw=ordinary;assert(sl_win_construct_private_arguments(&owners[0])==0);assert(owners[0].public_count==2&&module_calls==0);
 assert(owners[0].actual_output[owners[0].output_units-2]==L'-'&&owners[0].actual_output[owners[0].output_units-3]==L'-');
 assert(owners[0].raw_original==ordinary&&owners[0].command_line_query.returned);
 assert(owners[0].transport_prepared&&memcmp(owners[0].transport_header,"SLWARGV1",8)==0&&owners[0].transport_header[8]==1&&owners[0].transport_header[12]==0);
 assert(owners[0].transport_raw==ordinary&&owners[0].transport_raw_units==sizeof(ordinary)/sizeof(WCHAR)&&owners[0].transport_module==NULL&&owners[0].transport_module_units==0);
 fixture_raw=spaces;assert(sl_win_construct_private_arguments(&owners[1])==0&&owners[1].public_count==0&&!owners[1].empty_fallback);
 fixture_raw=empty;assert(sl_win_construct_private_arguments(&owners[2])==0&&owners[2].public_count==1&&owners[2].empty_fallback&&queries[2][0].query.returned);
 assert(owners[2].public_original==modules[2]&&owners[2].public_units==17);
 assert(owners[2].transport_header[12]==1&&owners[2].transport_raw_units==1&&owners[2].transport_module==modules[2]&&owners[2].transport_module_units==18&&owners[2].transport_body_bytes==38);
 module_error=1;fixture_error=5;assert(sl_win_construct_private_arguments(&owners[3])==0&&owners[3].module_failed&&owners[3].public_count==1&&owners[3].public_units==0&&queries[3][0].query.last_error==5);
 assert(owners[3].actual_output[owners[3].output_units-3]==L'-'&&owners[3].actual_output[owners[3].output_units-2]==L'-');
 module_error=0;fixture_error=0;owners[4].module_capacity=2;assert(sl_win_construct_private_arguments(&owners[4])<0&&owners[4].exhausted&&owners[4].output_units==0&&!queries[4][0].query.entered);
 fixture_raw=ordinary;owners[5].output_capacity=2;assert(sl_win_construct_private_arguments(&owners[5])<0&&owners[5].exhausted&&owners[5].output_units==2);
 {unsigned calls=command_calls;SIZE_T used=owners[5].output_units;assert(sl_win_construct_private_arguments(&owners[5])<0&&command_calls==calls&&owners[5].output_units==used);}
 {static WCHAR maximum_original[32767];SIZE_T i;for(i=0;i<32766;i++)maximum_original[i]=L'p';maximum_original[32766]=0;
  fixture_raw=maximum_original;owners[6].original_capacity=32767;
  assert(sl_win_construct_private_arguments(&owners[6])==0&&owners[6].raw_units==32766&&owners[6].public_count==1);
  assert(owners[6].raw_original==maximum_original&&owners[6].transport_body_bytes==65534&&owners[6].transport_header[40]==254&&owners[6].transport_header[41]==255&&owners[6].output_units<512&&maximum_original[32765]==L'p'&&maximum_original[32766]==0);
 }
 {unsigned calls=command_calls;owners[7].command_line_query.result=77;queries[7][0].query.source="retained module query";
  assert(sl_win_construct_private_arguments(&owners[7])<0&&command_calls==calls&&!owners[7].attempted&&owners[7].command_line_query.result==77&&queries[7][0].query.source!=NULL);
 }
 {unsigned calls=command_calls;owners[8].output=(WCHAR *)((unsigned char *)outputs[8]+1);
  assert(sl_win_construct_private_arguments(&owners[8])<0&&command_calls==calls&&owners[8].output_units==0&&!owners[8].command_line_query.entered);
 }
 {unsigned calls=command_calls;owners[9].module=(WCHAR *)((unsigned char *)modules[9]+1);
  assert(sl_win_construct_private_arguments(&owners[9])<0&&command_calls==calls&&owners[9].output_units==0&&!owners[9].command_line_query.entered);
 }
 /* UNRUN: genuine constructor shape, not selected provider/native evidence. */
 {static WCHAR long_module[1501];for(unsigned i=0;i<1500;i++)long_module[i]=L'm';long_module[1500]=0;
  fixture_raw=empty;fixture_module=long_module;
  assert(sl_win_construct_private_arguments(&owners[10])==0&&owners[10].module_query_count==2);
  assert(queries[10][0].actual_capacity==1024&&queries[10][0].returned_units==1024&&queries[10][0].query.last_error==ERROR_INSUFFICIENT_BUFFER);
  assert(queries[10][1].actual_capacity==2048&&queries[10][1].returned_units==1500&&queries[10][1].query.last_error==0);
  assert(queries[10][0].actual_backing==modules[10]&&queries[10][1].actual_backing==modules[10]+1024&&owners[10].public_original==queries[10][1].actual_backing);
  owners[11].module_query_capacity=1;
  assert(sl_win_construct_private_arguments(&owners[11])<0&&owners[11].exhausted&&owners[11].module_query_count==1&&queries[11][0].returned_units==1024&&!queries[11][1].attempted);
 }
 return 0;
}
#endif
