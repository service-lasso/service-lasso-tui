#ifndef SERVICE_LASSO_FAILED_PARENT_IDENTITY_LINUX_H
#define SERVICE_LASSO_FAILED_PARENT_IDENTITY_LINUX_H
#include "failed_archive_name_linux.h"
#include "deadline_linux.h"
#include "outer_birth_linux.h"
typedef struct sl_linux_failed_parent_identity {
 const unsigned char *identity_original;
 const sl_linux_outer_birth *birth_original;
 sl_linux_deadline_sequence *sequence_original;
 sl_linux_call process,clock;struct timespec instant;
 sl_linux_deadline_boundary process_boundary,clock_boundary;
 sl_linux_failed_archive_name name;
 long process_raw,clock_raw,process_boundary_result,clock_boundary_result,name_result,result;
 unsigned attempted,returned;
} sl_linux_failed_parent_identity;
long sl_linux_acquire_failed_parent_identity(sl_linux_failed_parent_identity *output,
 const unsigned char *identity,size_t identity_extent,const sl_linux_outer_birth *birth,
 sl_linux_deadline_sequence *sequence);
#endif
