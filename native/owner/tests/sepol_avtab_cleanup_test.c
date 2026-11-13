#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <sepol/policydb/avtab.h>
#include "../sepol_source_owner.h"
/* UNRUN. Link the pinned owned library with the selected redirect/diagnostic
 * route; this fixture is not permission to execute any parser or library. */
static union {max_align_t alignment;unsigned char bytes[8192];} arenas[4];
static sl_sepol_memory_attempt attempts[4][32];
static void begin(sl_sepol_memory_owner *owner,size_t capacity,unsigned slot){
 memset(owner,0,sizeof *owner);memset(attempts[slot],0,sizeof attempts[slot]);memset(&arenas[slot],0,sizeof arenas[slot]);
 owner->bytes=arenas[slot].bytes;owner->capacity=sizeof arenas[slot].bytes;owner->attempts=attempts[slot];owner->attempt_capacity=capacity;
 atomic_init(&owner->claimed,0);assert(sl_sepol_memory_begin(owner)==0);
}
int main(void){
 sl_sepol_memory_owner owner,owner2,owner3,owner4;avtab_t table,table2,table3;begin(&owner,32,0);memset(&table,0,sizeof table);assert(!avtab_init(&table)&&table.sl_source_owner==&owner);
 table.htable=sl_sepol_source_calloc(1,sizeof *table.htable,__FILE__,__LINE__);
 avtab_ptr_t first=sl_sepol_source_calloc(1,sizeof *first,__FILE__,__LINE__);
 avtab_ptr_t second=sl_sepol_source_calloc(1,sizeof *second,__FILE__,__LINE__);
 assert(table.htable&&first&&second);first->sl_source_owner=second->sl_source_owner=&owner;first->next=second;table.htable[0]=first;table.nslot=1;table.nel=2;
 void *original_buckets=table.htable;avtab_destroy(&table);
 assert(!table.htable&&!table.nslot&&!table.mask&&table.nel==2);
 unsigned root=0,bucket=0;
 for(size_t i=0;i<owner.attempt_count;i++){
  sl_sepol_memory_attempt *a=&attempts[0][i];
  if(a->operation==SL_SEPOL_POINTER_MUTATION&&a->field_original==&table.htable){assert(a->old_pointer==original_buckets&&a->returned);root++;}
  if(a->operation==SL_SEPOL_POINTER_MUTATION&&a->field_original==original_buckets){assert(a->old_pointer==first&&a->returned);bucket++;}
 }
 assert(root==1&&bucket==1&&first->next==second);assert(sl_sepol_memory_finish(&owner)==0);
 begin(&owner2,3,1);memset(&table2,0,sizeof table2);assert(!avtab_init(&table2)&&table2.sl_source_owner==&owner2);
 table2.htable=sl_sepol_source_calloc(1,sizeof *table2.htable,__FILE__,__LINE__);
 first=sl_sepol_source_calloc(1,sizeof *first,__FILE__,__LINE__);second=sl_sepol_source_calloc(1,sizeof *second,__FILE__,__LINE__);
 first->sl_source_owner=second->sl_source_owner=&owner2;first->next=second;table2.htable[0]=first;table2.nslot=1;original_buckets=table2.htable;
 avtab_destroy(&table2);
 assert(owner2.first_failure&&table2.htable==original_buckets&&table2.htable[0]==first&&table2.nslot==1&&first->next==second);
 assert(sl_sepol_memory_finish(&owner2)!=0);
 begin(&owner3,32,2);memset(&table3,0,sizeof table3);assert(!avtab_init(&table3)&&!avtab_alloc(&table3,1));avtab_key_t key={.source_type=1,.target_type=1,.target_class=1,.specified=AVTAB_ALLOWED};avtab_datum_t datum={.data=7};assert(!avtab_insert(&table3,&key,&datum));avtab_ptr_t healthy=avtab_search_node(&table3,&key);assert(healthy&&healthy->sl_source_owner==&owner3);assert(!sl_sepol_memory_finish(&owner3));
 begin(&owner4,32,3);size_t prior=owner4.attempt_count;sl_sepol_memory_attempt *failure=owner2.first_failure;
 assert(!avtab_search(&table2,&key)&&!avtab_search_node(&table2,&key)&&!avtab_search_node_next(first,AVTAB_ALLOWED));assert(avtab_insert(&table2,&key,&datum)!=0);avtab_destroy(&table2);
 assert(table2.sl_source_owner==&owner2&&table2.htable==original_buckets&&table2.htable[0]==first&&owner2.first_failure==failure&&owner4.attempt_count==prior&&!owner4.failed);
 assert(avtab_search(&table3,&key)==&healthy->datum&&avtab_insert(&table3,&key,&datum)!=0);avtab_destroy(&table3);assert(table3.htable&&table3.sl_source_owner==&owner3&&owner4.attempt_count==prior);assert(!sl_sepol_memory_finish(&owner4));return 0;
}