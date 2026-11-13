/* UNRUN: actual static parser caller. Link pinned dependencies without a
 * separate policydb.o; this translation unit contains the actual source body.
 * No native/profile or cross-episode bitmap ownership qualification. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include "../../../third_party/libsepol/src/policydb.c"
static union {max_align_t alignment;unsigned char bytes[4096];} backing[4];
static sl_sepol_memory_attempt rows[4][64];
static sl_sepol_memory_owner owners[4];
static ebitmap_node_t source_nodes[4];
static filename_trans_datum_t datums[4],duplicates[4];
int main(void){
 size_t complete_count=0;
 for(unsigned i=0;i<4;i++){
  sl_sepol_memory_owner *owner=&owners[i];
  owner->bytes=backing[i].bytes;owner->capacity=sizeof backing[i].bytes;
  owner->attempts=rows[i];owner->attempt_capacity=(i==1||i==3)?complete_count-6:i==2?complete_count-3:64;
  atomic_init(&owner->claimed,0);assert(!sl_sepol_memory_begin(owner));
  source_nodes[i].map=1;datums[i].stypes.node=&source_nodes[i];datums[i].stypes.highbit=MAPSIZE;datums[i].otype=1;
  if(i==3){duplicates[i].otype=1;datums[i].next=&duplicates[i];}
  int result=filename_trans_check_datum(&datums[i]);
  assert(source_nodes[i].map==1&&datums[i].stypes.node==&source_nodes[i]);
  if(!i){
   assert(!result&&!owner->first_failure);complete_count=owner->attempt_count;assert(complete_count>=6);
   assert(rows[i][complete_count-6].operation==SL_SEPOL_LOGICAL_FREE&&rows[i][complete_count-6].returned);
   assert(rows[i][complete_count-3].operation==SL_SEPOL_LOGICAL_FREE&&rows[i][complete_count-3].returned);
  }else{
   assert(result==-1&&owner->first_failure==&owner->exhaustion);
   assert(owner->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&!owner->exhaustion.returned);
   assert(owner->attempt_count==owner->actual_attempt_capacity);
   assert(owner->exhaustion.input);
   if(i==1)assert(!rows[i][0].logically_released&&!rows[i][1].logically_released);
   if(i==2)assert(!rows[i][0].logically_released&&rows[i][1].logically_released);
  }
  assert(sl_sepol_memory_finish(owner)==(i? -1:0));
 }
 assert(owners[1].first_failure&&owners[2].first_failure&&owners[3].first_failure&&!owners[0].first_failure);
 return 0;
}
