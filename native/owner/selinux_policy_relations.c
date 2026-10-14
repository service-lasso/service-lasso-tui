#include "selinux_policy_relations.h"
#include <limits.h>
const char sl_selinux_name_file[]="file";
const char sl_selinux_name_dir[]="dir";
const char sl_selinux_name_process[]="process";
const char sl_selinux_name_filesystem[]="filesystem";
const char sl_selinux_name_unix_stream_socket[]="unix_stream_socket";
const char sl_selinux_name_write[]="write";
const char sl_selinux_name_append[]="append";
const char sl_selinux_name_relabelfrom[]="relabelfrom";
const char sl_selinux_name_relabelto[]="relabelto";
const char sl_selinux_name_associate[]="associate";
const char sl_selinux_name_ptrace[]="ptrace";
const char sl_selinux_name_transition[]="transition";
const char sl_selinux_name_dyntransition[]="dyntransition";
const char sl_selinux_name_read[]="read";
const char sl_selinux_name_create[]="create";
const char sl_selinux_name_entrypoint[]="entrypoint";
const char sl_selinux_name_execute_no_trans[]="execute_no_trans";
const char sl_selinux_name_setattr[]="setattr";
const char sl_selinux_name_ioctl[]="ioctl";
const char sl_selinux_name_unlink[]="unlink";
const char sl_selinux_name_link[]="link";
const char sl_selinux_name_add_name[]="add_name";
const char sl_selinux_name_remove_name[]="remove_name";
const char sl_selinux_name_rename[]="rename";
const char sl_selinux_name_reparent[]="reparent";
const sl_selinux_lookup_source sl_selinux_lookup_sources[25]={
{sl_selinux_name_file,sizeof(sl_selinux_name_file)},
{sl_selinux_name_dir,sizeof(sl_selinux_name_dir)},
{sl_selinux_name_process,sizeof(sl_selinux_name_process)},
{sl_selinux_name_filesystem,sizeof(sl_selinux_name_filesystem)},
{sl_selinux_name_unix_stream_socket,sizeof(sl_selinux_name_unix_stream_socket)},
{sl_selinux_name_write,sizeof(sl_selinux_name_write)},
{sl_selinux_name_append,sizeof(sl_selinux_name_append)},
{sl_selinux_name_relabelfrom,sizeof(sl_selinux_name_relabelfrom)},
{sl_selinux_name_relabelto,sizeof(sl_selinux_name_relabelto)},
{sl_selinux_name_associate,sizeof(sl_selinux_name_associate)},
{sl_selinux_name_ptrace,sizeof(sl_selinux_name_ptrace)},
{sl_selinux_name_transition,sizeof(sl_selinux_name_transition)},
{sl_selinux_name_dyntransition,sizeof(sl_selinux_name_dyntransition)},
{sl_selinux_name_read,sizeof(sl_selinux_name_read)},
{sl_selinux_name_create,sizeof(sl_selinux_name_create)},
{sl_selinux_name_entrypoint,sizeof(sl_selinux_name_entrypoint)},
{sl_selinux_name_execute_no_trans,sizeof(sl_selinux_name_execute_no_trans)},
{sl_selinux_name_setattr,sizeof(sl_selinux_name_setattr)},
{sl_selinux_name_ioctl,sizeof(sl_selinux_name_ioctl)},
{sl_selinux_name_unlink,sizeof(sl_selinux_name_unlink)},
{sl_selinux_name_link,sizeof(sl_selinux_name_link)},
{sl_selinux_name_add_name,sizeof(sl_selinux_name_add_name)},
{sl_selinux_name_remove_name,sizeof(sl_selinux_name_remove_name)},
{sl_selinux_name_rename,sizeof(sl_selinux_name_rename)},
{sl_selinux_name_reparent,sizeof(sl_selinux_name_reparent)},
};
const sl_selinux_lookup_source *sl_selinux_lookup_source_for(const char *name){
 for(unsigned i=0;i<sizeof(sl_selinux_lookup_sources)/sizeof(sl_selinux_lookup_sources[0]);i++)if(name==sl_selinux_lookup_sources[i].bytes)return &sl_selinux_lookup_sources[i];return NULL;
}
static int relation_policy(policydb_t *p){
 return p&&p->policy_type==POLICY_KERN&&p->target_platform==SEPOL_TARGET_SELINUX&&
  p->p_types.nprim&&p->p_types.nprim<=UINT16_MAX&&p->type_val_to_struct;
}
int sl_selinux_interpret_bounds(sl_selinux_bounds *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_old=o->old_type;o->actual_new=o->new_type;
 o->actual_visited=o->visited;o->actual_capacity=o->capacity;
 policydb_t *p=o->actual_policy;unsigned index=o->actual_new;
 if(!relation_policy(p)||!o->actual_old||o->actual_old>p->p_types.nprim||
    !index||index>p->p_types.nprim||index==o->actual_old||!o->actual_visited||!o->actual_capacity||o->count)goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++)if(o->actual_visited[i])goto done;
 /* Selection requires a DISTINCT existing bounded reduction, not same-type
  * success. Record each actual type object before following its bounds. */
 while(o->count<o->actual_capacity){
  type_datum_t *type=p->type_val_to_struct[index-1];
  if(!type||type->s.value!=index||type->flavor!=TYPE_TYPE)goto done;
  for(unsigned long i=0;i<o->count;i++)if(o->actual_visited[i]==type)goto done;
  o->actual_visited[o->count++]=type;
  if(!type->bounds||type->bounds>p->p_types.nprim)goto done;
  if(type->bounds==o->actual_old){
   type_datum_t *old=p->type_val_to_struct[o->actual_old-1];
   if(!old||old->s.value!=o->actual_old||old->flavor!=TYPE_TYPE)goto done;
   o->result=0;break;
  }
  index=type->bounds;
 }
