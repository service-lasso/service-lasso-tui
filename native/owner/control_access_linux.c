#include "control_access_linux.h"
int sl_linux_interpret_control_access(sl_linux_control_access *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;o->actual_context=o->context_original;o->actual_kind=o->kind_original;
 sl_linux_selinux_context *context=o->actual_context;
 if(!context||!context->control_attempted||!context->control_returned||context->control_result||!context->attempted||!context->returned||context->result||context->status_result||!context->actual_parse||!context->actual_control||!context->type||o->inventory.custody.policy_original||o->inventory.custody.target_type||o->after_access.attempted||o->after_access.status_original)goto done;
 sl_linux_policy_parse *parse=context->actual_parse;
 if(!parse->returned||parse->result||parse->status_result||!parse->actual_snapshot||!sl_linux_equal_selinux_status(&parse->after_parse,&context->after_context))goto done;
 o->inventory.custody.policy_original=&parse->parser.policy;o->inventory.custody.target_type=context->type;
 o->after_access.status_original=&parse->actual_snapshot->status;
 if(o->actual_kind==SL_CONTROL_WRITE_APPEND)o->inventory_result=sl_selinux_interpret_access_sources(&o->inventory);
 else if(o->actual_kind==SL_CONTROL_RELABEL_FROM)o->inventory_result=sl_selinux_interpret_permission_sources(&o->inventory,sl_selinux_name_file,sl_selinux_name_relabelfrom);
 else {o->inventory_result=-1;goto sampled;}
 sampled:
 o->status_result=sl_linux_sample_selinux_status(&o->after_access);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&context->after_context,&o->after_access))o->status_result=-1;
 if(!o->inventory_result&&!o->status_result)o->result=0;
 done:o->returned=1;return o->result;
}