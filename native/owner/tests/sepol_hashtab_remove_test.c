/* UNRUN: actual owned pinned hash removal, not disposal/native proof. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/hashtab.h>
static union {max_align_t align;unsigned char bytes[4096];} backing[3];
static sl_sepol_memory_attempt rows[3][16];
static unsigned hash(hashtab_t h,const_hashtab_key_t k){(void)h;(void)k;return 0;}
static int compare(hashtab_t h,const_hashtab_key_t a,const_hashtab_key_t b){(void)h;return strcmp(a,b);}
int main(void){
 sl_sepol_memory_owner owners[3]={{0}};hashtab_t tables[3];hashtab_ptr_t originals[3];char key[]="original";
 for(unsigned i=0;i<3;i++){
  sl_sepol_memory_owner *o=&owners[i];o->bytes=backing[i].bytes;o->capacity=sizeof backing[i].bytes;o->attempts=rows[i];o->attempt_capacity=i==1?6:i==2?7:16;atomic_init(&o->claimed,0);assert(!sl_sepol_memory_begin(o));
  tables[i]=hashtab_create(hash,compare,1);assert(tables[i]&&!hashtab_insert(tables[i],key,key));originals[i]=tables[i]->htable[0];assert(o->attempt_count==6);
  int result=hashtab_remove(tables[i],key,NULL,NULL);
  if(!i){assert(!result&&!tables[i]->htable[0]&&!tables[i]->nel&&o->attempt_count==9);assert(rows[i][6].field_original==&tables[i]->htable[0]&&rows[i][6].old_pointer==originals[i]&&rows[i][6].returned);assert(rows[i][8].field_original==&tables[i]->nel&&rows[i][8].old_scalar==1&&rows[i][8].new_scalar==0&&rows[i][8].returned);assert(!sl_sepol_memory_finish(o));}
  else{assert(result<0&&tables[i]->nel==1&&o->first_failure==&o->exhaustion);if(i==1)assert(tables[i]->htable[0]==originals[i]&&o->exhaustion.operation==SL_SEPOL_POINTER_MUTATION&&o->exhaustion.old_pointer==originals[i]);else assert(!tables[i]->htable[0]&&rows[i][6].old_pointer==originals[i]&&rows[i][6].returned&&o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->exhaustion.input==originals[i]);assert(sl_sepol_memory_finish(o)!=0);}
  assert(originals[i]->key==key&&originals[i]->datum==key);
 }
 assert(tables[1]->htable[0]==originals[1]&&!tables[2]->htable[0]&&tables[1]->nel==1&&tables[2]->nel==1);return 0;
}