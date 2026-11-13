#include "candidate_exec_context_linux.h"
#include <limits.h>
static sl_exec_context_visit *visit(sl_linux_candidate_exec_context *o,enum sl_exec_visit_kind kind){if(o->count==o->actual_capacity)return NULL;sl_exec_context_visit *v=&o->actual_visits[o->count++];v->kind=kind;return v;}
static int record_map(sl_linux_candidate_exec_context *o,sl_exec_level_comparison *level,unsigned which,const ebitmap_t *map){
 unsigned long begin=o->count;unsigned prior=0,have=0;level->begins[which]=begin;
 if(!map||map->highbit%MAPSIZE)return -1;
 for(ebitmap_node_t *node=map->node;node;node=node->next){
  for(unsigned long i=begin;i<o->count;i++)if(o->actual_visits[i].bitmap_node_original==node)return -1;
  sl_exec_context_visit *v=visit(o,SL_EXEC_BITMAP);if(!v)return -1;v->bitmap_original=map;v->bitmap_node_original=node;v->startbit=node->startbit;v->map=node->map;
  if(node->startbit%MAPSIZE||node->startbit>=map->highbit||(have&&node->startbit<=prior))return -1;prior=node->startbit;have=1;
 }
 level->counts[which]=o->count-begin;return 0;
}
static int compare_level(sl_linux_candidate_exec_context *o,sl_exec_level_comparison *l){
 l->result=-1;if(!l->expected||!l->left||(l->intersection&&!l->right)||l->expected->sens!=l->sensitivity)return -1;
 if(record_map(o,l,0,&l->expected->cat)||record_map(o,l,1,&l->left->cat)||(l->intersection&&record_map(o,l,2,&l->right->cat)))return -1;
 unsigned long indexes[3]={0};
 while(indexes[0]<l->counts[0]||indexes[1]<l->counts[1]||(l->intersection&&indexes[2]<l->counts[2])){
  unsigned bit=UINT_MAX;for(unsigned i=0;i<(l->intersection?3:2);i++)if(indexes[i]<l->counts[i]){unsigned start=o->actual_visits[l->begins[i]+indexes[i]].startbit;if(start<bit)bit=start;}
  MAPTYPE values[3]={0};for(unsigned i=0;i<(l->intersection?3:2);i++)if(indexes[i]<l->counts[i]){sl_exec_context_visit *v=&o->actual_visits[l->begins[i]+indexes[i]];if(v->startbit==bit){values[i]=v->map;indexes[i]++;}}
  l->examined++;if(values[0]!=(l->intersection?(values[1]&values[2]):values[1]))return -1;
 }
 l->result=0;return 0;
}
static int roles(sl_linux_candidate_exec_context *o,policydb_t *p,const context_struct_t *source,const context_struct_t *file,unsigned class_id){
 unsigned long begin=o->count;
 for(role_trans_t *r=p->role_tr;r;r=r->next){
  for(unsigned long i=begin;i<o->count;i++)if(o->actual_visits[i].role_original==r)return -1;
  sl_exec_context_visit *v=visit(o,SL_EXEC_ROLE);if(!v)return -1;v->role_original=r;
  if(!r->role||r->role>p->p_roles.nprim||!r->new_role||r->new_role>p->p_roles.nprim||!r->type||r->type>p->p_types.nprim||!r->tclass||r->tclass>p->p_classes.nprim)return -1;
  if(r->role==source->role&&r->type==file->type&&r->tclass==class_id){if(o->role_matches&&o->new_role!=r->new_role)return -1;o->role_original=r;o->role_matches++;o->new_role=r->new_role;}
 }
 return o->new_role==source->role?0:-1;
}
static int ranges(sl_linux_candidate_exec_context *o,policydb_t *p,const context_struct_t *source,const context_struct_t *file,unsigned class_id){
 hashtab_t table=p->range_tr;unsigned long nodes=0,begin=o->count;
 if(!table||(table->size&&!table->htable)||(!table->size&&table->nel))return -1;
 for(unsigned long bucket=0;bucket<table->size;bucket++){
  sl_exec_context_visit *b=visit(o,SL_EXEC_RANGE_BUCKET);if(!b)return -1;b->table_original=table;b->bucket=bucket;
  for(hashtab_ptr_t node=table->htable[bucket];node;node=node->next){
   for(unsigned long i=begin;i<o->count;i++)if(o->actual_visits[i].node_original==node)return -1;
   sl_exec_context_visit *v=visit(o,SL_EXEC_RANGE_NODE);if(!v)return -1;v->table_original=table;v->node_original=node;v->bucket=bucket;v->key_original=(range_trans_t *)node->key;v->range_original=node->datum;nodes++;
   range_trans_t *key=v->key_original;if(!key||!v->range_original||!key->source_type||key->source_type>p->p_types.nprim||!key->target_type||key->target_type>p->p_types.nprim||!key->target_class||key->target_class>p->p_classes.nprim)return -1;
   if(key->source_type==source->type&&key->target_type==file->type&&key->target_class==class_id){if(o->range_matches)return -1;o->range_matches++;o->range_original=v->range_original;}
  }
 }
 if(nodes!=table->nel)return -1;
 for(unsigned i=0;i<2;i++){
  sl_exec_level_comparison *l=&o->levels[i];l->expected=&source->range.level[i];
  if(o->range_original)l->left=&o->range_original->level[i];
  else switch(o->default_range){
   case 0:case DEFAULT_SOURCE_LOW_HIGH:l->left=&source->range.level[i];break;
   case DEFAULT_SOURCE_LOW:l->left=&source->range.level[0];break;
   case DEFAULT_SOURCE_HIGH:l->left=&source->range.level[1];break;
   case DEFAULT_TARGET_LOW:l->left=&file->range.level[0];break;
   case DEFAULT_TARGET_HIGH:l->left=&file->range.level[1];break;
   case DEFAULT_TARGET_LOW_HIGH:l->left=&file->range.level[i];break;
   case DEFAULT_GLBLUB:
    if(source->range.level[1].sens<file->range.level[0].sens||file->range.level[1].sens<source->range.level[0].sens)return -1;
    l->left=&source->range.level[i];l->right=&file->range.level[i];l->intersection=1;break;
   default:return -1;
  }
  l->sensitivity=l->left->sens;if(l->intersection){unsigned right=l->right->sens;if((!i&&right>l->sensitivity)||(i&&right<l->sensitivity))l->sensitivity=right;}
  if(compare_level(o,l))return -1;
 }
 return 0;
}
int sl_linux_interpret_exec_context_stability(sl_linux_candidate_exec_context *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;o->actual_source=o->source_original;o->actual_file=o->file_original;o->actual_visits=o->visits;o->actual_capacity=o->capacity;
 sl_linux_selinux_context *source=o->actual_source,*file=o->actual_file;sl_selinux_exec_type *type=o->type_original;
 if(!source||!file||!source->returned||source->result||!file->returned||file->result||!source->context_original||!file->context_original||!source->actual_parse||source->actual_parse!=file->actual_parse||!type||!type->returned||type->result||type->custody.actual_policy!=&source->actual_parse->parser.policy||type->custody.actual_source!=source->type||type->custody.actual_target!=file->type||!type->custody.class_original||!o->actual_visits||!o->actual_capacity||o->count||o->role_matches||o->range_matches)goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++){sl_exec_context_visit *v=&o->actual_visits[i];if(v->kind||v->role_original||v->table_original||v->node_original||v->key_original||v->range_original||v->bitmap_original||v->bitmap_node_original||v->bucket||v->startbit||v->map)goto done;}
 policydb_t *p=type->custody.actual_policy;if(type->custody.actual_class!=p->process_class)goto done;o->class_original=type->custody.class_original;o->default_user=(unsigned char)o->class_original->default_user;o->default_role=(unsigned char)o->class_original->default_role;o->default_range=(unsigned char)o->class_original->default_range;
 if((o->default_user&&o->default_user!=DEFAULT_SOURCE&&o->default_user!=DEFAULT_TARGET)||(o->default_role&&o->default_role!=DEFAULT_SOURCE&&o->default_role!=DEFAULT_TARGET))goto done;
 o->new_user=o->default_user==DEFAULT_TARGET?file->user:source->user;o->new_role=o->default_role==DEFAULT_TARGET?file->role:source->role;
 if(o->new_user!=source->user||roles(o,p,source->context_original,file->context_original,type->custody.actual_class))goto done;
 if(p->mls&&ranges(o,p,source->context_original,file->context_original,type->custody.actual_class))goto done;
 o->result=0;
 done:o->returned=1;return o->result;
 /* SAME-policy prediction only. Actual postexec CURRENT/validity/kernel SID
  * and the initial closed-entry/OS premise stay independently required. */
}