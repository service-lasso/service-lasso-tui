#ifndef SERVICE_LASSO_EXEC_LABEL_LINUX_H
#define SERVICE_LASSO_EXEC_LABEL_LINUX_H
#include "immutable_payload_linux.h"
/* Exactly one source-owned installed public or immutable exec owner. */
typedef struct sl_linux_exec_label {
 sl_linux_local_installation *installation_original,*actual_installation;
 sl_linux_immutable_payload *payload_original,*actual_payload;
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity,extent,examined;
 sl_linux_call before,label,after;struct statx before_state,after_state;
 char attribute[17];int fd;long before_result,raw_count,after_result,result;
 unsigned attempted,returned,framing_complete;
} sl_linux_exec_label;
long sl_linux_observe_exec_label(sl_linux_exec_label *label);
#endif