done:o->returned=1;return o->result;
}
static sl_selinux_helper_visit *relation_helper(sl_selinux_te_union *o,enum sl_selinux_helper_kind kind){
 if(o->helper_count==o->actual_helper_capacity)return NULL;
 sl_selinux_helper_visit *v=&o->actual_helpers[o->helper_count++];v->kind=kind;return v;
}
static int relation_lookup(sl_selinux_te_union *o,hashtab_t table,const char **slot,void **out){
 unsigned long begin=o->helper_count,nodes=0;const char *name=slot?*slot:NULL;*out=NULL;
 if(!table||!name||(table->size&&!table->htable)||(!table->size&&table->nel))return -1;
 for(unsigned long bucket=0;bucket<table->size;bucket++){
  sl_selinux_helper_visit *v=relation_helper(o,SL_SELINUX_HASH_BUCKET);if(!v)return -1;
  v->table_original=table;v->bucket=bucket;v->name_original=name;v->name_slot_original=slot;v->name_source_original=sl_selinux_lookup_source_for(name);
  hashtab_ptr_t node=table->htable[bucket];
  while(node){
   for(unsigned long i=begin;i<o->helper_count;i++)if(o->actual_helpers[i].kind==SL_SELINUX_HASH_NODE&&o->actual_helpers[i].hash_original==node)return -1;
   v=relation_helper(o,SL_SELINUX_HASH_NODE);if(!v)return -1;
   v->table_original=table;v->bucket=bucket;v->hash_original=node;v->name_original=name;v->name_slot_original=slot;v->name_source_original=sl_selinux_lookup_source_for(name);v->key_original=node->key;nodes++;
   if(!node->key)return -1;
   int equal=1;unsigned long at=0;
   /* Complete both strings within the selected finite extent, even after a
    * differing byte. Retain the actual comparison occurrence and aliases. */
   unsigned name_end=0,key_end=0;
   for(;at<o->actual_string_capacity;at++){
    unsigned char a=0,b=0;
    if(!name_end){a=(unsigned char)name[at];v->name_read_extent=at+1;if(!a)v->name_terminated=1;}
    if(!key_end){b=(unsigned char)node->key[at];v->key_read_extent=at+1;if(!b)v->key_terminated=1;}
    if(a!=b)equal=0;if(!a)name_end=1;if(!b)key_end=1;v->compared=at+1;
    if(name_end&&key_end)break;
   }
   if(!name_end||!key_end)return -1;
   if(equal){if(*out||!node->datum)return -1;*out=node->datum;v->matched=1;}
   node=node->next;
  }
 }
 return nodes==table->nel?0:-1;
}
static int relation_bit(sl_selinux_te_union *o,ebitmap_t *bitmap,unsigned bit){
 unsigned long begin=o->helper_count;unsigned previous_end=0;int matched=0;
 ebitmap_node_t *node=bitmap->node;
 if(!node)return bitmap->highbit?-1:0;
 while(node){
  for(unsigned long i=begin;i<o->helper_count;i++)if(o->actual_helpers[i].bitmap_node_original==node)return -1;
  sl_selinux_helper_visit *v=relation_helper(o,SL_SELINUX_BITMAP_NODE);if(!v)return -1;
  v->bitmap_original=bitmap;v->bitmap_node_original=node;v->bit=bit;v->startbit=node->startbit;v->highbit=bitmap->highbit;v->map=node->map;
  if(node->startbit%MAPSIZE||node->startbit>UINT_MAX-MAPSIZE||node->startbit<previous_end||!node->map||node->startbit+MAPSIZE>bitmap->highbit)return -1;
  previous_end=node->startbit+MAPSIZE;
  if(bit>=node->startbit&&bit-node->startbit<MAPSIZE){v->matched=!!(node->map&(MAPBIT<<(bit-node->startbit)));matched=v->matched;}
  node=node->next;
 }
 return previous_end==bitmap->highbit?matched:-1;
}
static int relation_table(sl_selinux_te_union *o,avtab_t *table,unsigned conditional){
 if((table->nslot&&!table->htable)||(!table->nslot&&table->nel))return -1;
 unsigned long begin=o->count;
 for(unsigned long bucket=0;bucket<table->nslot;bucket++){
  sl_selinux_helper_visit *bucket_visit=relation_helper(o,SL_SELINUX_AVTAB_BUCKET);if(!bucket_visit)return -1;
  bucket_visit->avtab_original=table;bucket_visit->bucket=bucket;
  avtab_ptr_t node=table->htable[bucket];
  while(node){
   if(o->count==o->actual_capacity)return -1;
   /* Every traversed actual member has a distinct preowned visit. Detect a
    * repeated SAME node before following it; no unbounded cyclic traversal. */
   for(unsigned long i=0;i<o->count;i++)if(o->actual_visits[i].node_original==node)return -1;
   sl_selinux_rule_visit *visit=&o->actual_visits[o->count++];
   visit->table_original=table;visit->node_original=node;visit->bucket=bucket;visit->conditional=conditional;
   unsigned specified=node->key.specified&~(AVTAB_ENABLED|AVTAB_ENABLED_OLD);
   if(specified!=AVTAB_ALLOWED&&specified!=AVTAB_AUDITALLOW&&specified!=AVTAB_AUDITDENY&&
      specified!=AVTAB_TRANSITION&&specified!=AVTAB_MEMBER&&specified!=AVTAB_CHANGE&&
      specified!=AVTAB_XPERMS_ALLOWED&&specified!=AVTAB_XPERMS_AUDITALLOW&&specified!=AVTAB_XPERMS_DONTAUDIT)return -1;
   if(!node->key.source_type||node->key.source_type>o->actual_policy->p_types.nprim||
      !node->key.target_type||node->key.target_type>o->actual_policy->p_types.nprim||
      !node->key.target_class||node->key.target_class>o->actual_policy->p_classes.nprim)return -1;
   /* Conditional enable state is deliberately ignored: union BOTH possible
    * branches. This overapproximates TE permissions; it cannot grant access. */
   int source=0,target=0;
   if(specified==AVTAB_ALLOWED&&node->key.target_class==o->actual_class){
    source=relation_bit(o,o->source_attributes,node->key.source_type-1);
    target=relation_bit(o,o->target_attributes,node->key.target_type-1);
    if(source<0||target<0)return -1;
   }
   if(source&&target){
    visit->matched=1;
    if(conditional)o->conditional|=node->datum.data;else o->unconditional|=node->datum.data;
   }
   node=node->next;
  }
 }
 return o->count-begin==table->nel?0:-1;
}
int sl_selinux_interpret_te_union(sl_selinux_te_union *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_source=o->source_type;o->actual_target=o->target_type;
 o->actual_class_name=o->class_name;o->actual_permission_name=o->permission_name;
 o->actual_visits=o->visits;o->actual_capacity=o->capacity;
 o->actual_helpers=o->helpers;o->actual_helper_capacity=o->helper_capacity;o->actual_string_capacity=o->string_capacity;
 policydb_t *p=o->actual_policy;
 if(!o->actual_helpers||!o->actual_helper_capacity||!o->actual_string_capacity||o->helper_count)goto done;
 for(unsigned long i=0;i<o->actual_helper_capacity;i++){
  sl_selinux_helper_visit *v=&o->actual_helpers[i];
  if(v->kind||v->avtab_original||v->table_original||v->hash_original||v->name_original||v->key_original||v->name_slot_original||v->name_source_original||v->bucket||v->compared||v->name_read_extent||v->key_read_extent||v->name_terminated||v->key_terminated||
     v->filename_key_original||v->filename_datum_original||v->bitmap_original||v->bitmap_node_original||v->bit||v->startbit||v->highbit||v->map||v->matched)goto done;
 }
 if(!relation_policy(p)||!p->type_attr_map||!o->actual_source||o->actual_source>p->p_types.nprim||
    !o->actual_target||o->actual_target>p->p_types.nprim||!o->actual_class_name||!o->actual_permission_name||
    !o->actual_visits||!o->actual_capacity||o->count||o->unconditional||o->conditional||o->possible)goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++){
  sl_selinux_rule_visit *v=&o->actual_visits[i];
  if(v->table_original||v->node_original||v->bucket||v->conditional||v->matched)goto done;
 }
 if(!p->p_classes.table||!p->class_val_to_struct)goto done;
 void *found=NULL;
 if(relation_lookup(o,p->p_classes.table,&o->actual_class_name,&found))goto done;
 o->class_original=found;
 if(!o->class_original||!o->class_original->s.value||o->class_original->s.value>p->p_classes.nprim||
    o->class_original->s.value>UINT16_MAX)goto done;
 o->actual_class=(sepol_security_class_t)o->class_original->s.value;
 if(p->class_val_to_struct[o->actual_class-1]!=o->class_original)goto done;
 if(relation_lookup(o,o->class_original->permissions.table,&o->actual_permission_name,&found))goto done;
 o->permission_original=found;
 if(!o->permission_original){
  o->common_original=o->class_original->comdatum;
  if(o->common_original){if(relation_lookup(o,o->common_original->permissions.table,&o->actual_permission_name,&found))goto done;o->permission_original=found;}
 }
 if(!o->permission_original||!o->permission_original->s.value||o->permission_original->s.value>32)goto done;
 o->actual_permission=UINT32_C(1)<<(o->permission_original->s.value-1);
 o->source_attributes=&p->type_attr_map[o->actual_source-1];
 o->target_attributes=&p->type_attr_map[o->actual_target-1];
 if(relation_table(o,&p->te_avtab,0)||relation_table(o,&p->te_cond_avtab,1))goto done;
 o->possible=(o->unconditional|o->conditional)&o->actual_permission;
 o->result=0;
