/* UNRUN: actual owned bitmap/policy failure propagation, not disposal proof. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/policydb.h>
static union {max_align_t align;unsigned char bytes[4096];} backing[4];
static sl_sepol_memory_attempt attempts[4][14];
static unsigned hash(hashtab_t h,const_hashtab_key_t k){(void)h;(void)k;return 0;}
static int compare(hashtab_t h,const_hashtab_key_t a,const_hashtab_key_t b){(void)h;return strcmp(a,b);}
int main(void){
 sl_sepol_memory_owner owners[4]={{0}};policydb_t policies[4]={{0}};
 for(unsigned i=0;i<4;i++){
  sl_sepol_memory_owner *o=&owners[i];o->bytes=backing[i].bytes;o->capacity=sizeof backing[i].bytes;o->attempts=attempts[i];o->attempt_capacity=i==3?14:i==2?8:2;atomic_init(&o->claimed,0);assert(!sl_sepol_memory_begin(o));
  if(i==3){
   hashtab_t h=hashtab_create(hash,compare,1);char *key=sl_sepol_source_calloc(2,1,__FILE__,__LINE__);common_datum_t *datum=sl_sepol_source_calloc(1,sizeof *datum,__FILE__,__LINE__);assert(h&&key&&datum);key[0]='x';assert(!hashtab_insert(h,key,datum));hashtab_ptr_t node=h->htable[0];assert(o->attempt_count==8);
   policies[i].sl_source_owner=o;policies[i].p_commons.table=h;policydb_destroy(&policies[i]);assert(o->attempt_count==14&&o->first_failure==&o->exhaustion&&o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->exhaustion.input==key);assert(h->htable[0]==node&&node->key==key&&node->datum==datum&&key[0]=='x'&&!datum->permissions.table);assert(!attempts[i][2].logically_released&&!attempts[i][3].logically_released);assert(sl_sepol_memory_finish(o)!=0);continue;
  }
  ebitmap_node_t *first=sl_sepol_source_calloc(1,sizeof *first,__FILE__,__LINE__),*second=sl_sepol_source_calloc(1,sizeof *second,__FILE__,__LINE__);assert(first&&second);first->map=1;second->map=2;
  if(i==2){policies[i].sl_source_owner=o;policies[i].sym_val_to_name[0]=(char**)first;policies[i].class_val_to_struct=(class_datum_t**)second;policydb_destroy(&policies[i]);assert(o->first_failure==&o->exhaustion&&o->exhaustion.input==first&&o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->attempt_count==8);assert(policies[i].sym_val_to_name[0]==(char**)first&&policies[i].class_val_to_struct==(class_datum_t**)second&&first->map==1&&second->map==2);assert(sl_sepol_memory_finish(o)!=0);continue;}
  policies[i].sl_source_owner=o;policies[i].policycaps.node=first;policies[i].policycaps.highbit=MAPSIZE;
  if(!i){first->next=second;ebitmap_destroy(&policies[i].policycaps);assert(first->next==second);}
  else{policies[i].permissive_map.node=second;policies[i].permissive_map.highbit=MAPSIZE;policydb_destroy(&policies[i]);assert(policies[i].permissive_map.node==second&&policies[i].permissive_map.highbit==MAPSIZE);}
  assert(o->first_failure==&o->exhaustion&&o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->exhaustion.input==first&&o->attempt_count==2);
  assert(policies[i].policycaps.node==first&&policies[i].policycaps.highbit==MAPSIZE&&first->map==1&&second->map==2);assert(sl_sepol_memory_finish(o)!=0);
 }
 return 0;
}