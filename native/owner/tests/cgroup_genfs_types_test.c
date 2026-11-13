/* UNRUN actual parsed-union interpreter. Fixtures are substituted policy
 * objects, never authenticated live policy/native label or admission proof. */
#include "../selinux_policy_relations.h"
#include "../../../third_party/libsepol/src/flask.h"
#include <sepol/policydb/services.h>
#include <sepol/policydb/polcaps.h>
#include <assert.h>
int main(void){
 static sl_selinux_genfs_types roots[8];static policydb_t policies[8];
 static unsigned filesystem_types[8][4];static type_datum_t *filesystem_refs[8][4];static sl_selinux_helper_visit helpers[8][48];static unsigned types[8][4];static type_datum_t *refs[8][4],*type_map[8][4];static type_datum_t datums[8][4];
 static class_datum_t *class_map[8][2];static hashtab_val_t tables[8];static hashtab_ptr_t buckets[8][1];static hashtab_node_t nodes[8][2];static class_datum_t classes[8][2];
 static ocontext_t initial[8][2],rules[8],fsuse[8];static genfs_t genfs[8];
 static context_struct_t observed[8],*observed_roster[8][1];static ebitmap_node_t capnodes[8];
 for(unsigned mode=0;mode<8;mode++){
  sl_selinux_genfs_types *r=&roots[mode];policydb_t *p=&policies[mode];
  p->policy_type=POLICY_KERN;p->target_platform=SEPOL_TARGET_SELINUX;p->p_types.nprim=4;p->type_val_to_struct=type_map[mode];
  for(unsigned i=0;i<4;i++){datums[mode][i].s.value=i+1;datums[mode][i].flavor=TYPE_TYPE;type_map[mode][i]=&datums[mode][i];}
  tables[mode].size=1;tables[mode].nel=2;tables[mode].htable=buckets[mode];buckets[mode][0]=&nodes[mode][0];
  nodes[mode][0].key="file";nodes[mode][0].datum=&classes[mode][0];nodes[mode][0].next=&nodes[mode][1];
  nodes[mode][1].key="dir";nodes[mode][1].datum=&classes[mode][1];classes[mode][0].s.value=1;classes[mode][1].s.value=2;
  p->p_classes.table=&tables[mode];p->p_classes.nprim=2;p->class_val_to_struct=class_map[mode];class_map[mode][0]=&classes[mode][0];class_map[mode][1]=&classes[mode][1];
  initial[mode][0].sid[0]=SECINITSID_UNLABELED;initial[mode][0].context[0].type=1;initial[mode][0].next=&initial[mode][1];
  initial[mode][1].sid[0]=SECINITSID_FS;initial[mode][1].context[0].type=2;p->ocontexts[OCON_ISID]=initial[mode];
  fsuse[mode].u.name="cgroup2";fsuse[mode].v.behavior=SECURITY_FS_USE_GENFS;fsuse[mode].context[0].type=2;p->ocontexts[OCON_FSUSE]=&fsuse[mode];
  genfs[mode].fstype="cgroup2";genfs[mode].head=&rules[mode];p->genfs=&genfs[mode];rules[mode].u.name="/";rules[mode].context[0].type=3;
  observed[mode].type=4;observed_roster[mode][0]=&observed[mode];r->observed=observed_roster[mode];r->observed_count=1;
  r->filesystem.types=filesystem_types[mode];r->filesystem.referents=filesystem_refs[mode];r->filesystem.capacity=4;
  r->targets.types=types[mode];r->targets.referents=refs[mode];r->targets.capacity=4;r->targets.custody.policy_original=p;
  r->targets.custody.helpers=helpers[mode];r->targets.custody.helper_capacity=48;r->targets.custody.string_capacity=16;
  if(mode==1){p->policycaps.node=&capnodes[mode];p->policycaps.highbit=MAPSIZE;capnodes[mode].map=MAPBIT<<POLICYDB_CAP_CGROUPSECLABEL;}
  if(mode==2)fsuse[mode].v.behavior=SECURITY_FS_USE_XATTR;
  if(mode==3)rules[mode].context[0].type=5;
  if(mode==4)rules[mode].next=&rules[mode];
  if(mode==5)r->targets.custody.helper_capacity=1;
  if(mode==6)observed[mode].type=0;
  if(mode==7)initial[mode][1].sid[0]=0;
  int result=sl_selinux_interpret_cgroup_genfs_types(r);assert(r->returned&&r->targets.returned&&r->targets.custody.returned);
  assert((result==0)==(mode==0));
  if(!mode){assert(r->targets.count==4&&r->filesystem.count==3&&r->filesystem.returned&&!r->filesystem.result&&r->checked_observed==1&&r->matched_rules==1);assert(!filesystem_types[mode][3]&&!filesystem_refs[mode][3]);for(unsigned i=0;i<4;i++)assert(types[mode][i]==1&&refs[mode][i]==&datums[mode][i]);}
  unsigned long count=r->targets.custody.helper_count;assert(sl_selinux_interpret_cgroup_genfs_types(r)==-1&&r->result==result&&r->targets.custody.helper_count==count);
 }
 return 0;
}