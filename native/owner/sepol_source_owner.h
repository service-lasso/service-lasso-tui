#ifndef SERVICE_LASSO_SEPOL_SOURCE_OWNER_H
#define SERVICE_LASSO_SEPOL_SOURCE_OWNER_H
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
enum sl_sepol_memory_operation {SL_SEPOL_ALLOCATE=1,SL_SEPOL_ZERO_ALLOCATE,SL_SEPOL_REALLOCATE,SL_SEPOL_LOGICAL_FREE,SL_SEPOL_DUPLICATE,SL_SEPOL_BOUNDED_DUPLICATE,SL_SEPOL_REALLOCATE_ARRAY,SL_SEPOL_POINTER_MUTATION,SL_SEPOL_SCALAR_MUTATION};
typedef struct sl_sepol_memory_attempt {
 enum sl_sepol_memory_operation operation;
 const char *source;unsigned line;
 void *input,*result;size_t count,size,extent,offset;
 void *field_original,*old_pointer,*new_pointer;size_t field_width,old_scalar,new_scalar;
 struct sl_sepol_memory_attempt *previous_allocation;
 int error;unsigned entered,returned,logically_released;
} sl_sepol_memory_attempt;
typedef struct sl_sepol_memory_owner {
 unsigned char *bytes,*actual_bytes;size_t capacity,actual_capacity,used;
 sl_sepol_memory_attempt *attempts,*actual_attempts;
 size_t attempt_capacity,actual_attempt_capacity,attempt_count;
 sl_sepol_memory_attempt exhaustion,*first_failure;
 _Atomic unsigned claimed;
 int begin_result;unsigned begin_entered,begin_returned;
 unsigned prepared,active,finished,failed;
} sl_sepol_memory_owner;
/* Actual source constructor initializes claimed with atomic_init BEFORE any
 * borrower. Admission claims it before reading mutable root/slot state.
 * Parser entry must first establish this actual episode. No upstream parser is
 * called by these functions. No physical free/unmap or eligibility is supplied. */
int sl_sepol_memory_begin(sl_sepol_memory_owner *owner);
int sl_sepol_memory_finish(sl_sepol_memory_owner *owner);
/* Same synchronous TLS episode only; failure is absorbing, not disposal. */
int sl_sepol_source_cleanup_failed(void);
sl_sepol_memory_owner *sl_sepol_source_current_owner(void);
/* Source caller establishes SAME active episode or completed immutable parser
 * borrow before this check; no concurrent mutable-owner traversal is allowed. */
int sl_sepol_source_access_original(sl_sepol_memory_owner *original,unsigned mutation);
void *sl_sepol_source_malloc(size_t size,const char *source,unsigned line);
void *sl_sepol_source_calloc(size_t count,size_t size,const char *source,unsigned line);
void *sl_sepol_source_realloc(void *input,size_t size,const char *source,unsigned line);
void sl_sepol_source_free(void *input,const char *source,unsigned line);
char *sl_sepol_source_strdup(const char *input,const char *source,unsigned line);
char *sl_sepol_source_strndup(const char *input,size_t limit,const char *source,unsigned line);
void *sl_sepol_source_reallocarray(void *input,size_t count,size_t size,const char *source,unsigned line);
sl_sepol_memory_attempt *sl_sepol_source_pointer_before(void *field,void *old_pointer,void *new_pointer,size_t width,const char *source,unsigned line);
sl_sepol_memory_attempt *sl_sepol_source_scalar_before(void *field,size_t old_value,size_t new_value,size_t width,const char *source,unsigned line);
/* These source macros are for actual VOID cleanup bodies only. A failed
 * prewrite record returns before mutation; it does not authorize disposal.
 * Typed lvalue access preserves C alias rules and evaluates each value once. */
#define SL_SEPOL_CLEANUP_POINTER(field,value) do{ \
 __auto_type sl_field=&(field);__typeof__(field) sl_value=(value); \
 sl_sepol_memory_attempt *sl_occurrence=sl_sepol_source_pointer_before(sl_field,(void*)*sl_field,(void*)sl_value,sizeof *sl_field,__FILE__,__LINE__); \
 if(!sl_occurrence)return;*sl_field=sl_value;sl_occurrence->returned=1; \
}while(0)
#define SL_SEPOL_CLEANUP_SCALAR(field,value) do{ \
 __auto_type sl_field=&(field);__typeof__(field) sl_value=(value); \
 sl_sepol_memory_attempt *sl_occurrence=sl_sepol_source_scalar_before(sl_field,(size_t)*sl_field,(size_t)sl_value,sizeof *sl_field,__FILE__,__LINE__); \
 if(!sl_occurrence)return;*sl_field=sl_value;sl_occurrence->returned=1; \
}while(0)
#endif