/* UNRUN controlled parsed-object inventory, not loaded policy or actor proof. */
#include "../selinux_policy_relations.h"
#include <assert.h>
#include <string.h>
#include "../selinux_policy_relations.c"
int main(void){
 policydb_t policy={0};type_datum_t types[4]={{0}};type_datum_t *refs[4];ebitmap_t attrs[4]={{0}};ebitmap_node_t bits[4]={{0}};
 class_datum_t file={0};class_datum_t *classes[1]={&file};perm_datum_t write={0},append={0};
 hashtab_node_t class_node={.key="process",.datum=&file},write_node={.key="transition",.datum=&write},append_node={.key="dyntransition",.datum=&append};
 hashtab_ptr_t class_buckets[1]={&class_node},permission_buckets[1]={&write_node};
 hashtab_val_t class_table={.htable=class_buckets,.size=1,.nel=1},permissions={.htable=permission_buckets,.size=1,.nel=2};write_node.next=&append_node;
 file.s.value=1;file.permissions.table=&permissions;write.s.value=2;append.s.value=3;
 policy.policy_type=POLICY_KERN;policy.target_platform=SEPOL_TARGET_SELINUX;policy.p_types.nprim=4;policy.type_val_to_struct=refs;policy.type_attr_map=attrs;
 policy.p_classes.nprim=1;policy.p_classes.table=&class_table;policy.class_val_to_struct=classes;
 for(unsigned i=0;i<4;i++){refs[i]=&types[i];types[i].s.value=i+1;attrs[i].node=&bits[i];attrs[i].highbit=MAPSIZE;bits[i].map=MAPBIT<<i;}
 types[2].flavor=types[3].flavor=TYPE_ATTRIB;bits[0].map|=MAPBIT<<2;bits[1].map|=MAPBIT<<3;
 struct avtab_node normal={0},conditional={0};normal.key=(avtab_key_t){.source_type=3,.target_type=4,.target_class=1,.specified=AVTAB_ALLOWED};normal.datum.data=2;conditional.key=normal.key;conditional.datum.data=4;
 avtab_ptr_t normal_buckets[1]={&normal},conditional_buckets[1]={&conditional};policy.te_avtab=(avtab_t){.htable=normal_buckets,.nslot=1,.nel=1};policy.te_cond_avtab=(avtab_t){.htable=conditional_buckets,.nslot=1,.nel=1};
 sl_selinux_transition_edge edges[3][16]={0};unsigned matches[3][8]={{0}};sl_selinux_rule_visit visits[3][2]={0};sl_selinux_helper_visit helpers[3][32]={0};sl_selinux_transition_matrix roots[3]={0};
 for(unsigned i=0;i<3;i++){roots[i].edges=edges[i];roots[i].edge_capacity=16;roots[i].matches=matches[i];roots[i].match_capacity=8;roots[i].custody.policy_original=&policy;roots[i].custody.visits=visits[i];roots[i].custody.capacity=2;roots[i].custody.helpers=helpers[i];roots[i].custody.helper_capacity=i==1?7:32;roots[i].custody.string_capacity=32;}
 assert(sl_selinux_interpret_transition_matrix(&roots[0])==0&&roots[0].returned&&roots[0].types==4);
 assert(roots[0].transition_original==&write&&roots[0].dyntransition_original==&append&&edges[0][1].source_original==&types[0]&&edges[0][1].target_original==&types[1]);
 assert(edges[0][1].transition&&edges[0][1].dyntransition&&edges[0][1].transition_original==&normal&&edges[0][1].dyntransition_original==&conditional);
 assert(!edges[0][0].transition&&!edges[0][6].transition&&!edges[0][9].transition&&visits[0][1].conditional&&visits[0][1].matched&&!(conditional.key.specified&AVTAB_ENABLED));
 unsigned long prior=roots[0].custody.helper_count;assert(sl_selinux_interpret_transition_matrix(&roots[0])<0&&roots[0].custody.helper_count==prior&&edges[0][1].transition);
 assert(sl_selinux_interpret_transition_matrix(&roots[1])<0&&roots[1].custody.helper_count==7&&!roots[1].custody.count&&edges[0][1].dyntransition);
 policydb_t permissive_policy=policy;ebitmap_node_t permissive_node={.map=MAPBIT<<2};permissive_policy.permissive_map=(ebitmap_t){.node=&permissive_node,.highbit=MAPSIZE};roots[2].custody.policy_original=&permissive_policy;
 assert(sl_selinux_interpret_transition_matrix(&roots[2])==0);
 assert(edges[2][4].permissive&&edges[2][4].permissive_original==&permissive_policy.permissive_map&&edges[2][4].transition&&edges[2][4].dyntransition&&!edges[2][4].transition_original&&!edges[2][4].dyntransition_original);
 assert(edges[2][5].transition&&!edges[2][6].transition&&!edges[2][7].transition&&!edges[0][4].transition&&edges[0][1].transition_original==&normal);
 return 0;
}