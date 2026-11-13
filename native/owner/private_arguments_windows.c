#ifdef _WIN32
#include "private_arguments_windows.h"
#include <stdint.h>

static int argument_call_fresh(const sl_win_call *call) {
 return !call->source&&!call->subject&&!call->result&&!call->last_error&&!call->entered&&!call->returned;
}
static int argument_overlap(const void *a,SIZE_T an,const void *b,SIZE_T bn) {
 ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;
 if(!a||!b||x>~(ULONG_PTR)0-an||y>~(ULONG_PTR)0-bn)return 1;
 return x<y+bn&&y<x+an;
}
static int argument_units(SIZE_T n,SIZE_T *bytes) {
 if(n>~(SIZE_T)0/sizeof(WCHAR))return 0;*bytes=n*sizeof(WCHAR);return 1;
}
static int argument_extent(const WCHAR *text,SIZE_T capacity,SIZE_T *extent) {
 SIZE_T i;for(i=0;i<capacity;i++){if(text[i]==0){*extent=i;return 1;}}return 0;
}
/* Go readNextArg's delimiter/quote/slash control flow, counting only. UTF16
 * conversion cannot introduce ASCII delimiters from non-ASCII code units. */
static int argument_count(const WCHAR *text,SIZE_T units,SIZE_T *count) {
 SIZE_T i=0,total=0;
 while(i<units){
  SIZE_T slashes=0;unsigned quoted=0;
  if(text[i]==L' '||text[i]==L'\t'){i++;continue;}
  if(total==~(SIZE_T)0)return 0;total++;
  while(i<units){WCHAR c=text[i++];
   if((c==L' '||c==L'\t')&&!quoted)break;
   if(c==L'"'){
    if((slashes&1)==0){if(quoted&&i<units&&text[i]==L'"')i++;quoted=!quoted;}
    slashes=0;continue;
   }
   if(c==L'\\'){if(slashes==~(SIZE_T)0)return 0;slashes++;continue;}
   slashes=0;
  }
 }
 *count=total;return 1;
}
static int argument_put(sl_win_private_arguments *o,WCHAR c) {
 if(o->output_units>=o->actual_output_capacity)return 0;
 o->actual_output[o->output_units++]=c;return 1;
}
static int argument_literal(sl_win_private_arguments *o,const WCHAR *text) {
 while(*text){if(!argument_put(o,*text++))return 0;}return 1;
}
static int argument_decimal(sl_win_private_arguments *o,ULONG_PTR value) {
 /* Three decimal digits per byte is a checked ABI-derived upper bound. */
 WCHAR reversed[sizeof(ULONG_PTR)*3];SIZE_T n=0;
 do{if(n==sizeof(reversed)/sizeof(reversed[0]))return 0;reversed[n++]=(WCHAR)(L'0'+value%10);value/=10;}while(value);
 while(n){if(!argument_put(o,reversed[--n]))return 0;}return 1;
}
static int argument_number(sl_win_private_arguments *o,ULONG_PTR value) {
 return argument_decimal(o,value)&&argument_put(o,L' ');
}
static void argument_le(unsigned char *out,uint64_t value,unsigned bytes) {
 unsigned i;for(i=0;i<bytes;i++){out[i]=(unsigned char)value;value>>=8;}
}
/* Header construction follows complete original acquisition and private output.
 * These are retained original spans, never an echo or a stream authority. */
