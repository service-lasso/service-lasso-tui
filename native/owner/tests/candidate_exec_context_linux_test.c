/* UNRUN actual full-context prediction; no kernel SID or policy authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_exec_context_linux.h"
#include <assert.h>
#include "../candidate_exec_context_linux.c"
typedef struct{
 sl_linux_candidate_exec_context root;sl_linux_selinux_context source,file;
 sl_linux_policy_parse parse;sl_selinux_exec_type type;class_datum_t class;
 context_struct_t source_context,file_context;role_trans_t role;
 hashtab_val_t ranges;hashtab_ptr_t buckets[1];hashtab_node_t node;range_trans_t key;mls_range_t range;
 ebitmap_node_t source_nodes[2],file_nodes[2],range_nodes[2];sl_exec_context_visit visits[32];
} fixture;
static void level(mls_level_t *l,ebitmap_node_t *node,unsigned sens,MAPTYPE bits){l->sens=sens;l->cat.highbit=MAPSIZE;l->cat.node=node;node->startbit=0;node->map=bits;}
static void prepare(fixture *f){
 f->root.source_original=&f->source;f->root.file_original=&f->file;f->root.type_original=&f->type;f->root.visits=f->visits;f->root.capacity=32;
 f->source.returned=f->file.returned=1;f->source.context_original=&f->source_context;f->file.context_original=&f->file_context;f->source.actual_parse=f->file.actual_parse=&f->parse;
 f->source.user=f->source.role=f->source.type=1;f->file.user=f->file.role=f->file.type=2;
 f->source_context.user=f->source_context.role=f->source_context.type=1;f->file_context.user=f->file_context.role=f->file_context.type=2;
 policydb_t *p=&f->parse.parser.policy;p->p_roles.nprim=p->p_types.nprim=2;p->p_classes.nprim=p->process_class=1;p->mls=1;p->range_tr=&f->ranges;
 f->ranges.size=1;f->ranges.htable=f->buckets;
 f->type.returned=1;f->type.custody.actual_policy=p;f->type.custody.actual_source=1;f->type.custody.actual_target=2;f->type.custody.actual_class=1;f->type.custody.class_original=&f->class;
 for(unsigned i=0;i<2;i++){level(&f->source_context.range.level[i],&f->source_nodes[i],i+1,3);level(&f->file_context.range.level[i],&f->file_nodes[i],i+1,7);level(&f->range.level[i],&f->range_nodes[i],i+1,3);}
}
int main(void){fixture cases[9]={0};for(unsigned i=0;i<9;i++){fixture *f=&cases[i];prepare(f);
 if(i==1)f->class.default_user=DEFAULT_TARGET;
 if(i==2)f->class.default_role=DEFAULT_TARGET;
 if(i==3){f->parse.parser.policy.role_tr=&f->role;f->role.role=1;f->role.type=2;f->role.tclass=1;f->role.new_role=2;}
 if(i==4)f->class.default_range=DEFAULT_TARGET_LOW_HIGH;
 if(i==5){f->ranges.nel=1;f->buckets[0]=&f->node;f->node.key=(char *)&f->key;f->node.datum=&f->range;f->key.source_type=1;f->key.target_type=2;f->key.target_class=1;}
 if(i==6)f->class.default_range=DEFAULT_GLBLUB;
 if(i==7)f->root.capacity=1;
 if(i==8){f->class.default_range=DEFAULT_GLBLUB;f->file_nodes[0].map=1;}
 int result=sl_linux_interpret_exec_context_stability(&f->root);assert(f->root.returned&&f->root.actual_source==&f->source&&f->root.actual_file==&f->file&&f->root.class_original==&f->class);
 if(i==0||i==5||i==6){assert(!result&&f->root.new_user==1&&f->root.new_role==1);if(i==5)assert(f->root.range_original==&f->range&&f->root.range_matches==1&&f->root.levels[0].left==&f->range.level[0]);if(i==6)assert(f->root.levels[0].intersection&&f->root.levels[0].left==&f->source_context.range.level[0]&&f->root.levels[0].right==&f->file_context.range.level[0]);}
 else assert(result==-1);
 if(i==3)assert(f->root.role_original==&f->role&&f->root.new_role==2);
 if(i==7)assert(f->root.count==1&&f->visits[0].kind==SL_EXEC_RANGE_BUCKET);
 unsigned long count=f->root.count;assert(sl_linux_interpret_exec_context_stability(&f->root)==-1&&f->root.count==count&&f->root.result==result);
 }return 0;}