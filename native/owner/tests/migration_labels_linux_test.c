/* UNRUN actual startup consumer with controlled union/status outcomes.
 * Does not authenticate policy, labels, actor scope or native admission. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,unions,samples;
int sl_selinux_interpret_target_permission_sources(sl_selinux_access_sources *o,sl_selinux_creation_types *c,const char *class_name,const char *a,const char *b){(void)class_name;(void)o;(void)c;(void)a;(void)b;assert(0);return -1;}
int sl_selinux_interpret_cgroup_genfs_types(sl_selinux_genfs_types *r){unions++;assert(r->observed_count==1&&r->observed[0]);r->attempted=r->returned=1;return r->result=(mode==1||mode==2)?-71:0;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;return s->result=(mode==2||mode==3)?-72:0;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return mode!=4;}
int main(void){
 static sl_linux_public_startup startups[7];static sl_linux_migration_labels owners[7];
 static sl_linux_policy_parse parses[7];static sl_linux_selinux_snapshot snapshots[7];
 static sl_linux_cgroup_control_label controls[7];static sl_linux_selinux_context contexts[7];
 static context_struct_t originals[7],*observed[7][1];
 for(mode=0;mode<7;mode++){
  sl_linux_public_startup *s=&startups[mode];sl_linux_migration_labels *o=&owners[mode];
  s->migration_labels=o;s->migration_label_capacity=s->actual_policy_capacity=s->actual_parse_capacity=s->mounts.selinux_count=1;
  s->actual_parses=&parses[mode];s->actual_controls=&controls[mode];s->actual_control_capacity=s->actual_control_context_capacity=1;s->actual_control_contexts=&contexts[mode];
  parses[mode].returned=1;parses[mode].actual_snapshot=&snapshots[mode];controls[mode].attempted=controls[mode].returned=controls[mode].framing_complete=1;
  contexts[mode].control_returned=contexts[mode].returned=1;contexts[mode].actual_parse=&parses[mode];contexts[mode].actual_control=&controls[mode];contexts[mode].context_original=&originals[mode];contexts[mode].type=originals[mode].type=1;
  o->startup_original=s;o->parse_original=&parses[mode];o->observed_capacity=1;o->labels.observed=observed[mode];
  if(mode==5)o->observed_capacity=0;if(mode==6)contexts[mode].actual_control=0;
  unsigned before=unions,sp=samples;long result=sl_linux_interpret_migration_labels(o);
  assert(o->returned&&samples==sp+1);
  assert(result==(mode==0?0:mode==1||mode==2?-71:mode==3?-72:-EINVAL));
  assert(o->status_result==(mode==2||mode==3?-72:mode==4?-EINVAL:0));
  assert(unions==before+(mode<4));
  if(mode>=4)assert(!observed[mode][0]&&!o->labels.observed_count);
  assert(sl_linux_interpret_migration_labels(o)==-EINVAL&&samples==sp+1&&o->result==result);
 }
 return 0;
}