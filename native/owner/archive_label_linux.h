#ifndef SERVICE_LASSO_ARCHIVE_LABEL_LINUX_H
#define SERVICE_LASSO_ARCHIVE_LABEL_LINUX_H
#include "owner_linux.h"
#include <linux/stat.h>
struct sl_linux_public_startup;
enum sl_linux_archive_label_kind {SL_ARCHIVE_LOCAL_LABEL=0,SL_ARCHIVE_FINAL_LABEL=1,SL_ARCHIVE_DIRECTORY_LABEL=2};
typedef struct sl_linux_archive_label {
 struct sl_linux_public_startup *startup_original,*actual_startup;
 enum sl_linux_archive_label_kind kind,actual_kind;
 void *object_original,*creation_directory_original;struct statx *identity_original;sl_linux_call *creation_original;
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity,extent,examined;
 sl_linux_call before,label,after;struct statx before_state,after_state;
 char attribute[17];int fd;long before_result,raw_count,after_result,result;
 unsigned attempted,returned,framing_complete;
} sl_linux_archive_label;
long sl_linux_observe_archive_label(sl_linux_archive_label *root);
long sl_linux_observe_archive_labels(struct sl_linux_public_startup *startup);
long sl_linux_interpret_archive_contexts(struct sl_linux_public_startup *startup);
#endif
