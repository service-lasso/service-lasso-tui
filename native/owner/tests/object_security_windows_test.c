/* UNRUN Windows source fixture; substituted API is not real SD/access proof. */
#ifdef _WIN32
#include "../owner_windows.h"
#include <assert.h>
static unsigned calls;static BOOL response;static DWORD needed,error_value;
static BOOL fixture_query(HANDLE h,SECURITY_INFORMATION information,PSECURITY_DESCRIPTOR buffer,DWORD capacity,LPDWORD required){
 assert(h==(HANDLE)(LONG_PTR)-1);assert(information==(OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION));assert(buffer&&capacity==64);calls++;*required=needed;if(response&&needed<=capacity)((unsigned char*)buffer)[0]=9;return response;
}
static DWORD fixture_error(void){return error_value;}
#define GetKernelObjectSecurity fixture_query
#define GetLastError fixture_error
#include "../object_security_windows.c"
int main(void){sl_win_object_security rows[4]={0};unsigned char backing[4][64]={{0}};
 for(unsigned i=0;i<4;i++){rows[i].backing=backing[i];rows[i].capacity=64;response=i!=1;needed=i==1?65:i==2?0:32;error_value=i==1?ERROR_INSUFFICIENT_BUFFER:17;
 unsigned before=calls;int result=sl_win_observe_object_security(&rows[i],(HANDLE)(LONG_PTR)-1);
 assert(result==(i==0||i==3)&&calls==before+1&&rows[i].query.returned&&rows[i].query.last_error==error_value&&rows[i].required==needed&&rows[i].actual_backing==backing[i]&&rows[i].object_original==(HANDLE)(LONG_PTR)-1);
 rows[i].backing=backing[(i+1)%4];rows[i].capacity=1;
 assert(!sl_win_observe_object_security(&rows[i],(HANDLE)4)&&calls==before+1&&rows[i].actual_backing==backing[i]&&rows[i].actual_capacity==64);
 }
 assert(backing[0][0]==9&&!backing[1][0]);return 0;
}
#endif