#ifndef SERVICE_LASSO_SEPOL_DIAGNOSTIC_OWNER_H
#define SERVICE_LASSO_SEPOL_DIAGNOSTIC_OWNER_H
#include <sepol/handle.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <wchar.h>
#include <sys/types.h>
/* Argument cells are actual va_arg values and pointer aliases, not a saved
 * va_list usable after its invocation. Referent custody remains required. */
enum sl_sepol_argument_kind {SL_ARG_INT=1,SL_ARG_UINT,SL_ARG_LONG,SL_ARG_ULONG,SL_ARG_LLONG,SL_ARG_ULLONG,SL_ARG_INTMAX,SL_ARG_UINTMAX,SL_ARG_SIZE,SL_ARG_PTRDIFF,SL_ARG_DOUBLE,SL_ARG_LDOUBLE,SL_ARG_POINTER,SL_ARG_STRING,SL_ARG_WSTRING,SL_ARG_WINT,SL_ARG_SSIZE};
struct sl_sepol_memory_owner;
typedef struct sl_sepol_argument {
 enum sl_sepol_argument_kind kind;size_t format_offset;
 struct sl_sepol_memory_owner *referent_owner;const void *referent_original;unsigned char *referent_copy;
 size_t referent_extent,referent_limit,referent_copy_extent;int referent_result;
 unsigned precision_present,referent_entered,referent_returned,referent_terminated;
 union {int i;unsigned u;long l;unsigned long ul;long long ll;unsigned long long ull;intmax_t im;uintmax_t um;size_t size;ssize_t signed_size;ptrdiff_t difference;double d;long double ld;void *pointer;const char *string;const wchar_t *wide;wint_t wi;} value;
} sl_sepol_argument;
typedef struct sl_sepol_diagnostic {
 sepol_handle_t *handle_original;const char *channel,*function,*format;
 int level,entry_errno,format_result,format_errno;
 sl_sepol_argument *arguments,*actual_arguments;size_t argument_capacity,actual_argument_capacity,argument_count;
 char *output,*actual_output;size_t output_capacity,actual_output_capacity,format_length;
 unsigned entered,arguments_complete,format_entered,format_returned,returned;
 int result;
} sl_sepol_diagnostic;
typedef struct sl_sepol_diagnostic_owner {
 struct sl_sepol_memory_owner *actual_memory;
 sl_sepol_diagnostic *frames,*actual_frames;size_t capacity,actual_capacity,count,format_capacity,actual_format_capacity;
 _Atomic unsigned claimed;unsigned begin_entered,begin_returned,active,finished,exhausted;
 int begin_result;sl_sepol_diagnostic *first_failure;
 /* Reserved actual first over-capacity invocation, with no fake va_list. */
 sl_sepol_diagnostic exhaustion;
} sl_sepol_diagnostic_owner;
int sl_sepol_diagnostics_begin(sl_sepol_diagnostic_owner *owner);
int sl_sepol_diagnostics_finish(sl_sepol_diagnostic_owner *owner);
void sl_sepol_diagnostic_emit(sepol_handle_t *handle,int level,const char *channel,const char *function,const char *format,...);
#endif