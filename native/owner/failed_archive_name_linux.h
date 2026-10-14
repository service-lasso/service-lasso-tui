#ifndef SERVICE_LASSO_FAILED_ARCHIVE_NAME_LINUX_H
#define SERVICE_LASSO_FAILED_ARCHIVE_NAME_LINUX_H
#include "owner_linux.h"
#include <stddef.h>
#include <time.h>
#include <limits.h>
_Static_assert(sizeof(time_t)==8&&sizeof(long)==8&&INT_MAX==2147483647,"failed archive decimal source ABI");
enum {SL_FAILED_ARCHIVE_NAME_BYTES=(sizeof("service-lasso-tui.")-1)+64+1+10+1+19+1+9+sizeof(".failed")};
typedef struct sl_linux_failed_archive_name {
 const unsigned char *identity_original;
 const sl_linux_call *process_original,*clock_original;
 const struct timespec *instant_original;
 unsigned char name[SL_FAILED_ARCHIVE_NAME_BYTES];
 size_t extent;long result;unsigned attempted,returned;
} sl_linux_failed_archive_name;
long sl_linux_construct_failed_archive_name(sl_linux_failed_archive_name *output,
 const unsigned char *source_identity,size_t identity_extent,
 const sl_linux_call *process,const sl_linux_call *clock,const struct timespec *instant);
int sl_linux_failed_archive_name_matches_original(const sl_linux_failed_archive_name *original);
#endif