done:o->returned=1;return o->result;
}
static int access_source_table(sl_selinux_access_sources *root,avtab_t *table,unsigned conditional){
 sl_selinux_te_union *o=&root->custody;
 if((table->nslot&&!table->htable)||(!table->nslot&&table->nel))return -1;
 unsigned long begin=o->count;
 for(unsigned long bucket=0;bucket<table->nslot;bucket++){
  sl_selinux_helper_visit *b=relation_helper(o,SL_SELINUX_AVTAB_BUCKET);if(!b)return -1;b->avtab_original=table;b->bucket=bucket;
  for(avtab_ptr_t node=table->htable[bucket];node;node=node->next){
   if(o->count==o->actual_capacity)return -1;
   for(unsigned long i=0;i<o->count;i++)if(o->actual_visits[i].node_original==node)return -1;
   sl_selinux_rule_visit *v=&o->actual_visits[o->count++];v->table_original=table;v->node_original=node;v->bucket=bucket;v->conditional=conditional;
   unsigned specified=node->key.specified&~(AVTAB_ENABLED|AVTAB_ENABLED_OLD);
   if(specified!=AVTAB_ALLOWED&&specified!=AVTAB_AUDITALLOW&&specified!=AVTAB_AUDITDENY&&specified!=AVTAB_TRANSITION&&specified!=AVTAB_MEMBER&&specified!=AVTAB_CHANGE&&specified!=AVTAB_XPERMS_ALLOWED&&specified!=AVTAB_XPERMS_AUDITALLOW&&specified!=AVTAB_XPERMS_DONTAUDIT)return -1;
   if(!node->key.source_type||node->key.source_type>o->actual_policy->p_types.nprim||!node->key.target_type||node->key.target_type>o->actual_policy->p_types.nprim||!node->key.target_class||node->key.target_class>o->actual_policy->p_classes.nprim)return -1;
   if(specified!=AVTAB_ALLOWED||node->key.target_class!=o->actual_class||!(node->datum.data&root->requested_permissions))continue;
   int target=0;
   if(root->actual_creation){
    /* One retained table visit; union target attributes across the original
     * creation set without allocating a per-target source inventory. */
    for(unsigned long t=0;t<root->actual_creation->actual_capacity;t++)if(root->actual_creation->actual_types[t]){
     int match=relation_bit(o,&o->actual_policy->type_attr_map[t],node->key.target_type-1);if(match<0)return -1;target|=match;
    }
   }else{target=relation_bit(o,o->target_attributes,node->key.target_type-1);if(target<0)return -1;}
   if(!target)continue;
   for(unsigned long i=0;i<root->source_count;i++){
    sl_selinux_source_access *source=&root->actual_sources[i];if(source->flavor==TYPE_ATTRIB)continue;
    int matched=relation_bit(o,&o->actual_policy->type_attr_map[i],node->key.source_type-1);if(matched<0)return -1;
    if(matched){v->matched=1;if(conditional)source->conditional|=node->datum.data&root->requested_permissions;else source->unconditional|=node->datum.data&root->requested_permissions;}
   }
  }
 }
 return o->count-begin==table->nel?0:-1;
}
static int interpret_source_permissions(sl_selinux_access_sources *root,const char *class_name,const char *first,const char *second,unsigned legacy){
 if(!root||root->attempted)return -1;root->attempted=1;root->result=-1;
 sl_selinux_te_union *o=&root->custody;
 if(o->attempted||o->returned){root->returned=1;return -1;}o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_target=o->target_type;o->actual_visits=o->visits;o->actual_capacity=o->capacity;
 o->actual_helpers=o->helpers;o->actual_helper_capacity=o->helper_capacity;o->actual_string_capacity=o->string_capacity;
 root->actual_sources=root->sources;root->actual_source_capacity=root->source_capacity;
 policydb_t *p=o->actual_policy;
 if(root->creation_original){
  sl_selinux_creation_types *c=root->creation_original;root->actual_creation=c;
  if(!c->returned||c->result||!c->custody.returned||c->custody.result||c->custody.actual_policy!=p||!c->actual_types||!c->actual_referents||!p||c->actual_capacity!=p->p_types.nprim||!c->count)goto done;
  unsigned long count=0;for(unsigned long i=0;i<c->actual_capacity;i++){
   if(c->actual_types[i]>1)goto done;
   if(c->actual_types[i]){count++;if(!p->type_val_to_struct||c->actual_referents[i]!=p->type_val_to_struct[i]||!c->actual_referents[i]||c->actual_referents[i]->s.value!=i+1||c->actual_referents[i]->flavor!=TYPE_TYPE)goto done;}
   else if(c->actual_referents[i])goto done;
  }if(count!=c->count)goto done;
 }
 if(!class_name||!first||root->permission_count||root->permission_names[0]||root->permission_names[1]||root->permission_originals[0]||root->permission_originals[1]||root->permission_bits[0]||root->permission_bits[1])goto done;
 root->permission_names[0]=first;root->permission_names[1]=second;root->permission_count=second?2:1;
 if(!relation_policy(p)||!p->type_attr_map||!p->class_val_to_struct||!root->actual_sources||root->actual_source_capacity<p->p_types.nprim||root->source_count||root->write_original||root->append_original||root->requested_permissions||!o->actual_target||o->actual_target>p->p_types.nprim||!o->actual_visits||!o->actual_capacity||o->count||!o->actual_helpers||!o->actual_helper_capacity||!o->actual_string_capacity||o->helper_count)goto done;
 for(unsigned long i=0;i<root->actual_source_capacity;i++)if(root->actual_sources[i].type_original||root->actual_sources[i].type||root->actual_sources[i].flavor||root->actual_sources[i].unconditional||root->actual_sources[i].conditional||root->actual_sources[i].possible||root->actual_sources[i].permissive||root->actual_sources[i].permissive_original)goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++)if(o->actual_visits[i].table_original||o->actual_visits[i].node_original||o->actual_visits[i].bucket||o->actual_visits[i].conditional||o->actual_visits[i].matched)goto done;
 for(unsigned long i=0;i<o->actual_helper_capacity;i++){sl_selinux_helper_visit *v=&o->actual_helpers[i];if(v->kind||v->avtab_original||v->table_original||v->hash_original||v->name_original||v->key_original||v->name_slot_original||v->name_source_original||v->bucket||v->compared||v->name_read_extent||v->key_read_extent||v->name_terminated||v->key_terminated||v->filename_key_original||v->filename_datum_original||v->bitmap_original||v->bitmap_node_original||v->bit||v->startbit||v->highbit||v->map||v->matched)goto done;}
 for(unsigned long i=0;i<p->p_types.nprim;i++){
  type_datum_t *type=p->type_val_to_struct[i];sl_selinux_source_access *source=&root->actual_sources[i];source->type_original=type;source->type=i+1;root->source_count=i+1;
  if(!type||type->s.value!=i+1||(type->flavor!=TYPE_TYPE&&type->flavor!=TYPE_ATTRIB))goto done;source->flavor=type->flavor;
 }
 void *found=NULL;o->actual_class_name=class_name;
 if(relation_lookup(o,p->p_classes.table,&o->actual_class_name,&found))goto done;o->class_original=found;
 if(!o->class_original||!o->class_original->s.value||o->class_original->s.value>p->p_classes.nprim||o->class_original->s.value>UINT16_MAX)goto done;
 o->actual_class=o->class_original->s.value;if(p->class_val_to_struct[o->actual_class-1]!=o->class_original)goto done;
 for(unsigned i=0;i<root->permission_count;i++){
  if(relation_lookup(o,o->class_original->permissions.table,&root->permission_names[i],&found))goto done;
  if(!found&&o->class_original->comdatum){o->common_original=o->class_original->comdatum;if(relation_lookup(o,o->common_original->permissions.table,&root->permission_names[i],&found))goto done;}
  perm_datum_t *permission=found;if(!permission||!permission->s.value||permission->s.value>32)goto done;
  sepol_access_vector_t bit=UINT32_C(1)<<(permission->s.value-1);
  root->permission_originals[i]=permission;root->permission_bits[i]=bit;
  if(legacy){if(i){root->append_original=permission;root->append_permission=bit;}else{root->write_original=permission;root->write_permission=bit;}}root->requested_permissions|=bit;
 }
 if(root->permission_count==2&&root->permission_bits[0]==root->permission_bits[1])goto done;
 o->target_attributes=&p->type_attr_map[o->actual_target-1];
 if(access_source_table(root,&p->te_avtab,0)||access_source_table(root,&p->te_cond_avtab,1))goto done;
 for(unsigned long i=0;i<root->source_count;i++){
  sl_selinux_source_access *source=&root->actual_sources[i];source->permissive_original=&p->permissive_map;
  if(source->flavor!=TYPE_ATTRIB){int permissive=relation_bit(o,&p->permissive_map,source->type);if(permissive<0)goto done;source->permissive=permissive;}
  source->possible=source->unconditional|source->conditional|(source->permissive?root->requested_permissions:0);
 }
 root->result=o->result=0;
 done:o->returned=1;root->returned=1;return root->result;
}
int sl_selinux_interpret_access_sources(sl_selinux_access_sources *root){return interpret_source_permissions(root,sl_selinux_name_file,sl_selinux_name_write,sl_selinux_name_append,1);}
int sl_selinux_interpret_permission_sources(sl_selinux_access_sources *root,const char *class_name,const char *permission_name){return interpret_source_permissions(root,class_name,permission_name,NULL,0);}
int sl_selinux_interpret_permission_pair_sources(sl_selinux_access_sources *root,const char *class_name,const char *first,const char *second){if(!second)return -1;return interpret_source_permissions(root,class_name,first,second,0);}
int sl_selinux_interpret_target_permission_sources(sl_selinux_access_sources *root,sl_selinux_creation_types *creation,const char *class_name,const char *first,const char *second){
 if(!root||root->attempted||root->creation_original||root->actual_creation||root->custody.target_type||!creation)return -1;
 root->creation_original=creation;
 /* The representative target supplies existing scalar geometry only. Actual
  * matching consumes the complete SAME frozen creation set in the table walk. */
 root->custody.target_type=creation->custody.actual_target;
 return interpret_source_permissions(root,class_name,first,second,0);
}
int sl_selinux_interpret_creation_permission_sources(sl_selinux_access_sources *root,sl_selinux_creation_types *creation,const char *first,const char *second){return sl_selinux_interpret_target_permission_sources(root,creation,sl_selinux_name_file,first,second);}
static int transition_table(sl_selinux_transition_matrix *root,avtab_t *table,unsigned conditional){
 sl_selinux_te_union *o=&root->custody;unsigned long n=root->types,begin=o->count;
 if((table->nslot&&!table->htable)||(!table->nslot&&table->nel))return -1;
 for(unsigned long bucket=0;bucket<table->nslot;bucket++){
  sl_selinux_helper_visit *b=relation_helper(o,SL_SELINUX_AVTAB_BUCKET);if(!b)return -1;b->avtab_original=table;b->bucket=bucket;
  for(avtab_ptr_t node=table->htable[bucket];node;node=node->next){
   if(o->count==o->actual_capacity)return -1;
   for(unsigned long i=0;i<o->count;i++)if(o->actual_visits[i].node_original==node)return -1;
   sl_selinux_rule_visit *v=&o->actual_visits[o->count++];v->table_original=table;v->node_original=node;v->bucket=bucket;v->conditional=conditional;
   unsigned specified=node->key.specified&~(AVTAB_ENABLED|AVTAB_ENABLED_OLD);
   if(specified!=AVTAB_ALLOWED&&specified!=AVTAB_AUDITALLOW&&specified!=AVTAB_AUDITDENY&&specified!=AVTAB_TRANSITION&&specified!=AVTAB_MEMBER&&specified!=AVTAB_CHANGE&&specified!=AVTAB_XPERMS_ALLOWED&&specified!=AVTAB_XPERMS_AUDITALLOW&&specified!=AVTAB_XPERMS_DONTAUDIT)return -1;
   if(!node->key.source_type||node->key.source_type>n||!node->key.target_type||node->key.target_type>n||!node->key.target_class||node->key.target_class>o->actual_policy->p_classes.nprim)return -1;
   unsigned transition=!!(node->datum.data&root->transition_permission),dynamic=!!(node->datum.data&root->dyntransition_permission);
   if(specified!=AVTAB_ALLOWED||node->key.target_class!=o->actual_class||(!transition&&!dynamic))continue;
   /* Scratch is a synchronous projection only; each bitmap lookup retains
    * original nodes/maps/results in distinct helper occurrences before reuse. */
   for(unsigned long i=0;i<n;i++){
    type_datum_t *t=o->actual_policy->type_val_to_struct[i];root->actual_matches[i]=root->actual_matches[n+i]=0;if(t->flavor==TYPE_ATTRIB)continue;
    int source=relation_bit(o,&o->actual_policy->type_attr_map[i],node->key.source_type-1);if(source<0)return -1;root->actual_matches[i]=source;
    int target=relation_bit(o,&o->actual_policy->type_attr_map[i],node->key.target_type-1);if(target<0)return -1;root->actual_matches[n+i]=target;
   }
   for(unsigned long source=0;source<n;source++)if(root->actual_matches[source])for(unsigned long target=0;target<n;target++)if(root->actual_matches[n+target]){
    sl_selinux_transition_edge *e=&root->actual_edges[source*n+target];v->matched=1;
    if(transition&&!e->transition){e->transition_original=node;e->transition=1;}
    if(dynamic&&!e->dyntransition){e->dyntransition_original=node;e->dyntransition=1;}
   }
  }
 }
 return o->count-begin==table->nel?0:-1;
}
int sl_selinux_interpret_permission_matrix(sl_selinux_transition_matrix *root,const char *class_name,const char *first,const char *second){
 if(!root||root->attempted)return -1;root->attempted=1;root->result=-1;sl_selinux_te_union *o=&root->custody;
 if(o->attempted||o->returned){root->returned=1;return -1;}o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_visits=o->visits;o->actual_capacity=o->capacity;o->actual_helpers=o->helpers;o->actual_helper_capacity=o->helper_capacity;o->actual_string_capacity=o->string_capacity;
 root->actual_edges=root->edges;root->actual_edge_capacity=root->edge_capacity;root->actual_matches=root->matches;root->actual_match_capacity=root->match_capacity;
 policydb_t *p=o->actual_policy;unsigned long n=p?p->p_types.nprim:0;
 if(!class_name||!first||!second||root->permission_names[0]||root->permission_names[1]||!relation_policy(p)||!p->type_attr_map||!p->class_val_to_struct||n>ULONG_MAX/n||n>ULONG_MAX/2||!root->actual_edges||root->actual_edge_capacity<n*n||!root->actual_matches||root->actual_match_capacity<2*n||root->types||root->transition_original||root->dyntransition_original||root->transition_permission||root->dyntransition_permission||!o->actual_visits||!o->actual_capacity||o->count||!o->actual_helpers||!o->actual_helper_capacity||!o->actual_string_capacity||o->helper_count)goto done;
 for(unsigned long i=0;i<root->actual_edge_capacity;i++){sl_selinux_transition_edge *e=&root->actual_edges[i];if(e->source_original||e->target_original||e->transition_original||e->dyntransition_original||e->transition||e->dyntransition||e->permissive||e->permissive_original)goto done;}
 for(unsigned long i=0;i<root->actual_match_capacity;i++)if(root->actual_matches[i])goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++){sl_selinux_rule_visit *v=&o->actual_visits[i];if(v->table_original||v->node_original||v->bucket||v->conditional||v->matched)goto done;}
 for(unsigned long i=0;i<o->actual_helper_capacity;i++){sl_selinux_helper_visit *v=&o->actual_helpers[i];if(v->kind||v->avtab_original||v->table_original||v->hash_original||v->name_original||v->key_original||v->name_slot_original||v->name_source_original||v->bucket||v->compared||v->name_read_extent||v->key_read_extent||v->name_terminated||v->key_terminated||v->filename_key_original||v->filename_datum_original||v->bitmap_original||v->bitmap_node_original||v->bit||v->startbit||v->highbit||v->map||v->matched)goto done;}
 for(unsigned long i=0;i<n;i++){type_datum_t *t=p->type_val_to_struct[i];if(!t||t->s.value!=i+1||(t->flavor!=TYPE_TYPE&&t->flavor!=TYPE_ATTRIB))goto done;}
 root->types=n;
 for(unsigned long from=0;from<n;from++)for(unsigned long to=0;to<n;to++){sl_selinux_transition_edge *e=&root->actual_edges[from*n+to];e->source_original=p->type_val_to_struct[from];e->target_original=p->type_val_to_struct[to];}
 root->permission_names[0]=first;root->permission_names[1]=second;
 void *found=NULL;o->actual_class_name=class_name;if(relation_lookup(o,p->p_classes.table,&o->actual_class_name,&found))goto done;o->class_original=found;
 if(!o->class_original||!o->class_original->s.value||o->class_original->s.value>p->p_classes.nprim||o->class_original->s.value>UINT16_MAX)goto done;
 o->actual_class=o->class_original->s.value;if(p->class_val_to_struct[o->actual_class-1]!=o->class_original)goto done;
 for(unsigned i=0;i<2;i++){
  if(relation_lookup(o,o->class_original->permissions.table,&root->permission_names[i],&found))goto done;
  if(!found&&o->class_original->comdatum){o->common_original=o->class_original->comdatum;if(relation_lookup(o,o->common_original->permissions.table,&root->permission_names[i],&found))goto done;}
  perm_datum_t *permission=found;if(!permission||!permission->s.value||permission->s.value>32)goto done;sepol_access_vector_t bit=UINT32_C(1)<<(permission->s.value-1);
  if(i){root->dyntransition_original=permission;root->dyntransition_permission=bit;}else{root->transition_original=permission;root->transition_permission=bit;}
 }
 if(root->transition_permission==root->dyntransition_permission)goto done;
 if(transition_table(root,&p->te_avtab,0)||transition_table(root,&p->te_cond_avtab,1))goto done;
 for(unsigned long source=0;source<n;source++){
  if(p->type_val_to_struct[source]->flavor==TYPE_ATTRIB)continue;
  int permissive=relation_bit(o,&p->permissive_map,(unsigned)source+1);if(permissive<0)goto done;
  for(unsigned long target=0;target<n;target++){
   sl_selinux_transition_edge *edge=&root->actual_edges[source*n+target];edge->permissive_original=&p->permissive_map;edge->permissive=permissive;
   if(permissive&&p->type_val_to_struct[target]->flavor!=TYPE_ATTRIB){edge->transition=1;edge->dyntransition=1;}
  }
 }
 root->result=o->result=0;
 done:o->returned=1;root->returned=1;return root->result;
}
int sl_selinux_interpret_transition_matrix(sl_selinux_transition_matrix *root){return sl_selinux_interpret_permission_matrix(root,sl_selinux_name_process,sl_selinux_name_transition,sl_selinux_name_dyntransition);}
static int exec_type_table(sl_selinux_exec_type *root,avtab_t *table,unsigned conditional){
 sl_selinux_te_union *o=&root->custody;policydb_t *p=o->actual_policy;unsigned long begin=o->count;
 if((table->nslot&&!table->htable)||(!table->nslot&&table->nel))return -1;
 for(unsigned long bucket=0;bucket<table->nslot;bucket++){
  sl_selinux_helper_visit *b=relation_helper(o,SL_SELINUX_AVTAB_BUCKET);if(!b)return -1;b->avtab_original=table;b->bucket=bucket;
  for(avtab_ptr_t node=table->htable[bucket];node;node=node->next){
   if(o->count==o->actual_capacity)return -1;for(unsigned long i=0;i<o->count;i++)if(o->actual_visits[i].node_original==node)return -1;
   sl_selinux_rule_visit *v=&o->actual_visits[o->count++];v->table_original=table;v->node_original=node;v->bucket=bucket;v->conditional=conditional;
   unsigned specified=node->key.specified&~(AVTAB_ENABLED|AVTAB_ENABLED_OLD);
   if(specified!=AVTAB_ALLOWED&&specified!=AVTAB_AUDITALLOW&&specified!=AVTAB_AUDITDENY&&specified!=AVTAB_TRANSITION&&specified!=AVTAB_MEMBER&&specified!=AVTAB_CHANGE&&specified!=AVTAB_XPERMS_ALLOWED&&specified!=AVTAB_XPERMS_AUDITALLOW&&specified!=AVTAB_XPERMS_DONTAUDIT)return -1;
   if(!node->key.source_type||node->key.source_type>p->p_types.nprim||!node->key.target_type||node->key.target_type>p->p_types.nprim||!node->key.target_class||node->key.target_class>p->p_classes.nprim)return -1;
   if(specified!=AVTAB_TRANSITION||node->key.target_class!=o->actual_class)continue;
   int source=relation_bit(o,&p->type_attr_map[o->actual_source-1],node->key.source_type-1);if(source<0)return -1;
   int target=relation_bit(o,&p->type_attr_map[o->actual_target-1],node->key.target_type-1);if(target<0)return -1;if(!source||!target)continue;
   unsigned destination=node->datum.data;if(!destination||destination>p->p_types.nprim)return -1;
   type_datum_t *type=p->type_val_to_struct[destination-1];if(!type||type->s.value!=destination||type->flavor!=TYPE_TYPE)return -1;
   v->matched=1;if(!root->matched)root->first_original=node;root->last_original=node;root->matched++;if(conditional)root->conditional_matched++;else root->permanent_matched++;if(destination!=o->actual_source)root->changed_type=1;
  }
 }
 return o->count-begin==table->nel?0:-1;
}
int sl_selinux_interpret_exec_type_stability(sl_selinux_exec_type *root){
 if(!root||root->attempted)return -1;root->attempted=1;root->result=-1;sl_selinux_te_union *o=&root->custody;
 if(o->attempted||o->returned){root->returned=1;return -1;}o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_source=o->source_type;o->actual_target=o->target_type;o->actual_visits=o->visits;o->actual_capacity=o->capacity;o->actual_helpers=o->helpers;o->actual_helper_capacity=o->helper_capacity;o->actual_string_capacity=o->string_capacity;
 policydb_t *p=o->actual_policy;
 if(!relation_policy(p)||!p->type_attr_map||!p->class_val_to_struct||!o->actual_source||o->actual_source>p->p_types.nprim||!o->actual_target||o->actual_target>p->p_types.nprim||!o->actual_visits||!o->actual_capacity||o->count||!o->actual_helpers||!o->actual_helper_capacity||!o->actual_string_capacity||o->helper_count||root->matched||root->first_original||root->last_original||root->changed_type||root->default_type||root->permanent_matched||root->conditional_matched)goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++){sl_selinux_rule_visit *v=&o->actual_visits[i];if(v->table_original||v->node_original||v->bucket||v->conditional||v->matched)goto done;}
 for(unsigned long i=0;i<o->actual_helper_capacity;i++){sl_selinux_helper_visit *v=&o->actual_helpers[i];if(v->kind||v->avtab_original||v->table_original||v->hash_original||v->name_original||v->key_original||v->name_slot_original||v->name_source_original||v->bucket||v->compared||v->name_read_extent||v->key_read_extent||v->name_terminated||v->key_terminated||v->filename_key_original||v->filename_datum_original||v->bitmap_original||v->bitmap_node_original||v->bit||v->startbit||v->highbit||v->map||v->matched)goto done;}
 root->source_original=p->type_val_to_struct[o->actual_source-1];root->file_original=p->type_val_to_struct[o->actual_target-1];
 if(!root->source_original||!root->file_original||root->source_original->s.value!=o->actual_source||root->file_original->s.value!=o->actual_target||root->source_original->flavor!=TYPE_TYPE||root->file_original->flavor!=TYPE_TYPE)goto done;
 void *found=NULL;o->actual_class_name=sl_selinux_name_process;if(relation_lookup(o,p->p_classes.table,&o->actual_class_name,&found))goto done;o->class_original=found;
 if(!o->class_original||!o->class_original->s.value||o->class_original->s.value>p->p_classes.nprim||o->class_original->s.value>UINT16_MAX)goto done;o->actual_class=o->class_original->s.value;
 if(p->class_val_to_struct[o->actual_class-1]!=o->class_original)goto done;
 root->default_type=(unsigned char)o->class_original->default_type;
 if(root->default_type&&root->default_type!=DEFAULT_SOURCE&&root->default_type!=DEFAULT_TARGET)goto done;
 if(exec_type_table(root,&p->te_avtab,0)||exec_type_table(root,&p->te_cond_avtab,1)||root->changed_type)goto done;
 if(!root->permanent_matched&&root->default_type==DEFAULT_TARGET&&o->actual_target!=o->actual_source){root->changed_type=1;goto done;}
 root->result=o->result=0;
 done:o->returned=1;root->returned=1;return root->result;
}

