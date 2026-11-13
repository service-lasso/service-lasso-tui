#include "context_reachability_linux.h"
int sl_linux_interpret_context_reachability(sl_linux_context_reachability *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;o->actual_context=o->context_original;
 sl_linux_selinux_context *context=o->actual_context;
 if(!context||!context->self_attempted||!context->self_returned||context->self_result||!context->actual_self||!context->attempted||!context->returned||context->result||context->status_result||!context->actual_parse||!context->type||o->graph.policy_original||o->graph.seed||o->after_graph.attempted||o->after_graph.status_original)goto done;
 sl_linux_policy_parse *parse=context->actual_parse;
 if(!parse->returned||parse->result||parse->status_result||!parse->actual_snapshot||!sl_linux_equal_selinux_status(&parse->after_parse,&context->after_context))goto done;
 o->graph.policy_original=&parse->parser.policy;o->graph.seed=context->type;o->after_graph.status_original=&parse->actual_snapshot->status;
 o->graph_result=sl_selinux_interpret_reachability(&o->graph);o->status_result=sl_linux_sample_selinux_status(&o->after_graph);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&context->after_context,&o->after_graph))o->status_result=-1;
 if(!o->graph_result&&!o->status_result)o->result=0;
 done:o->returned=1;return o->result;
}