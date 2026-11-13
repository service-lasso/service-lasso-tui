/* UNRUN source-bound bridge cases; no loaded-policy or native authority. */
#include "../control_access_linux.h"
#include <assert.h>
static int inventory_failure,status_failure,inventory_calls,status_calls,relabel_calls;
static sl_linux_selinux_context *expected;
int sl_selinux_interpret_access_sources(sl_selinux_access_sources *o){
 inventory_calls++;assert(o->custody.policy_original==&expected->actual_parse->parser.policy&&o->custody.target_type==expected->type);
 o->attempted=1;o->returned=1;o->result=inventory_failure;return o->result;
}
int sl_selinux_interpret_permission_sources(sl_selinux_access_sources *o,const char *class_name,const char *permission_name){
 assert(class_name[0]=='f'&&class_name[4]==0&&permission_name[0]=='r'&&permission_name[11]==0);relabel_calls++;return sl_selinux_interpret_access_sources(o);
}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *o){
 status_calls++;assert(o->status_original==&expected->actual_parse->actual_snapshot->status);
 o->attempted=1;o->returned=1;o->actual_status=o->status_original;o->result=status_failure;return o->result;
}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../control_access_linux.c"
int main(void){
 sl_linux_control_access roots[5]={0};sl_linux_selinux_context contexts[5]={0};sl_linux_policy_parse parses[5]={0};sl_linux_selinux_snapshot snapshots[5]={0};sl_linux_cgroup_control_label controls[5]={0};
 for(unsigned i=0;i<5;i++){
  expected=&contexts[i];expected->control_attempted=expected->control_returned=expected->attempted=expected->returned=1;expected->type=2;expected->actual_control=&controls[i];expected->actual_parse=&parses[i];parses[i].returned=1;parses[i].actual_snapshot=&snapshots[i];roots[i].context_original=expected;roots[i].kind_original=i?SL_CONTROL_RELABEL_FROM:SL_CONTROL_WRITE_APPEND;
  inventory_failure=(i==1||i==4)?-7:0;status_failure=(i==2||i==4)?-9:0;
  int result=sl_linux_interpret_control_access(&roots[i]);assert(result==((i==1||i==2||i==4)?-1:0)&&roots[i].returned&&roots[i].actual_context==expected);
  assert(roots[i].inventory_result==inventory_failure&&roots[i].status_result==status_failure&&roots[i].after_access.returned);
  int prior_inventory=inventory_calls,prior_status=status_calls;assert(sl_linux_interpret_control_access(&roots[i])<0&&inventory_calls==prior_inventory&&status_calls==prior_status);
 }
 assert(inventory_calls==5&&status_calls==5&&relabel_calls==4&&roots[1].inventory.result==-7&&roots[2].after_access.result==-9);
 assert(roots[3].actual_kind==SL_CONTROL_RELABEL_FROM&&!roots[3].result&&roots[4].inventory_result==-7&&roots[4].status_result==-9); return 0;
}