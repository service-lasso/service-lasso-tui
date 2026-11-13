#ifndef SERVICE_LASSO_PUBLIC_INPUTS_LINUX_H
#define SERVICE_LASSO_PUBLIC_INPUTS_LINUX_H
#include <stddef.h>
typedef struct sl_linux_public_string {const char *bytes;unsigned long units;} sl_linux_public_string;
typedef struct sl_linux_public_input_source {
 int argc;char *const *argv,*const *environment;
 unsigned long argument_capacity,environment_capacity,argument_bytes,environment_bytes;
} sl_linux_public_input_source;
typedef struct sl_linux_public_inputs {
 const sl_linux_public_input_source *source_original;
 sl_linux_public_string *arguments,*environment;
 const sl_linux_public_input_source *actual_source;
 sl_linux_public_input_source source_snapshot;
 sl_linux_public_string *actual_arguments,*actual_environment;
 unsigned long argument_count,environment_count,argument_extent,environment_extent;
 long result;unsigned attempted,returned;
} sl_linux_public_inputs;
_Static_assert(sizeof(sl_linux_public_string)==16,"original public string view");
_Static_assert(sizeof(sl_linux_public_input_source)==56,"original public input source");
_Static_assert(sizeof(sl_linux_public_inputs)==152&&offsetof(sl_linux_public_inputs,actual_source)==24,"retained public original owner geometry");
int sl_linux_public_input_source_excludes_output(const sl_linux_public_input_source *source,const void *output,unsigned long extent);
long sl_linux_capture_entry_public_inputs(sl_linux_public_inputs *original,sl_linux_public_input_source *source,int argc,char **argv,char **environment);
long sl_linux_capture_public_inputs(sl_linux_public_inputs *original);
int sl_linux_public_inputs_match_original(const sl_linux_public_inputs *original);
int sl_linux_public_inputs_exclude_output(const sl_linux_public_inputs *original,const void *output,unsigned long extent);
/* units includes original NUL; no copied strings or pointer-as-authority.
 * Exact native entry and full physical parent/child lifetimes remain required. */
#endif