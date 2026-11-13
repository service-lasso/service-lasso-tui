#ifndef SERVICE_LASSO_FAILED_ARCHIVE_LINUX_H
#define SERVICE_LASSO_FAILED_ARCHIVE_LINUX_H
#include <stdint.h>
#include <stddef.h>
enum { SL_FAILED_ARCHIVE_HEADER_BYTES=64 };
/* Independent readonly framing observation. This is neither FINAL nor ACK. */
typedef struct sl_linux_failed_archive_header {
 const unsigned char *bytes_original;
 size_t extent_original;
 uint64_t total_extent,record_count,records_extent,creator_birth_index;
 long result;unsigned attempted,returned;
} sl_linux_failed_archive_header;
long sl_linux_interpret_failed_archive_header(sl_linux_failed_archive_header *output,
 const unsigned char *bytes,size_t extent);
#endif
