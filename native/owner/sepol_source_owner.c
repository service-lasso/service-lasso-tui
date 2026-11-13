#include "sepol_source_owner.h"
#include <errno.h>
#include <string.h>
#include <limits.h>
static _Thread_local sl_sepol_memory_owner *current_memory;
static int memory_range(const void *p,size_t n,uintptr_t *end){
 uintptr_t start=(uintptr_t)p;if(!p||!n||n>UINTPTR_MAX-start)return -1;*end=start+n;return 0;
}
static int memory_overlap(const void *a,size_t n,const void *b,size_t m){
 uintptr_t ae,be;if(memory_range(a,n,&ae)||memory_range(b,m,&be))return 1;
 return (uintptr_t)a<be&&(uintptr_t)b<ae;
}
static int memory_attempt_fresh(const sl_sepol_memory_attempt *a){
 return !(a->operation||a->source||a->line||a->input||a->result||a->count||a->size||a->extent||a->offset||
  a->field_original||a->old_pointer||a->new_pointer||a->field_width||a->old_scalar||a->new_scalar||a->previous_allocation||a->error||a->entered||a->returned||a->logically_released);
}
int sl_sepol_memory_begin(sl_sepol_memory_owner *o){
 size_t slots_bytes;unsigned expected=0;uintptr_t end;
 if(!o||current_memory)return -1;
 /* Actual constructor must initialize this atomic before root publication.
  * Losing claim reads no mutable slot/root fields while the winner works. */
 if(!atomic_compare_exchange_strong(&o->claimed,&expected,1))return -1;
 o->begin_entered=1;o->begin_result=-1;
 if(o->prepared||o->active||o->finished||o->failed||o->used||o->attempt_count||o->first_failure||
    o->actual_bytes||o->actual_capacity||o->actual_attempts||o->actual_attempt_capacity||
    !o->bytes||!o->capacity||!o->attempts||!o->attempt_capacity||
    o->attempt_capacity>SIZE_MAX/sizeof *o->attempts||
    (uintptr_t)o->bytes%_Alignof(max_align_t)||(uintptr_t)o->attempts%_Alignof(sl_sepol_memory_attempt))goto done;
 slots_bytes=o->attempt_capacity*sizeof *o->attempts;
 if(memory_range(o->bytes,o->capacity,&end)||memory_range(o->attempts,slots_bytes,&end)||
    memory_overlap(o->bytes,o->capacity,o,sizeof *o)||memory_overlap(o->attempts,slots_bytes,o,sizeof *o)||
    memory_overlap(o->bytes,o->capacity,o->attempts,slots_bytes))goto done;
 for(size_t i=0;i<o->attempt_capacity;i++)if(!memory_attempt_fresh(&o->attempts[i]))goto done;
 if(!memory_attempt_fresh(&o->exhaustion))goto done;
 o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;
 o->actual_attempts=o->attempts;o->actual_attempt_capacity=o->attempt_capacity;
 o->prepared=o->active=1;current_memory=o;o->begin_result=0;
done:o->begin_returned=1;return o->begin_result;
}
static sl_sepol_memory_attempt *memory_attempt(enum sl_sepol_memory_operation operation,void *input,size_t count,size_t size,const char *source,unsigned line){
 sl_sepol_memory_owner *o=current_memory;
 if(!o||!o->active){errno=ENOMEM;return NULL;}
 if(o->failed){errno=ENOMEM;return NULL;} /* caller must unwind on absorbing loss */
 sl_sepol_memory_attempt *a;
 if(o->attempt_count==o->actual_attempt_capacity){a=&o->exhaustion;o->failed=1;}
 else a=&o->actual_attempts[o->attempt_count++];
 a->operation=operation;a->input=input;a->count=count;a->size=size;a->source=source;a->line=line;a->entered=1;
 if(o->failed){a->error=ENOMEM;a->returned=1;if(!o->first_failure)o->first_failure=a;errno=ENOMEM;return NULL;}
 return a;
}
static void memory_failure(sl_sepol_memory_attempt *a,int error){
 sl_sepol_memory_owner *o=current_memory;a->error=error;a->returned=1;
 /* An ordinary allocation exhaustion may be handled by the library; retain
  * its original either way and deny overall profile even if parsing continues. */
 if(!o->first_failure)o->first_failure=a;errno=error;
}
static void *memory_allocate(sl_sepol_memory_attempt *a,size_t extent,unsigned clear){
 sl_sepol_memory_owner *o=current_memory;size_t alignment=_Alignof(max_align_t),offset=o->used;
 size_t padding=(alignment-offset%alignment)%alignment,charged=extent?extent:1;
 a->extent=extent;
 if(padding>SIZE_MAX-offset||charged>o->actual_capacity||offset+padding>o->actual_capacity-charged){memory_failure(a,ENOMEM);return NULL;}
 offset+=padding;a->offset=offset;a->result=o->actual_bytes+offset;
 o->used=offset+charged;
 if(clear&&extent)memset(a->result,0,extent);
 return a->result;
}
static sl_sepol_memory_attempt *memory_original(void *pointer){
 sl_sepol_memory_owner *o=current_memory;
 for(size_t i=0;i<o->attempt_count;i++){
  sl_sepol_memory_attempt *a=&o->actual_attempts[i];
  if(a->operation!=SL_SEPOL_LOGICAL_FREE&&a->returned&&!a->error&&a->result==pointer)return a;
 }
 return NULL;
}
void *sl_sepol_source_malloc(size_t size,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_ALLOCATE,NULL,1,size,source,line);
 if(!a)return NULL;void *result=memory_allocate(a,size,0);a->returned=1;return result;
}
void *sl_sepol_source_calloc(size_t count,size_t size,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_ZERO_ALLOCATE,NULL,count,size,source,line);
 if(!a)return NULL;if(size&&count>SIZE_MAX/size){memory_failure(a,ENOMEM);return NULL;}
 void *result=memory_allocate(a,count*size,1);a->returned=1;return result;
}
void sl_sepol_source_free(void *input,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_LOGICAL_FREE,input,0,0,source,line);
 if(!a)return;
 if(!input){a->returned=1;return;}
 a->previous_allocation=memory_original(input);
 if(!a->previous_allocation||a->previous_allocation->logically_released){memory_failure(a,EINVAL);current_memory->failed=1;return;}
 a->previous_allocation->logically_released=1;a->returned=1;
 /* SAME physical bytes and allocation root remain alive; library ownership
  * release is recorded separately from eventual authorized native disposal. */
}
static void *memory_reallocate(sl_sepol_memory_attempt *a,size_t size){
 void *input=a->input;
 if(input){
  a->previous_allocation=memory_original(input);
  if(!a->previous_allocation||a->previous_allocation->logically_released){memory_failure(a,EINVAL);current_memory->failed=1;return NULL;}
 }
 if(input&&!size){a->previous_allocation->logically_released=1;a->returned=1;return NULL;}
 void *result=memory_allocate(a,size,0);
 if(result&&a->previous_allocation){
  size_t copied=a->previous_allocation->extent<size?a->previous_allocation->extent:size;
  if(copied)memcpy(result,input,copied);
  a->previous_allocation->logically_released=1;
 }
 a->returned=1;return result;
}
void *sl_sepol_source_realloc(void *input,size_t size,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_REALLOCATE,input,1,size,source,line);
 return a?memory_reallocate(a,size):NULL;
}
void *sl_sepol_source_reallocarray(void *input,size_t count,size_t size,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_REALLOCATE_ARRAY,input,count,size,source,line);
 if(!a)return NULL;if(size&&count>SIZE_MAX/size){memory_failure(a,ENOMEM);return NULL;}
 return memory_reallocate(a,count*size);
}
static char *memory_duplicate(sl_sepol_memory_attempt *a,const char *input,size_t limit,unsigned bounded){
 size_t length=0,scan=current_memory->actual_capacity;
 if(!input){memory_failure(a,EINVAL);current_memory->failed=1;return NULL;}
 if(bounded&&limit<scan)scan=limit;
 /* Actual parser/string callsites must supply their SAME valid string/extent
  * objects. The finite arena bounds work; it does not authenticate a pointer. */
 while(length<scan&&input[length])length++;
 if(length==scan&&(!bounded||scan!=limit)){memory_failure(a,ENOMEM);return NULL;}
 if(length==SIZE_MAX){memory_failure(a,ENOMEM);return NULL;}
 char *result=memory_allocate(a,length+1,0);
 if(result){if(length)memcpy(result,input,length);result[length]=0;}
 a->returned=1;return result;
}
char *sl_sepol_source_strdup(const char *input,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_DUPLICATE,(void*)input,1,0,source,line);
 return a?memory_duplicate(a,input,0,0):NULL;
}
char *sl_sepol_source_strndup(const char *input,size_t limit,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_BOUNDED_DUPLICATE,(void*)input,limit,0,source,line);
 return a?memory_duplicate(a,input,limit,1):NULL;
}
int sl_sepol_memory_finish(sl_sepol_memory_owner *o){
 if(!o||current_memory!=o||!o->active||o->finished)return -1;
 o->active=0;o->finished=1;current_memory=NULL;
 return o->failed||o->first_failure?-1:0;
 /* This is actual synchronous episode return, NOT all parser aliases, worker
  * joins, D2 disposition, physical release or full profile qualification. */
}
sl_sepol_memory_attempt *sl_sepol_source_pointer_before(void *field,void *old_pointer,void *new_pointer,size_t width,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_POINTER_MUTATION,field,1,width,source,line);
 if(!a){
  /* The reserved first exhaustion occurrence retains actual target aliases
   * before the caller returns without changing its typed field. */
  sl_sepol_memory_owner *o=current_memory;
  if(o&&o->exhaustion.operation==SL_SEPOL_POINTER_MUTATION&&!o->exhaustion.field_original){
   o->exhaustion.field_original=field;o->exhaustion.old_pointer=old_pointer;o->exhaustion.new_pointer=new_pointer;o->exhaustion.field_width=width;
  }
  return NULL;
 }
 a->field_original=field;a->old_pointer=old_pointer;a->new_pointer=new_pointer;a->field_width=width;return a;
}
sl_sepol_memory_attempt *sl_sepol_source_scalar_before(void *field,size_t old_value,size_t new_value,size_t width,const char *source,unsigned line){
 sl_sepol_memory_attempt *a=memory_attempt(SL_SEPOL_SCALAR_MUTATION,field,1,width,source,line);
 if(!a){
  sl_sepol_memory_owner *o=current_memory;
  if(o&&o->exhaustion.operation==SL_SEPOL_SCALAR_MUTATION&&!o->exhaustion.field_original){
   o->exhaustion.field_original=field;o->exhaustion.old_scalar=old_value;o->exhaustion.new_scalar=new_value;o->exhaustion.field_width=width;
  }
  return NULL;
 }
 a->field_original=field;a->old_scalar=old_value;a->new_scalar=new_value;a->field_width=width;return a;
}
int sl_sepol_source_cleanup_failed(void){return !current_memory||!current_memory->active||current_memory->failed||current_memory->first_failure;}

sl_sepol_memory_owner *sl_sepol_source_current_owner(void){return sl_sepol_source_cleanup_failed()?NULL:current_memory;}
int sl_sepol_source_access_original(sl_sepol_memory_owner *original,unsigned mutation){
 if(sl_sepol_source_cleanup_failed()||!original||!original->prepared||!original->begin_returned||original->begin_result||original->failed||original->first_failure)return -1;
 if(original==current_memory)return original->active&&!original->finished?0:-1;
 return !mutation&&original->finished&&!original->active?0:-1;
}