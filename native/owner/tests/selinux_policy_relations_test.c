/* UNRUN controlled policy-object relations, NOT parser/kernel/enforcement proof. */
#include "../selinux_policy_relations.h"
#include <assert.h>
#include <string.h>
static hashtab_val_t classes,permissions;
static class_datum_t file_class;static perm_datum_t write_permission;
static hashtab_node_t class_node,permission_node;static hashtab_ptr_t class_buckets[1],permission_buckets[1];
static sl_selinux_helper_visit helpers[32];
#include "../selinux_policy_relations.c"
static policydb_t p;static type_datum_t types[4];static type_datum_t *type_refs[4];
static class_datum_t *class_refs[1];static ebitmap_t attributes[4];static ebitmap_node_t bits[4];
static struct avtab_node normal,conditional;
static avtab_ptr_t normal_buckets[1],conditional_buckets[1];
static sl_selinux_rule_visit visits[3];static type_datum_t *chain[4];
static void setup(void){
 memset(&classes,0,sizeof classes);memset(&permissions,0,sizeof permissions);memset(&class_node,0,sizeof class_node);memset(&permission_node,0,sizeof permission_node);memset(helpers,0,sizeof helpers);
 class_node.key=(char*)"file";class_node.datum=&file_class;permission_node.key=(char*)"write";permission_node.datum=&write_permission;
 class_buckets[0]=&class_node;permission_buckets[0]=&permission_node;
 classes.htable=class_buckets;classes.size=classes.nel=1;permissions.htable=permission_buckets;permissions.size=permissions.nel=1;
 memset(&p,0,sizeof p);memset(types,0,sizeof types);memset(&file_class,0,sizeof file_class);
 memset(&write_permission,0,sizeof write_permission);memset(attributes,0,sizeof attributes);memset(bits,0,sizeof bits);
 memset(&normal,0,sizeof normal);memset(&conditional,0,sizeof conditional);memset(visits,0,sizeof visits);memset(chain,0,sizeof chain);
 p.policy_type=POLICY_KERN;p.target_platform=SEPOL_TARGET_SELINUX;p.p_types.nprim=4;p.type_val_to_struct=type_refs;
 for(unsigned i=0;i<4;i++){types[i].s.value=i+1;type_refs[i]=&types[i];attributes[i].node=&bits[i];bits[i].map=MAPBIT<<i;attributes[i].highbit=MAPSIZE;}
 types[2].flavor=types[3].flavor=TYPE_ATTRIB;types[1].bounds=1;
 bits[0].map|=MAPBIT<<2;bits[1].map|=MAPBIT<<3;p.type_attr_map=attributes;
 p.p_classes.nprim=1;p.p_classes.table=&classes;p.class_val_to_struct=class_refs;class_refs[0]=&file_class;
 file_class.s.value=1;file_class.permissions.table=&permissions;write_permission.s.value=2;
 normal.key=(avtab_key_t){.source_type=3,.target_type=4,.target_class=1,.specified=AVTAB_ALLOWED};normal.datum.data=2;
 conditional.key=normal.key;conditional.datum.data=2;
 normal_buckets[0]=&normal;conditional_buckets[0]=&conditional;
 p.te_avtab=(avtab_t){.htable=normal_buckets,.nslot=1,.nel=1};
 p.te_cond_avtab=(avtab_t){.htable=conditional_buckets,.nslot=1,.nel=1};
}
static sl_selinux_te_union query(void){
 return (sl_selinux_te_union){.policy_original=&p,.source_type=1,.target_type=2,.class_name="file",.permission_name="write",.visits=visits,.capacity=3,.helpers=helpers,.helper_capacity=32,.string_capacity=64};
}
static sl_selinux_bounds bounds(void){return (sl_selinux_bounds){.policy_original=&p,.old_type=1,.new_type=2,.visited=chain,.capacity=4};}
int main(void){
 setup();sl_selinux_bounds b=bounds();assert(sl_selinux_interpret_bounds(&b)==0&&b.count==1&&chain[0]==&types[1]&&b.actual_policy==&p);
 unsigned prior=b.count;assert(sl_selinux_interpret_bounds(&b)<0&&b.count==prior&&chain[0]==&types[1]);
 setup();b=bounds();types[1].bounds=2;assert(sl_selinux_interpret_bounds(&b)<0&&b.count==1&&chain[0]==&types[1]);
 setup();b=bounds();b.capacity=1;types[1].bounds=3;types[2].flavor=TYPE_TYPE;types[2].bounds=1;
 assert(sl_selinux_interpret_bounds(&b)<0&&b.count==1); /* no capacity for next actual member */
 setup();b=bounds();b.new_type=1;assert(sl_selinux_interpret_bounds(&b)<0&&!b.count); /* no reduction */
 setup();b=bounds();chain[0]=&types[0];assert(sl_selinux_interpret_bounds(&b)<0&&!b.count&&chain[0]==&types[0]);
 setup();sl_selinux_te_union q=query();assert(sl_selinux_interpret_te_union(&q)==0&&q.possible==2&&q.unconditional==2&&q.conditional==2);
 assert(q.count==2&&visits[0].node_original==&normal&&visits[1].node_original==&conditional&&visits[1].conditional&&visits[1].matched);
 assert(!(conditional.key.specified&AVTAB_ENABLED)); /* disabled branch STILL included */
 assert(q.class_original==&file_class&&q.permission_original==&write_permission&&q.source_attributes==&attributes[0]);
 prior=q.helper_count;assert(sl_selinux_interpret_te_union(&q)<0&&q.helper_count==prior&&q.possible==2);
 setup();q=query();normal.datum.data=0;conditional.key.specified|=AVTAB_ENABLED;
 assert(sl_selinux_interpret_te_union(&q)==0&&q.unconditional==0&&q.conditional==2&&q.possible==2);
 setup();q=query();normal.datum.data=conditional.datum.data=0;assert(sl_selinux_interpret_te_union(&q)==0&&!q.possible&&q.count==2);
 setup();q=query();q.capacity=1;assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1&&q.unconditional==2&&!q.possible);
 setup();q=query();normal.next=&normal;assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1&&visits[0].node_original==&normal);
 setup();q=query();p.te_avtab.nel=2;assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1);
 setup();q=query();q.permission_name="not-a-permission";assert(sl_selinux_interpret_te_union(&q)<0&&!q.count);
 setup();q=query();write_permission.s.value=33;assert(sl_selinux_interpret_te_union(&q)<0&&!q.count&&q.permission_original==&write_permission);
 setup();q=query();visits[0].node_original=&normal;assert(sl_selinux_interpret_te_union(&q)<0&&!q.helper_count&&visits[0].node_original==&normal);
 setup();q=query();normal.key.target_type=0;assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1&&visits[0].node_original==&normal);
 setup();q=query();normal.key.specified=0x4000;assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1&&visits[0].node_original==&normal);
 setup();q=query();q.helper_capacity=2;assert(sl_selinux_interpret_te_union(&q)<0&&q.helper_count==2&&!q.count&&helpers[1].hash_original==&class_node);
 setup();q=query();class_node.next=&class_node;assert(sl_selinux_interpret_te_union(&q)<0&&q.helper_count==2&&helpers[1].hash_original==&class_node);
 setup();q=query();bits[0].next=&bits[0];assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1&&!q.possible);
 setup();q=query();attributes[0].highbit=MAPSIZE+1;assert(sl_selinux_interpret_te_union(&q)<0&&q.count==1);
 setup();q=query();q.string_capacity=3;assert(sl_selinux_interpret_te_union(&q)<0&&helpers[1].compared==3&&!q.count);
 setup();q=query();avtab_ptr_t empty_buckets[8]={0};p.te_avtab=(avtab_t){.htable=empty_buckets,.nslot=8,.nel=0};q.helper_capacity=7;
 assert(sl_selinux_interpret_te_union(&q)<0&&!q.count&&q.helper_count==7);
 assert(helpers[4].kind==SL_SELINUX_AVTAB_BUCKET&&helpers[4].avtab_original==&p.te_avtab&&helpers[6].bucket==2);
 return 0;
}