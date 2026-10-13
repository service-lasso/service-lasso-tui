/* UNRUN controlled parsed-object inventory, not loaded policy or actor proof. */
#include "../selinux_policy_relations.h"
#include <assert.h>
#include <string.h>
#include "../selinux_policy_relations.c"
int main(void){
 policydb_t policy={0};type_datum_t types[4]={{0}};type_datum_t *refs[4];ebitmap_t attrs[4]={{0}};ebitmap_node_t bits[4]={{0}};
 class_datum_t file={0};class_datum_t *classes[1]={&file};perm_datum_t write={0},append={0};
 hashtab_node_t class_node={.key="file",.datum=&file},write_node={.key="write",.datum=&write},append_node={.key="append",.datum=&append};
 hashtab_ptr_t class_buckets[1]={&class_node},permission_buckets[1]={&write_node};
 hashtab_val_t class_table={.htable=class_buckets,.size=1,.nel=1},permissions={.htable=permission_buckets,.size=1,.nel=2};write_node.next=&append_node;
 file.s.value=1;file.permissions.table=&permissions;write.s.value=2;append.s.value=3;
 policy.policy_type=POLICY_KERN;policy.target_platform=SEPOL_TARGET_SELINUX;policy.p_types.nprim=4;policy.type_val_to_struct=refs;policy.type_attr_map=attrs;
 policy.p_classes.nprim=1;policy.p_classes.table=&class_table;policy.class_val_to_struct=classes;
 for(unsigned i=0;i<4;i++){refs[i]=&types[i];types[i].s.value=i+1;attrs[i].node=&bits[i];attrs[i].highbit=MAPSIZE;bits[i].map=MAPBIT<<i;}
 types[2].flavor=types[3].flavor=TYPE_ATTRIB;bits[0].map|=MAPBIT<<2;bits[1].map|=MAPBIT<<3;
 struct avtab_node normal={0},conditional={0};normal.key=(avtab_key_t){.source_type=3,.target_type=4,.target_class=1,.specified=AVTAB_ALLOWED};normal.datum.data=2;conditional.key=normal.key;conditional.datum.data=4;
 avtab_ptr_t normal_buckets[1]={&normal},conditional_buckets[1]={&conditional};policy.te_avtab=(avtab_t){.htable=normal_buckets,.nslot=1,.nel=1};policy.te_cond_avtab=(avtab_t){.htable=conditional_buckets,.nslot=1,.nel=1};
 sl_selinux_source_access sources[5][4]={0};sl_selinux_rule_visit visits[5][2]={0};sl_selinux_helper_visit helpers[5][32]={0};sl_selinux_access_sources roots[5]={0};
 for(unsigned i=0;i<5;i++){roots[i].sources=sources[i];roots[i].source_capacity=4;roots[i].custody.policy_original=&policy;roots[i].custody.target_type=2;roots[i].custody.visits=visits[i];roots[i].custody.capacity=2;roots[i].custody.helpers=helpers[i];roots[i].custody.helper_capacity=i==1?7:32;roots[i].custody.string_capacity=32;}
 assert(sl_selinux_interpret_access_sources(&roots[0])==0&&roots[0].returned&&roots[0].source_count==4);
 assert(roots[0].write_original==&write&&roots[0].append_original==&append&&sources[0][0].type_original==&types[0]);
 assert(sources[0][0].unconditional==2&&sources[0][0].conditional==4&&sources[0][0].possible==6&&!sources[0][1].possible&&!sources[0][2].possible);
 assert(visits[0][1].conditional&&visits[0][1].matched&&!(conditional.key.specified&AVTAB_ENABLED));unsigned long prior=roots[0].custody.helper_count;
 assert(sl_selinux_interpret_access_sources(&roots[0])<0&&roots[0].custody.helper_count==prior&&sources[0][0].possible==6);
 assert(sl_selinux_interpret_access_sources(&roots[1])<0&&roots[1].custody.helper_count==7&&!roots[1].custody.count&&sources[0][0].possible==6);
 const char class_original[]="file",permission_original[]="write",missing_original[]="not-a-permission";
 assert(sl_selinux_interpret_permission_sources(&roots[2],class_original,permission_original)==0);
 assert(roots[2].custody.actual_class_name==class_original&&roots[2].permission_names[0]==permission_original&&roots[2].permission_originals[0]==&write&&roots[2].permission_bits[0]==2&&roots[2].permission_count==1);
 assert(!roots[2].write_original&&!roots[2].append_original&&sources[2][0].unconditional==2&&!sources[2][0].conditional&&sources[2][0].possible==2);
 assert(sl_selinux_interpret_permission_sources(&roots[3],class_original,missing_original)<0&&roots[3].returned&&roots[3].permission_names[0]==missing_original&&!roots[3].permission_originals[0]&&!roots[3].custody.count);
 assert(sources[0][0].possible==6&&roots[0].write_original==&write&&roots[2].permission_originals[0]==&write);
 policydb_t permissive_policy=policy;ebitmap_node_t permissive_node={.map=MAPBIT<<2};permissive_policy.permissive_map=(ebitmap_t){.node=&permissive_node,.highbit=MAPSIZE};roots[4].custody.policy_original=&permissive_policy;
 assert(sl_selinux_interpret_access_sources(&roots[4])==0&&sources[4][1].permissive&&sources[4][1].permissive_original==&permissive_policy.permissive_map&&sources[4][1].possible==6&&!sources[4][1].unconditional&&!sources[4][1].conditional);
 assert(!sources[4][0].permissive&&sources[0][1].possible==0&&sources[0][0].possible==6);
 /* Distinct directory policy/class roots: the archive pair consumer must
  * retain BOTH common permissions and disabled conditional append rules. */
 policydb_t directory_policy=policy;class_datum_t directory=file;class_datum_t *directory_classes[1]={&directory};
 hashtab_node_t directory_class_node={.key="dir",.datum=&directory};hashtab_ptr_t directory_buckets[1]={&directory_class_node};hashtab_val_t directory_table={.htable=directory_buckets,.size=1,.nel=1};
 directory_policy.p_classes.table=&directory_table;directory_policy.class_val_to_struct=directory_classes;
 sl_selinux_source_access pair_sources[2][4]={0};sl_selinux_rule_visit pair_visits[2][2]={0};sl_selinux_helper_visit pair_helpers[2][32]={0};sl_selinux_access_sources pairs[2]={0};
 for(unsigned i=0;i<2;i++){pairs[i].sources=pair_sources[i];pairs[i].source_capacity=4;pairs[i].custody.policy_original=&directory_policy;pairs[i].custody.target_type=2;pairs[i].custody.visits=pair_visits[i];pairs[i].custody.capacity=2;pairs[i].custody.helpers=pair_helpers[i];pairs[i].custody.helper_capacity=32;pairs[i].custody.string_capacity=32;}
 const char directory_name[]="dir",append_name[]="append";
 assert(sl_selinux_interpret_permission_pair_sources(&pairs[0],directory_name,permission_original,append_name)==0);
 assert(pairs[0].custody.actual_class_name==directory_name&&pairs[0].custody.class_original==&directory&&pairs[0].permission_names[0]==permission_original&&pairs[0].permission_names[1]==append_name&&pairs[0].permission_originals[0]==&write&&pairs[0].permission_originals[1]==&append&&pairs[0].permission_count==2);
 assert(pair_sources[0][0].unconditional==2&&pair_sources[0][0].conditional==4&&pair_sources[0][0].possible==6&&pair_visits[0][1].conditional&&pair_visits[0][1].matched);
 assert(sl_selinux_interpret_permission_pair_sources(&pairs[1],directory_name,permission_original,permission_original)<0&&pairs[1].permission_bits[0]==pairs[1].permission_bits[1]&&!pairs[1].custody.count);
 assert(sources[0][0].possible==6&&roots[0].custody.actual_policy==&policy&&roots[0].custody.class_original==&file);
 return 0;
}