static int creation_add(sl_selinux_creation_types *root,unsigned type){
 policydb_t *p=root->custody.actual_policy;
 if(!type||type>p->p_types.nprim||type>root->actual_capacity)return -1;
 type_datum_t *referent=p->type_val_to_struct[type-1];
 if(!referent||referent->s.value!=type||referent->flavor!=TYPE_TYPE)return -1;
 if(!root->actual_types[type-1]){root->actual_types[type-1]=1;root->actual_referents[type-1]=referent;root->count++;}
 else if(root->actual_referents[type-1]!=referent)return -1;
 return 0;
}
static int creation_table(sl_selinux_creation_types *root,avtab_t *table,unsigned conditional){
 sl_selinux_te_union *o=&root->custody;
 if((table->nslot&&!table->htable)||(!table->nslot&&table->nel))return -1;
 unsigned long begin=o->count;
 for(unsigned long bucket=0;bucket<table->nslot;bucket++){
  sl_selinux_helper_visit *h=relation_helper(o,SL_SELINUX_AVTAB_BUCKET);if(!h)return -1;h->avtab_original=table;h->bucket=bucket;
  for(avtab_ptr_t node=table->htable[bucket];node;node=node->next){
   if(o->count==o->actual_capacity)return -1;
   for(unsigned long i=0;i<o->count;i++)if(o->actual_visits[i].node_original==node)return -1;
   sl_selinux_rule_visit *visit=&o->actual_visits[o->count++];visit->table_original=table;visit->node_original=node;visit->bucket=bucket;visit->conditional=conditional;
   if(!node->key.source_type||node->key.source_type>o->actual_policy->p_types.nprim||!node->key.target_type||node->key.target_type>o->actual_policy->p_types.nprim||!node->key.target_class||node->key.target_class>o->actual_policy->p_classes.nprim)return -1;
   unsigned specified=node->key.specified&~(AVTAB_ENABLED|AVTAB_ENABLED_OLD);
   if(specified!=AVTAB_ALLOWED&&specified!=AVTAB_AUDITALLOW&&specified!=AVTAB_AUDITDENY&&specified!=AVTAB_TRANSITION&&specified!=AVTAB_MEMBER&&specified!=AVTAB_CHANGE&&specified!=AVTAB_XPERMS_ALLOWED&&specified!=AVTAB_XPERMS_AUDITALLOW&&specified!=AVTAB_XPERMS_DONTAUDIT)return -1;
   if(specified==AVTAB_TRANSITION&&node->key.target_class==o->actual_class){
    int source=relation_bit(o,o->source_attributes,node->key.source_type-1),target=relation_bit(o,o->target_attributes,node->key.target_type-1);
    if(source<0||target<0)return -1;
    if(source&&target){visit->matched=1;if(creation_add(root,node->datum.data))return -1;}
   }
  }
 }
 return o->count-begin==table->nel?0:-1;
}
static int creation_filenames(sl_selinux_creation_types *root){
 sl_selinux_te_union *o=&root->custody;hashtab_t table=o->actual_policy->filename_trans;
 if(!table)return o->actual_policy->filename_trans_count?-1:0;
 if((table->size&&!table->htable)||(!table->size&&table->nel))return -1;
 unsigned long begin=o->helper_count,nodes=0;
 for(unsigned long bucket=0;bucket<table->size;bucket++){
  sl_selinux_helper_visit *h=relation_helper(o,SL_SELINUX_FILENAME_BUCKET);if(!h)return -1;h->table_original=table;h->bucket=bucket;
  for(hashtab_ptr_t node=table->htable[bucket];node;node=node->next){
   for(unsigned long i=begin;i<o->helper_count;i++)if(o->actual_helpers[i].kind==SL_SELINUX_FILENAME_NODE&&o->actual_helpers[i].hash_original==node)return -1;
   h=relation_helper(o,SL_SELINUX_FILENAME_NODE);if(!h)return -1;h->table_original=table;h->hash_original=node;h->bucket=bucket;nodes++;
   filename_trans_key_t *key=(filename_trans_key_t*)node->key;h->filename_key_original=key;
   if(!key||!key->name||!key->ttype||key->ttype>o->actual_policy->p_types.nprim||!key->tclass||key->tclass>o->actual_policy->p_classes.nprim||!node->datum)return -1;
   h->key_original=key->name;unsigned ended=0;
   for(unsigned long i=0;i<o->actual_string_capacity;i++){h->compared=h->key_read_extent=i+1;if(!key->name[i]){h->key_terminated=ended=1;break;}}
   if(!ended)return -1;
   unsigned matching=key->ttype==o->actual_target&&key->tclass==o->actual_class;h->matched=matching;
   for(filename_trans_datum_t *datum=node->datum;datum;datum=datum->next){
    for(unsigned long i=begin;i<o->helper_count;i++)if(o->actual_helpers[i].kind==SL_SELINUX_FILENAME_DATUM&&o->actual_helpers[i].filename_datum_original==datum)return -1;
    h=relation_helper(o,SL_SELINUX_FILENAME_DATUM);if(!h)return -1;h->table_original=table;h->hash_original=node;h->filename_key_original=key;h->filename_datum_original=datum;
    if(!datum->otype||datum->otype>o->actual_policy->p_types.nprim)return -1;
    /* Visit the complete original bitmap even for nonmatching keys. */
    int source=relation_bit(o,&datum->stypes,o->actual_source-1);if(source<0)return -1;
    if(matching&&source){h->matched=1;if(creation_add(root,datum->otype))return -1;}
   }
  }
 }
 return nodes==table->nel?0:-1;
}
int sl_selinux_interpret_creation_types(sl_selinux_creation_types *root){
 if(!root||root->attempted)return -1;root->attempted=1;root->result=-1;
 sl_selinux_te_union *o=&root->custody;if(o->attempted||o->returned)goto done;o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_source=o->source_type;o->actual_target=o->target_type;o->actual_visits=o->visits;o->actual_capacity=o->capacity;o->actual_helpers=o->helpers;o->actual_helper_capacity=o->helper_capacity;o->actual_string_capacity=o->string_capacity;
 root->actual_types=root->types;root->actual_referents=root->referents;root->actual_capacity=root->capacity;
 policydb_t *p=o->actual_policy;
 if(!relation_policy(p)||!p->type_attr_map||!p->class_val_to_struct||!root->actual_types||!root->actual_referents||root->actual_capacity!=p->p_types.nprim||root->count||!o->actual_source||o->actual_source>p->p_types.nprim||!o->actual_target||o->actual_target>p->p_types.nprim||!o->actual_visits||!o->actual_capacity||o->count||!o->actual_helpers||!o->actual_helper_capacity||!o->actual_string_capacity||o->helper_count)goto done;
 for(unsigned long i=0;i<root->actual_capacity;i++)if(root->actual_types[i]||root->actual_referents[i])goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++){sl_selinux_rule_visit *v=&o->actual_visits[i];if(v->table_original||v->node_original||v->bucket||v->conditional||v->matched)goto done;}
 for(unsigned long i=0;i<o->actual_helper_capacity;i++){sl_selinux_helper_visit *v=&o->actual_helpers[i];if(v->kind||v->avtab_original||v->table_original||v->hash_original||v->name_original||v->key_original||v->name_slot_original||v->name_source_original||v->bucket||v->compared||v->name_read_extent||v->key_read_extent||v->name_terminated||v->key_terminated||v->filename_key_original||v->filename_datum_original||v->bitmap_original||v->bitmap_node_original||v->bit||v->startbit||v->highbit||v->map||v->matched)goto done;}
 void *found=NULL;o->actual_class_name=sl_selinux_name_file;if(relation_lookup(o,p->p_classes.table,&o->actual_class_name,&found))goto done;o->class_original=found;
 if(!o->class_original||!o->class_original->s.value||o->class_original->s.value>p->p_classes.nprim||o->class_original->s.value>UINT16_MAX)goto done;o->actual_class=o->class_original->s.value;
 if(p->class_val_to_struct[o->actual_class-1]!=o->class_original)goto done;
 root->default_type=(unsigned char)o->class_original->default_type;if(root->default_type&&root->default_type!=DEFAULT_SOURCE&&root->default_type!=DEFAULT_TARGET)goto done;
 if(creation_add(root,o->actual_source)||creation_add(root,o->actual_target))goto done;
 if(root->explicit_type){if(root->explicit_type>p->p_types.nprim||root->explicit_original!=p->type_val_to_struct[root->explicit_type-1]||creation_add(root,root->explicit_type))goto done;}
 else if(root->explicit_original)goto done;
 root->owner_original=p->type_val_to_struct[o->actual_source-1];root->directory_original=p->type_val_to_struct[o->actual_target-1];o->source_attributes=&p->type_attr_map[o->actual_source-1];o->target_attributes=&p->type_attr_map[o->actual_target-1];
 if(creation_table(root,&p->te_avtab,0)||creation_table(root,&p->te_cond_avtab,1)||creation_filenames(root))goto done;
 root->result=o->result=0;
 done:o->returned=1;root->returned=1;return root->result;
 /* This type overapprox does not establish filesystem/newSID, kernel-policy
  * mapping or enforced permission exclusion and supplies no archive grant. */
}

