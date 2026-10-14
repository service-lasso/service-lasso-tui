#ifndef SERVICE_LASSO_CREATOR_FILE_LINUX_H
#define SERVICE_LASSO_CREATOR_FILE_LINUX_H
#include "local_installation_linux.h"
#include "deadline_linux.h"
typedef struct sl_linux_creator_read {
 sl_linux_call call;unsigned long offset,requested;long raw;unsigned attempted,returned;
 sl_linux_deadline_boundary boundary;
} sl_linux_creator_read;
typedef struct sl_linux_creator_file {
 const sl_linux_local_installation *installation_original;
 unsigned char *body;unsigned long body_capacity;
 sl_linux_creator_read *reads;unsigned long read_capacity;
 sl_linux_deadline_sequence *sequence_original;
 const sl_linux_local_installation *actual_installation;
 unsigned char *actual_body;unsigned long actual_body_capacity;
 sl_linux_creator_read *actual_reads;unsigned long actual_read_capacity,count,extent;
 int actual_fd;
 sl_linux_deadline_sequence *actual_sequence;
 sl_linux_deadline_boundary before_boundary,eof_boundary,after_boundary;
 sl_linux_call before_call,eof_call,after_call;
 struct statx before,after;unsigned char eof_byte;
 long before_raw,eof_raw,after_raw,result;
 unsigned attempted,returned,complete;
} sl_linux_creator_file;
_Static_assert(sizeof(sl_linux_creator_read)==176,"retained creator pread row and two-clock boundary");
_Static_assert(offsetof(sl_linux_creator_read,boundary)==112,"retained creator boundary offset" );
_Static_assert(sizeof(sl_linux_creator_file)==1120&&offsetof(sl_linux_creator_file,before_boundary)==120,"creator owner and boundary geometry" );
_Static_assert(sizeof(struct statx)==256,"selected statx output ABI");
long sl_linux_capture_creator_file(sl_linux_creator_file *original);
int sl_linux_creator_file_matches_original(const sl_linux_creator_file *original);
int sl_linux_creator_file_excludes_output(const sl_linux_creator_file *original,const void *output,unsigned long extent);
/* Borrowed held FD, no close/seek/loaded-image authority. All containing
 * source/D/provider/own-writer/referent and actual entry joins remain required. */
#endif
