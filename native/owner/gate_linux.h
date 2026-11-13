#ifndef SERVICE_LASSO_GATE_LINUX_H
#define SERVICE_LASSO_GATE_LINUX_H
#include "public_controller_linux.h"
#include "signal_linux.h"
typedef struct sl_linux_gate_delivery {
 sl_linux_public_controller *controller_original;
 sl_linux_birth *birth_original;sl_linux_bootstrap *bootstrap_original;
 const unsigned char *bytes_original;
 sl_linux_write_signal signal;
 sl_linux_call writes[8],close;
 unsigned count,offset,attempted,returned,close_attempted;
 int fd;long signal_result,write_result,close_result,restore_result,result;
 sl_linux_call *write_failure_original;
} sl_linux_gate_delivery;
/* Native delivery operation ONLY. Necessary source associations below do not
 * replace the separate complete selected175 admission consumer. The owning
 * public caller must establish that cut before invoking this operation. */
long sl_linux_deliver_gate_token(sl_linux_gate_delivery *,sl_linux_public_controller *);
#endif
