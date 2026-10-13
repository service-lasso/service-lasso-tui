#if defined(__linux__) && defined(__x86_64__)
#include "policy_parse_linux.h"
int sl_linux_parse_policy_snapshot(sl_linux_policy_parse *o){
 sl_linux_selinux_snapshot *s;
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;o->actual_snapshot=o->snapshot_original;s=o->actual_snapshot;
 if(!s||!s->attempted||!s->returned||s->result||!s->eof||!s->actual_body||!s->length||s->length>s->actual_body_capacity||
    o->parser.attempted||o->parser.body||o->parser.extent||o->parser.returned||o->after_parse.attempted||o->after_parse.status_original||
    !sl_linux_equal_selinux_status(&s->before_policy,&s->after_policy)||s->before_policy.enforcing!=1)goto done;
 /* The actual arena binds distinct monotonic memory/attempt/diagnostic backing
  * before snapshot acquisition. Input is SAME acquired body, never a re-read. */
 o->parser.body=s->actual_body;o->parser.extent=s->length;
 o->after_parse.status_original=&s->status;
 o->result=sl_sepol_parse_owned(&o->parser);
 o->status_result=sl_linux_sample_selinux_status(&o->after_parse);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&s->after_policy,&o->after_parse))o->status_result=-1;
 if(!o->result&&o->status_result)o->result=o->status_result;
done:o->returned=1;return o->result;
 /* No policydb_destroy or graph disposition. Native policy/epoch/enforcement,
  * full parser error-edge custody and actor/exec interpretation remain required. */
}
#endif