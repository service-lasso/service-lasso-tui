/* UNRUN source-owned parsed-object cases; no kernel/creation/seal authority. */
#include "../selinux_policy_relations.h"
#include <assert.h>
#include "../selinux_policy_relations.c"
typedef struct creation_case {
 policydb_t policy;type_datum_t types[5];type_datum_t *refs[5];
 ebitmap_t attrs[5];ebitmap_node_t bits[5];
 class_datum_t file;class_datum_t *classes[1];hashtab_node_t class_node;
 hashtab_ptr_t class_buckets[1];hashtab_val_t class_table;
 struct avtab_node permanent,conditional,allow;avtab_ptr_t permanent_buckets[1],conditional_buckets[1];
 avtab_ptr_t empty_buckets[4];
 perm_datum_t write;hashtab_node_t permission_node;hashtab_ptr_t permission_buckets[1];hashtab_val_t permissions;
 filename_trans_key_t key;filename_trans_datum_t datum;ebitmap_node_t source_bit;
 hashtab_node_t filename_node;hashtab_ptr_t filename_buckets[1];hashtab_val_t filename_table;
 unsigned output[5];type_datum_t *referents[5];sl_selinux_rule_visit visits[3];
 sl_selinux_helper_visit helpers[128];sl_selinux_creation_types root;
} creation_case;
static void prepare(creation_case *c){
 c->policy.policy_type=POLICY_KERN;c->policy.target_platform=SEPOL_TARGET_SELINUX;
 c->policy.p_types.nprim=5;c->policy.type_val_to_struct=c->refs;c->policy.type_attr_map=c->attrs;
 for(unsigned i=0;i<5;i++){c->types[i].s.value=i+1;c->types[i].flavor=TYPE_TYPE;c->refs[i]=&c->types[i];c->bits[i].map=MAPBIT<<i;c->attrs[i].node=&c->bits[i];c->attrs[i].highbit=MAPSIZE;}
 c->file.s.value=1;c->classes[0]=&c->file;c->class_node.key="file";c->class_node.datum=&c->file;c->class_buckets[0]=&c->class_node;
 c->class_table=(hashtab_val_t){.htable=c->class_buckets,.size=1,.nel=1};
 c->policy.p_classes.nprim=1;c->policy.p_classes.table=&c->class_table;c->policy.class_val_to_struct=c->classes;
 c->permanent.key=(avtab_key_t){.source_type=1,.target_type=2,.target_class=1,.specified=AVTAB_TRANSITION};c->permanent.datum.data=3;
 c->conditional.key=c->permanent.key;c->conditional.datum.data=4;
 c->write.s.value=1;c->permission_node.key="write";c->permission_node.datum=&c->write;c->permission_buckets[0]=&c->permission_node;c->permissions=(hashtab_val_t){.htable=c->permission_buckets,.size=1,.nel=1};c->file.permissions.table=&c->permissions;
 c->allow.key=(avtab_key_t){.source_type=1,.target_type=4,.target_class=1,.specified=AVTAB_ALLOWED};c->allow.datum.data=1;c->permanent.next=&c->allow;
 c->permanent_buckets[0]=&c->permanent;c->conditional_buckets[0]=&c->conditional;
 c->policy.te_avtab=(avtab_t){.htable=c->permanent_buckets,.nslot=1,.nel=2};c->policy.te_cond_avtab=(avtab_t){.htable=c->conditional_buckets,.nslot=1,.nel=1};
 c->key=(filename_trans_key_t){.ttype=2,.tclass=1,.name="actual-name"};c->source_bit.map=MAPBIT;
 c->datum.otype=5;c->datum.stypes=(ebitmap_t){.node=&c->source_bit,.highbit=MAPSIZE};
 c->filename_node.key=(char*)&c->key;c->filename_node.datum=&c->datum;c->filename_buckets[0]=&c->filename_node;
 c->filename_table=(hashtab_val_t){.htable=c->filename_buckets,.size=1,.nel=1};c->policy.filename_trans=&c->filename_table;c->policy.filename_trans_count=1;
 c->root.types=c->output;c->root.referents=c->referents;c->root.capacity=5;
 c->root.custody=(sl_selinux_te_union){.policy_original=&c->policy,.source_type=1,.target_type=2,.visits=c->visits,.capacity=3,.helpers=c->helpers,.helper_capacity=128,.string_capacity=32};
}
int main(void){
 static creation_case cases[8];for(unsigned i=0;i<8;i++)prepare(&cases[i]);
 creation_case *good=&cases[0];assert(sl_selinux_interpret_creation_types(&good->root)==0&&good->root.count==5);
 for(unsigned i=0;i<5;i++)assert(good->output[i]&&good->referents[i]==&good->types[i]);
 assert(good->visits[2].conditional&&good->visits[2].matched&&!(good->conditional.key.specified&AVTAB_ENABLED));
 unsigned saw_datum=0;for(unsigned long i=0;i<good->root.custody.helper_count;i++)if(good->helpers[i].filename_datum_original==&good->datum)saw_datum++;
 assert(saw_datum==1);unsigned long old=good->root.custody.helper_count;
 assert(sl_selinux_interpret_creation_types(&good->root)<0&&good->root.custody.helper_count==old&&good->root.result==0);
 cases[1].source_bit.map=MAPBIT<<1;assert(sl_selinux_interpret_creation_types(&cases[1].root)==0&&cases[1].root.count==4&&!cases[1].output[4]);
 cases[2].file.default_type=99;assert(sl_selinux_interpret_creation_types(&cases[2].root)<0&&!cases[2].root.count);
 cases[3].datum.otype=6;assert(sl_selinux_interpret_creation_types(&cases[3].root)<0&&cases[3].root.returned&&cases[3].datum.otype==6);
 cases[4].datum.next=&cases[4].datum;assert(sl_selinux_interpret_creation_types(&cases[4].root)<0&&cases[4].datum.next==&cases[4].datum);
 cases[5].root.custody.string_capacity=4;assert(sl_selinux_interpret_creation_types(&cases[5].root)<0&&!cases[5].root.custody.count);
 cases[6].root.custody.helper_capacity=1;assert(sl_selinux_interpret_creation_types(&cases[6].root)<0&&cases[6].root.custody.helper_count==1&&!cases[6].root.count);
 cases[7].policy.te_avtab=(avtab_t){.htable=cases[7].empty_buckets,.nslot=4};cases[7].policy.te_cond_avtab=(avtab_t){0};cases[7].policy.filename_trans=NULL;cases[7].policy.filename_trans_count=0;cases[7].root.custody.helper_capacity=5;
 assert(sl_selinux_interpret_creation_types(&cases[7].root)<0&&cases[7].root.custody.helper_count==5&&!cases[7].root.custody.count&&cases[7].root.count==2);
 assert(cases[7].helpers[4].kind==SL_SELINUX_AVTAB_BUCKET&&cases[7].helpers[4].bucket==2&&cases[7].helpers[4].avtab_original==&cases[7].policy.te_avtab);
 assert(good->root.result==0&&good->root.count==5&&good->referents[4]==&good->types[4]&&cases[4].root.result<0);
 sl_selinux_source_access sources[5]={0};sl_selinux_rule_visit visits[3]={0};sl_selinux_helper_visit helpers[128]={0};
 sl_selinux_access_sources access={.sources=sources,.source_capacity=5,.custody={.policy_original=&good->policy,.visits=visits,.capacity=3,.helpers=helpers,.helper_capacity=128,.string_capacity=32}};
 assert(sl_selinux_interpret_creation_permission_sources(&access,&good->root,"write",NULL)==0);
 assert(access.actual_creation==&good->root&&access.custody.count==3&&sources[0].possible==1&&visits[1].node_original==&good->allow&&visits[1].matched);
 assert(good->root.custody.helper_count==old&&good->root.count==5&&good->referents[4]==&good->types[4]);
 return 0;
}
