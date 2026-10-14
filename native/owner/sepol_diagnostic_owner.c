#include "sepol_diagnostic_owner.h"
#include "sepol_source_owner.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
static _Thread_local sl_sepol_diagnostic_owner *diagnostics;
static int diagnostic_overlap(const void *a,size_t n,const void *b,size_t m){
 uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
 if(!a||!b||!n||!m||n>UINTPTR_MAX-aa||m>UINTPTR_MAX-bb)return 1;
 return aa<bb+m&&bb<aa+n;
}
int sl_sepol_diagnostics_begin(sl_sepol_diagnostic_owner *o){
 unsigned expected=0;if(!o||diagnostics)return -1;
 if(!atomic_compare_exchange_strong(&o->claimed,&expected,1))return -1;
 o->begin_entered=1;o->begin_result=-1;
 if(o->actual_memory||o->active||o->finished||o->exhausted||o->count||o->first_failure||o->actual_frames||o->actual_capacity||o->actual_format_capacity||
    !o->frames||!o->capacity||!o->format_capacity||o->capacity>SIZE_MAX/sizeof *o->frames)goto done;
 if(diagnostic_overlap(o,sizeof *o,o->frames,o->capacity*sizeof *o->frames))goto done;
 for(size_t i=0;i<o->capacity;i++){
  sl_sepol_diagnostic *f=&o->frames[i];
  if(f->entered||f->returned||f->actual_arguments||f->actual_output||f->argument_count||!f->arguments||!f->argument_capacity||!f->output||!f->output_capacity||f->output_capacity>INT_MAX)goto done;
  if(f->argument_capacity>SIZE_MAX/sizeof *f->arguments)goto done;
  size_t bytes=f->argument_capacity*sizeof *f->arguments;
  if(diagnostic_overlap(f->arguments,bytes,o,sizeof *o)||diagnostic_overlap(f->output,f->output_capacity,o,sizeof *o)||
     diagnostic_overlap(f->arguments,bytes,o->frames,o->capacity*sizeof *o->frames)||
     diagnostic_overlap(f->output,f->output_capacity,o->frames,o->capacity*sizeof *o->frames)||
     diagnostic_overlap(f->arguments,bytes,f->output,f->output_capacity))goto done;
  for(size_t j=0;j<i;j++){
   sl_sepol_diagnostic *prior=&o->frames[j];size_t prior_bytes=prior->argument_capacity*sizeof *prior->arguments;
   if(diagnostic_overlap(f->arguments,bytes,prior->arguments,prior_bytes)||diagnostic_overlap(f->arguments,bytes,prior->output,prior->output_capacity)||
      diagnostic_overlap(f->output,f->output_capacity,prior->arguments,prior_bytes)||diagnostic_overlap(f->output,f->output_capacity,prior->output,prior->output_capacity))goto done;
  }
  for(size_t j=0;j<f->argument_capacity;j++){sl_sepol_argument *a=&f->arguments[j];if(a->kind||a->referent_owner||a->referent_original||a->referent_copy||a->referent_extent||a->referent_limit||a->referent_copy_extent||a->referent_result||a->precision_present||a->referent_entered||a->referent_returned||a->referent_terminated)goto done;}
 }
 if(o->exhaustion.entered||o->exhaustion.returned)goto done;
 o->actual_frames=o->frames;o->actual_capacity=o->capacity;o->actual_format_capacity=o->format_capacity;
 for(size_t i=0;i<o->actual_capacity;i++){
  sl_sepol_diagnostic *f=&o->actual_frames[i];f->actual_arguments=f->arguments;f->actual_argument_capacity=f->argument_capacity;
  f->actual_output=f->output;f->actual_output_capacity=f->output_capacity;
 }
 o->actual_memory=sl_sepol_source_current_owner();diagnostics=o;o->active=1;o->begin_result=0;
 done:o->begin_returned=1;return o->begin_result;
}
static sl_sepol_argument *argument(sl_sepol_diagnostic *f,size_t at,enum sl_sepol_argument_kind kind){
 if(f->argument_count==f->actual_argument_capacity)return NULL;
 sl_sepol_argument *a=&f->actual_arguments[f->argument_count++];a->kind=kind;a->format_offset=at;return a;
}
static int diagnostic_string(sl_sepol_argument *a,int precision){
 a->referent_entered=1;a->referent_result=-1;a->referent_original=a->value.string;
 a->precision_present=precision>=0;a->referent_limit=precision>=0?(size_t)precision:SIZE_MAX;
 sl_sepol_memory_owner *m=diagnostics?diagnostics->actual_memory:NULL;a->referent_owner=m;
 if(!a->value.string){a->referent_result=0;goto done;}
 if(!m||!m->active)goto done;
 size_t bound=m->actual_capacity;if(a->precision_present&&a->referent_limit<bound)bound=a->referent_limit;
 while(a->referent_extent<bound){char value=a->value.string[a->referent_extent++];if(!value){a->referent_terminated=1;break;}}
 if(!a->referent_terminated&&(!a->precision_present||a->referent_extent!=a->referent_limit))goto done;
 if(a->referent_extent){a->referent_copy=sl_sepol_source_malloc(a->referent_extent,__FILE__,__LINE__);if(!a->referent_copy)goto done;memcpy(a->referent_copy,a->referent_original,a->referent_extent);a->referent_copy_extent=a->referent_extent;}
 a->referent_result=0;
 done:a->referent_returned=1;return a->referent_result;
}
#define TAKE(kind,field,type) do{if(validation_count==f->actual_argument_capacity)return -1;validation_count++;if(ap){sl_sepol_argument *a=argument(f,start,(kind));if(!a)return -1;a->value.field=va_arg(*ap,type);}}while(0)
static int diagnostic_arguments(sl_sepol_diagnostic *f,va_list *ap){
 const char *s=f->format;size_t validation_count=0;
 for(size_t at=0;at<f->format_length;at++){
  if(s[at]!='%')continue;size_t start=at;at++;if(at==f->format_length)return -1;if(s[at]=='%')continue;
  while(at<f->format_length&&(s[at]=='#'||s[at]=='0'||s[at]=='-'||s[at]==' '||s[at]=='+'||s[at]=='\''))at++;
  if(at<f->format_length&&s[at]=='*'){TAKE(SL_ARG_INT,i,int);at++;}
  else while(at<f->format_length&&s[at]>='0'&&s[at]<='9')at++;
  int precision=-1;
  if(at<f->format_length&&s[at]=='.'){
   at++;precision=0;if(at<f->format_length&&s[at]=='*'){TAKE(SL_ARG_INT,i,int);if(ap)precision=f->actual_arguments[f->argument_count-1].value.i;at++;}
   else while(at<f->format_length&&s[at]>='0'&&s[at]<='9'){if(precision>(INT_MAX-(s[at]-'0'))/10)return -1;precision=precision*10+s[at]-'0';at++;}
  }
  int length=0;
  if(at<f->format_length){
   switch(s[at]){
    case 'h':length=1;at++;if(at<f->format_length&&s[at]=='h'){length=2;at++;}break;
    case 'l':length=3;at++;if(at<f->format_length&&s[at]=='l'){length=4;at++;}break;
    case 'j':length=5;at++;break;case 'z':length=6;at++;break;case 't':length=7;at++;break;case 'L':length=8;at++;break;
   }
  }
  if(at==f->format_length)return -1;char c=s[at];
  if(c=='d'||c=='i'){
   if(length<=2)TAKE(SL_ARG_INT,i,int);else if(length==3)TAKE(SL_ARG_LONG,l,long);
   else if(length==4)TAKE(SL_ARG_LLONG,ll,long long);else if(length==5)TAKE(SL_ARG_INTMAX,im,intmax_t);
   else if(length==6)TAKE(SL_ARG_SSIZE,signed_size,ssize_t);else if(length==7)TAKE(SL_ARG_PTRDIFF,difference,ptrdiff_t);else return -1;
  }else if(c=='u'||c=='o'||c=='x'||c=='X'){
   if(length==1||length==2)TAKE(SL_ARG_INT,i,int);else if(!length)TAKE(SL_ARG_UINT,u,unsigned);else if(length==3)TAKE(SL_ARG_ULONG,ul,unsigned long);
   else if(length==4)TAKE(SL_ARG_ULLONG,ull,unsigned long long);else if(length==5)TAKE(SL_ARG_UINTMAX,um,uintmax_t);
   else if(length==6)TAKE(SL_ARG_SIZE,size,size_t);else return -1;
  }else if(c=='s'){
   if(!length){TAKE(SL_ARG_STRING,string,const char*);if(ap)diagnostic_string(&f->actual_arguments[f->argument_count-1],precision);}else if(length==3){TAKE(SL_ARG_WSTRING,wide,const wchar_t*);if(ap){sl_sepol_argument *a=&f->actual_arguments[f->argument_count-1];a->referent_original=a->value.wide;a->referent_entered=a->referent_returned=1;a->referent_result=-1;}}else return -1;
  }else if(c=='c'){
   if(!length)TAKE(SL_ARG_INT,i,int);else if(length==3)TAKE(SL_ARG_WINT,wi,wint_t);else return -1;
  }else if(c=='p'&&!length){TAKE(SL_ARG_POINTER,pointer,void*);
  }else if(c=='f'||c=='F'||c=='e'||c=='E'||c=='g'||c=='G'||c=='a'||c=='A'){
   if(length==8)TAKE(SL_ARG_LDOUBLE,ld,long double);else if(!length||length==3)TAKE(SL_ARG_DOUBLE,d,double);else return -1;
  }else if(c=='m'&&!length){/* glibc errno formatting: actual entry errno below */
  }else return -1; /* positional formats, %n and unknown ABI stay unresolved */
 }
 return 0;
}
#undef TAKE
void sl_sepol_diagnostic_emit(sepol_handle_t *handle,int level,const char *channel,const char *function,const char *format,...){
 int entry_errno=errno;sl_sepol_diagnostic_owner *o=diagnostics;
 if(!o||!o->active){errno=entry_errno;return;} /* linked caller must establish actual owner before entry */
 if(o->exhausted){errno=entry_errno;return;}
 sl_sepol_diagnostic *f;
 if(o->count==o->actual_capacity){f=&o->exhaustion;o->exhausted=1;}else f=&o->actual_frames[o->count++];
 f->handle_original=handle;f->level=level;f->channel=channel;f->function=function;f->format=format;f->entry_errno=entry_errno;f->entered=1;f->result=-1;
 if(o->exhausted||!format)goto done;
 while(f->format_length<o->actual_format_capacity&&format[f->format_length])f->format_length++;
 if(f->format_length==o->actual_format_capacity)goto done;
 /* Validate complete lexical ABI/capacity before ANY va_arg. Positional or
  * truncated star formats must not consume a nonexistent/wrong-type value. */
 if(diagnostic_arguments(f,NULL))goto done;
 va_list original,inspection;va_start(original,format);va_copy(inspection,original);
 int result=diagnostic_arguments(f,&inspection);va_end(inspection);
 if(!result){
  f->arguments_complete=1;errno=entry_errno;f->format_entered=1;
  f->format_result=vsnprintf(f->actual_output,f->actual_output_capacity,format,original);
  f->format_errno=errno;f->format_returned=1;
  if(f->format_result>=0&&(size_t)f->format_result<f->actual_output_capacity)f->result=0;
  for(size_t i=0;i<f->argument_count;i++){sl_sepol_argument *a=&f->actual_arguments[i];if((a->kind==SL_ARG_STRING||a->kind==SL_ARG_WSTRING)&&(!a->referent_returned||a->referent_result))f->result=-1;}
 }
 va_end(original);
 done:f->returned=1;if(f->result&&!o->first_failure)o->first_failure=f;errno=entry_errno;
 /* Formatting return is not public forwarding, referent disposition or final
  * archive acceptance. SAME typed pointer referents remain separately owned. */
}
int sl_sepol_diagnostics_finish(sl_sepol_diagnostic_owner *o){
 if(!o||diagnostics!=o||!o->active||o->finished)return -1;
 o->active=0;o->finished=1;diagnostics=NULL;return o->exhausted||o->first_failure?-1:0;
}