#ifndef SERVICE_LASSO_SEPOL_SOURCE_REDIRECT_H
#define SERVICE_LASSO_SEPOL_SOURCE_REDIRECT_H
/* Fixed policy-library build include ONLY, never applied to the owner or libc.
 * Consume system declarations before call macros. Native build integration must
 * select this include for every actually linked library translation unit. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sepol_source_owner.h"
#ifndef HAVE_REALLOCARRAY
#define HAVE_REALLOCARRAY 1
#endif
#define malloc(n) sl_sepol_source_malloc((n),__FILE__,__LINE__)
#define calloc(n,s) sl_sepol_source_calloc((n),(s),__FILE__,__LINE__)
#define realloc(p,n) sl_sepol_source_realloc((p),(n),__FILE__,__LINE__)
#define reallocarray(p,n,s) sl_sepol_source_reallocarray((p),(n),(s),__FILE__,__LINE__)
#define free(p) sl_sepol_source_free((p),__FILE__,__LINE__)
#define strdup(p) sl_sepol_source_strdup((p),__FILE__,__LINE__)
#define strndup(p,n) sl_sepol_source_strndup((p),(n),__FILE__,__LINE__)
/* vasprintf/asprintf and diagnostics are NOT captured here. Their actual
 * reachable source owners must be connected before any real parse admission. */
#endif