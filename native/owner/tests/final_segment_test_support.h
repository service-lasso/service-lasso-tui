#ifndef SERVICE_LASSO_FINAL_SEGMENT_TEST_SUPPORT_H
#define SERVICE_LASSO_FINAL_SEGMENT_TEST_SUPPORT_H
/* UNRUN fixture support only. Each test owns its own main and distinct cases. */
#include "../public_startup_linux.h"
#include "../gate_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){(void)c;(void)s;(void)n;(void)a;(void)b;(void)d;(void)e;(void)f;(void)g;assert(0);return -EINVAL;}
#include "../selinux_policy_relations.c"
#include "../final_capture_linux.c"
typedef struct gate_case {sl_linux_public_controller c;sl_linux_gate_delivery gate;sl_linux_bootstrap bootstrap;sl_linux_final_record records[24];unsigned char bytes[4096];sl_linux_call writes[2],reads[2];} gate_case;
typedef struct bootstrap_case {sl_linux_public_controller c;sl_linux_bootstrap bootstrap;sl_linux_final_record records[100];unsigned char bytes[16384];sl_linux_call writes[2],reads[2];} bootstrap_case;
#endif
