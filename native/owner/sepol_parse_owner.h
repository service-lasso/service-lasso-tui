#ifndef SERVICE_LASSO_SEPOL_PARSE_OWNER_H
#define SERVICE_LASSO_SEPOL_PARSE_OWNER_H
#include "sepol_source_owner.h"
#include "sepol_diagnostic_owner.h"
#include <sepol/policydb/policydb.h>
#include "../../third_party/libsepol/src/handle.h"
typedef struct sl_sepol_parse_owner {
 unsigned char *body,*actual_body;size_t extent,actual_extent;
 sl_sepol_memory_owner memory;
 sl_sepol_diagnostic_owner diagnostics;
 policydb_t policy;policy_file_t file;struct sepol_handle handle;
 int memory_begin,diagnostic_begin,init_result,init_errno,read_result,read_errno,memory_finish,diagnostic_finish,result;
 char *read_cursor;size_t read_remaining;
 unsigned attempted,init_entered,init_returned,file_initialized,read_entered,read_returned,returned;
} sl_sepol_parse_owner;
/* Requires actual immutable policy-body acquisition and the complete fixed
 * redirected native source build. Does not authenticate a loaded policy or
 * dispose any original graph. Internal destructive error-edge custody remains
 * a separate unfinished source obligation; no native admission follows. */
int sl_sepol_parse_owned(sl_sepol_parse_owner *owner);
#endif