/* Constants below are the retained pinned parser ABI, not native authority. */
#include <sepol/policydb/polcaps.h>
#include <sepol/policydb/services.h>
#include "../../third_party/libsepol/src/flask.h"
static int genfs_name(sl_selinux_te_union *o,sl_selinux_helper_visit *v,const char *name){
 if(!name)return -1;v->name_original=name;
 unsigned cg=1,cg2=1;static const char first[]="cgroup",second[]="cgroup2";
 for(unsigned long i=0;i<o->actual_string_capacity;i++){
  unsigned char byte=(unsigned char)name[i];v->compared=v->name_read_extent=i+1;if(!byte)v->name_terminated=1;
  if(i>=sizeof(first)||byte!=(unsigned char)first[i])cg=0;
  if(i>=sizeof(second)||byte!=(unsigned char)second[i])cg2=0;
  if(!byte)return cg?1:cg2?2:0;
 }
 return -1;
}
static int genfs_path(sl_selinux_te_union *o,sl_selinux_helper_visit *v,const char *name){
 if(!name)return -1;v->key_original=name;
 v->key_read_extent=1;unsigned char first=(unsigned char)name[0];if(!first)v->key_terminated=1;
 if(first!='/')return -1;
 for(unsigned long i=0;i<o->actual_string_capacity;i++){v->compared=v->key_read_extent=i+1;if(!name[i]){v->key_terminated=1;return 0;}}
 return -1;
}
static int genfs_add_set(sl_selinux_creation_types *t,context_struct_t *context){
 policydb_t *p=t->custody.actual_policy;
 if(!context||!context->type||context->type>p->p_types.nprim)return -1;
 type_datum_t *type=p->type_val_to_struct[context->type-1];
 if(!type||type->s.value!=context->type||type->flavor!=TYPE_TYPE)return -1;
 unsigned long index=context->type-1;
 if(index>=t->actual_capacity)return -1;
 if(t->actual_types[index])return t->actual_types[index]==1&&t->actual_referents[index]==type?0:-1;
 if(t->count==t->actual_capacity||t->actual_referents[index])return -1;
 t->actual_types[index]=1;t->actual_referents[index]=type;t->count++;
 if(!t->custody.actual_target)t->custody.actual_target=context->type;return 0;
}
static sl_selinux_helper_visit *genfs_context_visit(sl_selinux_te_union *o,ocontext_t *c,unsigned list){
 for(unsigned long i=0;i<o->helper_count;i++)if(o->actual_helpers[i].kind==SL_SELINUX_OCONTEXT_NODE&&o->actual_helpers[i].ocontext_original==c)return NULL;
 sl_selinux_helper_visit *v=relation_helper(o,SL_SELINUX_OCONTEXT_NODE);
 if(v){v->ocontext_original=c;v->context_list=list;}return v;
}
int sl_selinux_interpret_cgroup_genfs_types(sl_selinux_genfs_types *root){
 if(!root||root->attempted)return -1;root->attempted=1;root->result=-1;
 sl_selinux_creation_types *t=&root->targets,*fs=&root->filesystem;sl_selinux_te_union *o=&t->custody;unsigned entered=0;
 if(t->attempted||o->attempted||fs->attempted||fs->custody.attempted||root->class_names[0]||root->class_names[1])goto done;entered=1;t->attempted=o->attempted=fs->attempted=fs->custody.attempted=1;t->result=o->result=fs->result=fs->custody.result=-1;
 o->actual_policy=o->policy_original;o->actual_helpers=o->helpers;o->actual_helper_capacity=o->helper_capacity;o->actual_string_capacity=o->string_capacity;
 t->actual_types=t->types;t->actual_referents=t->referents;t->actual_capacity=t->capacity;
 fs->actual_types=fs->types;fs->actual_referents=fs->referents;fs->actual_capacity=fs->capacity;fs->custody.actual_policy=o->actual_policy;
 root->actual_observed=root->observed;root->actual_observed_count=root->observed_count;
 policydb_t *p=o->actual_policy;
 if(!relation_policy(p)||!p->class_val_to_struct||!o->actual_helpers||!o->actual_helper_capacity||!o->actual_string_capacity||o->helper_count||
    !t->actual_types||!t->actual_referents||t->actual_capacity!=p->p_types.nprim||t->count||
    !fs->actual_types||!fs->actual_referents||fs->actual_capacity!=p->p_types.nprim||fs->count||
    (root->actual_observed_count&&!root->actual_observed)||root->checked_observed||root->matched_filesystems||root->matched_rules||
    root->unlabeled_original||root->filesystem_original)goto done;
 for(unsigned long i=0;i<t->actual_capacity;i++)if(t->actual_types[i]||t->actual_referents[i]||fs->actual_types[i]||fs->actual_referents[i])goto done;
 /* Existing candidate branch only. No runtime policy setting is changed. */
 if(relation_bit(o,&p->policycaps,POLICYDB_CAP_CGROUPSECLABEL)!=0)goto done;
 root->class_names[0]=sl_selinux_name_file;root->class_names[1]=sl_selinux_name_dir;
 void *datum=NULL;if(relation_lookup(o,p->p_classes.table,&root->class_names[0],&datum)||!datum)goto done;
 root->file_class=((class_datum_t *)datum)->s.value;
 if(!root->file_class||root->file_class>p->p_classes.nprim||p->class_val_to_struct[root->file_class-1]!=datum)goto done;
 datum=NULL;if(relation_lookup(o,p->p_classes.table,&root->class_names[1],&datum)||!datum)goto done;
 root->directory_class=((class_datum_t *)datum)->s.value;
 if(!root->directory_class||root->directory_class>p->p_classes.nprim||p->class_val_to_struct[root->directory_class-1]!=datum)goto done;
 for(ocontext_t *c=p->ocontexts[OCON_ISID];c;c=c->next){
  if(!genfs_context_visit(o,c,OCON_ISID))goto done;
  if(c->sid[0]==SECINITSID_UNLABELED){if(root->unlabeled_original||genfs_add_set(t,&c->context[0])||genfs_add_set(fs,&c->context[0]))goto done;root->unlabeled_original=c;}
  if(c->sid[0]==SECINITSID_FS){if(root->filesystem_original||genfs_add_set(t,&c->context[0])||genfs_add_set(fs,&c->context[0]))goto done;root->filesystem_original=c;}
 }
 if(!root->unlabeled_original||!root->filesystem_original)goto done;
 for(ocontext_t *c=p->ocontexts[OCON_FSUSE];c;c=c->next){
  sl_selinux_helper_visit *v=genfs_context_visit(o,c,OCON_FSUSE);if(!v)goto done;
  int match=genfs_name(o,v,c->u.name);if(match<0)goto done;
  if(match){if(c->v.behavior!=SECURITY_FS_USE_GENFS||genfs_add_set(t,&c->context[0])||genfs_add_set(fs,&c->context[0]))goto done;v->matched=1;}
 }
 for(genfs_t *g=p->genfs;g;g=g->next){
  for(unsigned long i=0;i<o->helper_count;i++)if(o->actual_helpers[i].kind==SL_SELINUX_GENFS_NODE&&o->actual_helpers[i].genfs_original==g)goto done;
  sl_selinux_helper_visit *v=relation_helper(o,SL_SELINUX_GENFS_NODE);if(!v)goto done;v->genfs_original=g;
  int match=genfs_name(o,v,g->fstype);if(match<0)goto done;if(match){if(root->matched_filesystems==UINT_MAX)goto done;v->matched=1;root->matched_filesystems++;}
  for(ocontext_t *c=g->head;c;c=c->next){
   sl_selinux_helper_visit *cv=genfs_context_visit(o,c,OCON_NUM);if(!cv)goto done;
   if(genfs_path(o,cv,c->u.name)||c->v.sclass>p->p_classes.nprim)goto done;
   if(match&&(!c->v.sclass||c->v.sclass==root->file_class||c->v.sclass==root->directory_class)){
    if(root->matched_rules==UINT_MAX||genfs_add_set(t,&c->context[0]))goto done;
    if((!c->v.sclass||c->v.sclass==root->directory_class)&&genfs_add_set(fs,&c->context[0]))goto done;cv->matched=1;root->matched_rules++;
   }
  }
 }
 if(!root->matched_filesystems||!root->matched_rules)goto done;
 for(unsigned long i=0;i<root->actual_observed_count;i++){
  if(genfs_add_set(t,root->actual_observed[i]))goto done;root->checked_observed++;
 }
 root->result=0;
 done:root->returned=1;
 /* Only owners entered by this operation receive completion here. */
 if(entered){t->result=fs->result=root->result;t->returned=fs->returned=1;o->result=fs->custody.result=root->result;o->returned=fs->custody.returned=1;}
 return root->result;
}