static int argument_transport(sl_win_private_arguments *o) {
 static const unsigned char magic[8]={'S','L','W','A','R','G','V','1'};
 SIZE_T raw=o->raw_units+1,module=0,total,bytes;unsigned i;
 if(!raw)return 0;
 if(o->empty_fallback){if(o->public_units==~(SIZE_T)0)return 0;module=o->public_units+1;}
 if(raw>~(SIZE_T)0-module)return 0;total=raw+module;
 if(!argument_units(total,&bytes)||sizeof(WCHAR)!=2)return 0;
 for(i=0;i<8;i++)o->transport_header[i]=magic[i];
 argument_le(o->transport_header+8,1,4);argument_le(o->transport_header+12,o->empty_fallback?1:0,4);
 argument_le(o->transport_header+16,raw,8);argument_le(o->transport_header+24,module,8);
 argument_le(o->transport_header+32,o->public_count,8);argument_le(o->transport_header+40,bytes,8);
 o->transport_raw=o->raw_original;o->transport_module=o->empty_fallback?o->public_original:NULL;
 o->transport_raw_units=raw;o->transport_module_units=module;o->transport_body_bytes=bytes;o->transport_prepared=1;
 return 1;
}
static int argument_module(sl_win_private_arguments *o){
 SIZE_T capacity=1024,used=0;
 for(SIZE_T index=0;index<o->actual_module_query_capacity;index++){
  sl_win_module_query *q=&o->actual_module_queries[index];DWORD result,error;
  if(capacity>MAXDWORD||used>o->actual_module_arena_capacity||capacity>o->actual_module_arena_capacity-used){o->exhausted=1;return -1;}
  q->actual_backing=o->actual_module_arena+used;q->actual_capacity=capacity;q->attempted=1;o->module_query_count++;
  q->query.source="GetModuleFileNameW.incremental-original";q->query.subject=q->actual_backing;q->query.entered=1;
  result=GetModuleFileNameW(NULL,q->actual_backing,(DWORD)capacity);error=GetLastError();
  q->returned_units=result;q->query.result=result;q->query.last_error=error;q->query.returned=1;q->returned=1;
  if(result>capacity)return -1;
  if(result==capacity){
   /* Nonzero truncation projects nil error exactly like the genuine Go wrapper.
    * Keep this whole buffer/call and move to a distinct next1024-step slice. */
   used+=capacity;if(capacity>~(SIZE_T)0-1024)return -1;capacity+=1024;continue;
  }
  if(result==0){o->module_failed=1;o->public_original=o->raw_original;o->public_units=0;return 0;}
  if(q->actual_backing[result]!=0)return -1;
  o->actual_module=q->actual_backing;o->actual_module_capacity=capacity;
  o->public_original=q->actual_backing;o->public_units=result;return 0;
 }
 o->exhausted=1;return -1;
}
static int argument_construct_body(sl_win_private_arguments *o) {
 SIZE_T output_bytes,module_bytes,raw_bytes,query_bytes;DWORD error;
 if(!o->output||!o->module||((ULONG_PTR)o->output%_Alignof(WCHAR))||((ULONG_PTR)o->module%_Alignof(WCHAR))||!o->output_capacity||!o->module_capacity||!o->original_capacity||!o->security_label_bytes||!o->terminal_calls||!o->representation_bytes||!o->endpoint_original||o->endpoint_original==INVALID_HANDLE_VALUE||
    !o->module_queries||!o->module_query_capacity||(ULONG_PTR)o->module_queries%_Alignof(sl_win_module_query)||o->module_query_capacity>~(SIZE_T)0/sizeof(sl_win_module_query))return -1;
 if(!argument_units(o->output_capacity,&output_bytes)||!argument_units(o->module_capacity,&module_bytes)||argument_overlap(o->output,output_bytes,o,sizeof(*o))||argument_overlap(o->module,module_bytes,o,sizeof(*o))||argument_overlap(o->output,output_bytes,o->module,module_bytes))return -1;
 query_bytes=o->module_query_capacity*sizeof(sl_win_module_query);
 if(argument_overlap(o->module_queries,query_bytes,o,sizeof(*o))||argument_overlap(o->module_queries,query_bytes,o->output,output_bytes)||argument_overlap(o->module_queries,query_bytes,o->module,module_bytes))return -1;
 for(SIZE_T i=0;i<o->module_query_capacity;i++){
  sl_win_module_query *q=&o->module_queries[i];
  if(q->owner_original||q->actual_backing||q->actual_capacity||q->returned_units||q->attempted||q->returned||!argument_call_fresh(&q->query))return -1;
 }
 o->actual_output=o->output;o->actual_output_capacity=o->output_capacity;
 o->actual_module=o->module;o->actual_module_capacity=o->module_capacity;o->actual_original_capacity=o->original_capacity;
 o->actual_module_arena=o->module;o->actual_module_arena_capacity=o->module_capacity;
 o->actual_module_queries=o->module_queries;o->actual_module_query_capacity=o->module_query_capacity;
 for(SIZE_T i=0;i<o->actual_module_query_capacity;i++)o->actual_module_queries[i].owner_original=o;
 o->command_line_query.source="GetCommandLineW";o->command_line_query.subject=o;o->command_line_query.entered=1;
 o->raw_original=GetCommandLineW();error=GetLastError();
 o->command_line_query.result=(ULONG_PTR)o->raw_original;o->command_line_query.last_error=error;o->command_line_query.returned=1;
 if(!o->raw_original||!argument_extent(o->raw_original,o->actual_original_capacity,&o->raw_units)){o->exhausted=1;return -1;}
 if(!argument_units(o->raw_units+1,&raw_bytes)||argument_overlap(o->raw_original,raw_bytes,o->actual_output,output_bytes)||argument_overlap(o->raw_original,raw_bytes,o->actual_module,module_bytes)||argument_overlap(o->raw_original,raw_bytes,o,sizeof(*o))||argument_overlap(o->raw_original,raw_bytes,o->actual_module_queries,query_bytes))return -1;
 o->public_original=o->raw_original;o->public_units=o->raw_units;
 if(o->raw_units==0){
  o->empty_fallback=1;if(argument_module(o))return -1;
  o->public_count=1;
 }else if(!argument_count(o->raw_original,o->raw_units,&o->public_count))return -1;
 /* Complete source acquisition/count precedes the first output write. */
 if(!argument_literal(o,L"service-lasso-tui-native-payload service-lasso-tui-native-windows-v3 ")||!argument_number(o,(ULONG_PTR)o->endpoint_original)||!argument_number(o,o->public_count)||!argument_number(o,o->security_label_bytes)||!argument_number(o,o->terminal_calls)||!argument_number(o,o->representation_bytes)||!argument_literal(o,L"--")){o->exhausted=1;return -1;}
 if(!argument_put(o,0)){o->exhausted=1;return -1;}if(!argument_transport(o))return -1;return 0;
}
int sl_win_construct_private_arguments(sl_win_private_arguments *o) {
 unsigned i;
 if(!o||o->attempted||o->returned||o->output_units||o->raw_units||o->public_units||o->public_count||o->raw_original||o->public_original||o->actual_output||o->actual_module||o->actual_output_capacity||o->actual_module_capacity||o->actual_original_capacity||o->actual_module_arena||o->actual_module_arena_capacity||o->actual_module_queries||o->actual_module_query_capacity||o->module_query_count||o->empty_fallback||o->module_failed||o->exhausted||o->result||!argument_call_fresh(&o->command_line_query)||o->transport_prepared||o->transport_raw||o->transport_module||o->transport_raw_units||o->transport_module_units||o->transport_body_bytes)return -1;
 for(i=0;i<sizeof(o->transport_header);i++)if(o->transport_header[i])return -1;
 o->attempted=1;o->result=argument_construct_body(o);o->returned=1;return o->result;
}
#endif
