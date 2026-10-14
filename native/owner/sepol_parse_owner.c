#ifndef SERVICE_LASSO_OWNED_SEPOL
#error The owned parser entry requires the fixed repository-owned libsepol build
#endif
#include "sepol_parse_owner.h"
#include <errno.h>
int sl_sepol_parse_owned(sl_sepol_parse_owner *o){
 int memory_active=0,diagnostic_active=0;
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;
 o->actual_body=o->body;o->actual_extent=o->extent;
 if(!o->actual_body||!o->actual_extent||o->returned||o->init_entered||o->read_entered||o->file_initialized||o->policy.sl_source_owner)goto done;
 /* Both embedded roots exist before any library call. No stack policy_file
  * wrapper or destructive public policydb_from_image projection is used. */
 o->memory_begin=sl_sepol_memory_begin(&o->memory);if(o->memory_begin)goto done;memory_active=1;
 o->diagnostic_begin=sl_sepol_diagnostics_begin(&o->diagnostics);if(o->diagnostic_begin)goto finish;diagnostic_active=1;
 o->init_entered=1;o->init_result=policydb_init(&o->policy);o->init_errno=errno;o->init_returned=1;
 /* Even a library return0 cannot hide an actual failed owned allocation or
  * diagnostic. Never read a partially destroyed initializer in that case. */
 if(o->init_result||o->memory.first_failure||o->memory.failed||o->diagnostics.first_failure||o->diagnostics.exhausted)goto finish;
 policy_file_init(&o->file);o->file_initialized=1;
 o->file.type=PF_USE_MEMORY;o->file.data=(char*)o->actual_body;o->file.len=o->actual_extent;o->file.size=o->actual_extent;o->file.handle=&o->handle;
 o->read_entered=1;o->read_result=policydb_read(&o->policy,&o->file,0);o->read_errno=errno;
 o->read_cursor=o->file.data;o->read_remaining=o->file.len;o->read_returned=1;
 if(!o->read_result&&!o->read_remaining&&!o->memory.first_failure&&!o->memory.failed&&!o->diagnostics.first_failure&&!o->diagnostics.exhausted)o->result=0;
 finish:
 if(diagnostic_active){o->diagnostic_finish=sl_sepol_diagnostics_finish(&o->diagnostics);if(o->diagnostic_finish)o->result=-1;}
 if(memory_active){o->memory_finish=sl_sepol_memory_finish(&o->memory);if(o->memory_finish)o->result=-1;}
 done:o->returned=1;return o->result;
 /* No policydb_destroy/free here: these SAME roots, all raw failures and
  * retained allocation/argument referents remain owned. Parser internal error
  * cleanup mutation still needs its own actual source-edge custody before
  * this source can become an admitted complete profile implementation. */
}