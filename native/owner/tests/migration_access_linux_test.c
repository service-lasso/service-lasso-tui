/* UNRUN actual purpose consumer with controlled table interpretation/status. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
static unsigned mode,calls,samples;
int sl_selinux_interpret_cgroup_genfs_types(sl_selinux_genfs_types *r){(void)r;assert(0);return -1;}
int sl_selinux_interpret_target_permission_sources(sl_selinux_access_sources *o,sl_selinux_creation_types *c,const char *class_name,const char *a,const char *b){
 calls++;assert(c&&o->custody.policy_original==c->custody.actual_policy);
 assert(!strcmp(class_name,mode==7?"filesystem":"file"));
 assert(!strcmp(a,mode==1||mode==7?"relabelfrom":"write")&&!strcmp(b,mode==7?"associate":mode==1?"relabelto":"append"));
 o->creation_original=o->actual_creation=c;o->returned=1;return o->result=(mode==2||mode==3)?-81:0;
}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;return s->result=(mode==3||mode==4)?-82:0;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return mode!=5;}
int main(void){
 static sl_linux_public_startup startups[8];static sl_linux_migration_labels labels[8];static sl_linux_policy_parse parses[8];static sl_linux_migration_access accesses[8][3];
 for(mode=0;mode<8;mode++){
  sl_linux_public_startup *s=&startups[mode];sl_linux_migration_labels *l=&labels[mode];sl_linux_migration_access *o=&accesses[mode][mode==7?2:mode==1?1:0];
  s->migration_labels=l;s->migration_label_capacity=1;s->migration_access=accesses[mode];s->migration_access_capacity=3;
  l->returned=l->labels.returned=1;l->parse_original=&parses[mode];l->labels.targets.custody.actual_policy=l->labels.filesystem.custody.actual_policy=&parses[mode].parser.policy;
  o->startup_original=s;o->labels_original=l;o->kind=mode==7?2:mode==1?1:0;
  if(mode==6)o->inventory.custody.target_type=9;
  unsigned before=calls,sp=samples;long result=sl_linux_interpret_migration_access(o);
  assert(o->returned&&samples==sp+1&&calls==before+(mode!=6));
  assert(result==(mode<=1||mode==7?0:mode==2||mode==3?-81:mode==4?-82:-EINVAL));
  assert(o->status_result==(mode==3||mode==4?-82:mode==5?-EINVAL:0));
  assert(sl_linux_interpret_migration_access(o)==-EINVAL&&o->result==result&&samples==sp+1);
 }
 return 0;
}