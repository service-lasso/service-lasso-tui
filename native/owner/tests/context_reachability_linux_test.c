/* UNRUN source-bound bridge cases; no loaded-policy or native authority. */
#include "../context_reachability_linux.h"
#include <assert.h>
static int inventory_failure,status_failure,inventory_calls,status_calls;
static sl_linux_selinux_context *expected;
int sl_selinux_interpret_reachability(sl_selinux_reachability *o){
 inventory_calls++;assert(o->policy_original==&expected->actual_parse->parser.policy&&o->seed==expected->type);
 o->attempted=1;o->returned=1;o->result=inventory_failure;return o->result;
}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *o){
 status_calls++;assert(o->status_original==&expected->actual_parse->actual_snapshot->status);
 o->attempted=1;o->returned=1;o->actual_status=o->status_original;o->result=status_failure;return o->result;
}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../context_reachability_linux.c"
int main(void){
 sl_linux_context_reachability roots[3]={0};sl_linux_selinux_context contexts[3]={0};sl_linux_policy_parse parses[3]={0};sl_linux_selinux_snapshot snapshots[3]={0};sl_linux_lsm_self self[3]={0};
 for(unsigned i=0;i<3;i++){
  expected=&contexts[i];expected->self_attempted=expected->self_returned=expected->attempted=expected->returned=1;expected->type=2;expected->actual_self=&self[i];expected->actual_parse=&parses[i];parses[i].returned=1;parses[i].actual_snapshot=&snapshots[i];roots[i].context_original=expected;
  inventory_failure=i==1?-7:0;status_failure=i==2?-9:0;
  int result=sl_linux_interpret_context_reachability(&roots[i]);assert(result==(i?-1:0)&&roots[i].returned&&roots[i].actual_context==expected);
  assert(roots[i].graph_result==inventory_failure&&roots[i].status_result==status_failure&&roots[i].after_graph.returned);
  int prior_inventory=inventory_calls,prior_status=status_calls;assert(sl_linux_interpret_context_reachability(&roots[i])<0&&inventory_calls==prior_inventory&&status_calls==prior_status);
 }
 assert(inventory_calls==3&&status_calls==3&&roots[1].graph.result==-7&&roots[2].after_graph.result==-9);
 return 0;
}