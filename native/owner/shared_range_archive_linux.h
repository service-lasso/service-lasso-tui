#ifndef SERVICE_LASSO_SHARED_RANGE_ARCHIVE_LINUX_H
#define SERVICE_LASSO_SHARED_RANGE_ARCHIVE_LINUX_H
#include <stdint.h>
#include <stddef.h>
/* Readonly archive representation, not native acquisition/authentication. */
typedef struct sl_linux_shared_range_archive {
 const unsigned char *native_original,*range_original;
 uint64_t base,mapping_bytes,offset,bytes,address;
 long result;unsigned attempted,returned;
} sl_linux_shared_range_archive;
long sl_linux_interpret_shared_range_archive(sl_linux_shared_range_archive *output,
 const unsigned char *native_body,size_t native_extent,
 const unsigned char *range_body,size_t range_extent);
struct sl_linux_final_record;
typedef struct sl_linux_shared_range_archive_roster {
 sl_linux_shared_range_archive roles[9];
 const unsigned char *bytes_original;
 const struct sl_linux_final_record *records_original;
 uint64_t expected,seen;long result;unsigned attempted,returned;
} sl_linux_shared_range_archive_roster;
long sl_linux_interpret_shared_range_records(sl_linux_shared_range_archive_roster *output,
 const unsigned char *bytes,size_t extent,const struct sl_linux_final_record *records,size_t count);
#endif
