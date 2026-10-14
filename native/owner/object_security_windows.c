#ifdef _WIN32
#include "owner_windows.h"
int sl_win_observe_object_security(sl_win_object_security *o,HANDLE object){
 BOOL result;DWORD error;
 if(!o||o->attempted)return 0;
 o->attempted=1;o->actual_backing=o->backing;o->actual_capacity=o->capacity;o->object_original=object;
 o->requested=OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION;
 if(!object||!o->actual_backing||!o->actual_capacity||o->query.entered||o->query.returned){o->returned=1;return 0;}
 o->query.source="GetKernelObjectSecurity.owner.group.dacl";o->query.subject=o;o->query.entered=1;
 result=GetKernelObjectSecurity(object,o->requested,o->actual_backing,o->actual_capacity,&o->required);
 error=GetLastError();
 o->query.result=(ULONG_PTR)result;o->query.last_error=error;o->query.returned=1;
 /* No retry or allocation from required. A successful observation is merely
  * bytes from this object, never proof of prior/future access exclusion. */
 o->result=result!=FALSE&&o->required&&o->required<=o->actual_capacity;
 o->returned=1;return o->result;
}
#endif