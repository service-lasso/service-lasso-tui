#ifndef SERVICE_LASSO_OWNED_SEPOL
#error The context entry requires the fixed repository-owned libsepol build
#endif
#include "selinux_context_linux.h"
#include "../../third_party/libsepol/src/context.h"
#include <errno.h>
#include <stdint.h>
int sl_linux_interpret_selinux_context(sl_linux_selinux_context *o){
 int memory_active=0,diagnostic_active=0;
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;
 o->actual_parse=o->parse_original;o->actual_body=o->body;o->actual_extent=o->extent;
 sl_linux_policy_parse *p=o->actual_parse;
 if(!p||!p->attempted||!p->returned||p->result||p->status_result||
    !p->parser.returned||p->parser.result||!p->parser.init_returned||p->parser.init_result||
    !p->parser.read_returned||p->parser.read_result||p->parser.read_remaining||
    !p->actual_snapshot||!sl_linux_equal_selinux_status(&p->actual_snapshot->after_policy,&p->after_parse)||
    !o->actual_body||!o->actual_extent||o->actual_extent==SIZE_MAX||o->context_original||o->returned||
    o->after_context.attempted||o->after_context.status_original)goto done;
 /* No projected/terminated replacement string is substituted for input.
  * libsepol makes its owned working copy from these SAME exact bytes. */
 for(size_t i=0;i<o->actual_extent;i++){o->examined=i+1;if(!o->actual_body[i])goto done;}
 if(o->actual_extent==8&&o->actual_body[0]=='<'&&o->actual_body[1]=='<'&&
    o->actual_body[2]=='n'&&o->actual_body[3]=='o'&&o->actual_body[4]=='n'&&
    o->actual_body[5]=='e'&&o->actual_body[6]=='>'&&o->actual_body[7]=='>')goto done;
 o->after_context.status_original=&p->actual_snapshot->status;
 o->memory_begin=sl_sepol_memory_begin(&o->memory);if(o->memory_begin)goto done;memory_active=1;
 o->diagnostic_begin=sl_sepol_diagnostics_begin(&o->diagnostics);if(o->diagnostic_begin)goto finish;diagnostic_active=1;
 o->call_entered=1;
 o->call_result=context_from_string(&o->handle,&p->parser.policy,&o->context_original,(const char*)o->actual_body,o->actual_extent);
 o->call_errno=errno;o->call_returned=1;
 /* Sample after every actual returned library call, including a failed one.
  * This distinct failure never erases the call's raw result/error/root. */
 o->status_result=sl_linux_sample_selinux_status(&o->after_context);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&p->after_parse,&o->after_context))o->status_result=-1;
 if(!o->call_result&&!o->status_result&&o->context_original&&!o->memory.first_failure&&!o->memory.failed&&
    !o->diagnostics.first_failure&&!o->diagnostics.exhausted){
  o->user=o->context_original->user;o->role=o->context_original->role;o->type=o->context_original->type;
  if(o->user&&o->role&&o->type)o->result=0;
 }
 finish:
 if(diagnostic_active){o->diagnostic_finish=sl_sepol_diagnostics_finish(&o->diagnostics);if(o->diagnostic_finish)o->result=-1;}
 if(memory_active){o->memory_finish=sl_sepol_memory_finish(&o->memory);if(o->memory_finish)o->result=-1;}
 done:o->returned=1;return o->result;
 /* No context_destroy/free or policy mutation here. Internal helper traversal,
  * destructive partial roots and referents require complete owned source
  * closure. A valid context is not proof of kernel SID or controller identity. */
}
int sl_linux_interpret_selinux_self(sl_linux_selinux_context *o,sl_linux_lsm_self *self){
 if(!o||o->self_attempted||o->attempted)return -1;
 o->self_attempted=1;o->self_result=-1;o->self_original=self;o->actual_self=self;
 if(!self||!self->attempted||!self->returned||self->result||!self->framing_complete||
    self->raw_query!=1||!self->actual_bytes||self->actual_capacity<sizeof(sl_linux_lsm_header)+2||!self->context||!self->extent||self->extent>=self->actual_capacity-sizeof(sl_linux_lsm_header)||
    self->observed.ctx_len!=self->extent+1||self->context!=self->actual_bytes+sizeof(sl_linux_lsm_header)||
    self->context[self->extent]!=0||o->body||o->extent)goto done;
 /* The complete kernel record, including NUL and padding, stays in self.
  * context_from_string appends its own working-copy terminator. Pass only the
  * already validated original non-NUL span, without replacement allocation. */
 o->body=self->context;o->extent=self->extent;
 o->self_result=sl_linux_interpret_selinux_context(o);
 done:o->self_returned=1;return o->self_result;
}
int sl_linux_interpret_control_context(sl_linux_selinux_context *o){
 if(!o||o->control_attempted||o->attempted)return -1;
 o->control_attempted=1;o->control_result=-1;o->actual_control=o->control_original;
 sl_linux_cgroup_control_label *c=o->actual_control;
 if(!c||!c->attempted||!c->returned||c->result||!c->framing_complete||!c->actual_bytes||
    !c->extent||c->extent>=c->actual_capacity||c->raw_extent!=c->extent+1||
    c->raw_label_count<0||(unsigned long)c->raw_label_count!=c->raw_extent||c->actual_bytes[c->extent]||o->body||o->extent)goto done;
 o->body=c->actual_bytes;o->extent=c->extent;
 o->control_result=sl_linux_interpret_selinux_context(o);
 done:o->control_returned=1;return o->control_result;
}
int sl_linux_interpret_exec_context(sl_linux_selinux_context *o){
 if(!o||o->exec_attempted||o->attempted)return -1;o->exec_attempted=1;o->exec_result=-1;o->actual_exec=o->exec_original;
 sl_linux_exec_label *label=o->actual_exec;
 if(!label||!label->attempted||!label->returned||label->result||!label->framing_complete||!label->actual_bytes||!label->extent||label->extent>=label->actual_capacity||label->raw_count<0||(unsigned long)label->raw_count!=label->extent+1||label->actual_bytes[label->extent]||o->body||o->extent)goto done;
 o->body=label->actual_bytes;o->extent=label->extent;o->exec_result=sl_linux_interpret_selinux_context(o);
 done:o->exec_returned=1;return o->exec_result;
}
int sl_linux_interpret_archive_context(sl_linux_selinux_context *o){
 if(!o||o->archive_attempted||o->attempted)return -1;
 o->archive_attempted=1;o->archive_result=-1;o->actual_archive=o->archive_original;
 sl_linux_archive_label *label=o->actual_archive;
 if(!label||!label->attempted||!label->returned||label->result||!label->framing_complete||!label->actual_bytes||!label->extent||label->extent>=label->actual_capacity||label->raw_count<0||(unsigned long)label->raw_count!=label->extent+1||label->actual_bytes[label->extent]||o->body||o->extent)goto done;
 o->body=label->actual_bytes;o->extent=label->extent;
 o->archive_result=sl_linux_interpret_selinux_context(o);
 done:o->archive_returned=1;return o->archive_result;
}
