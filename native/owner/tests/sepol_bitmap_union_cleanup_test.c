/* UNRUN linked pinned bitmap caller; active-episode cleanup only, not complete
 * cross-episode bitmap ownership or parser/native qualification. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include <errno.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/ebitmap.h>
static union {max_align_t alignment;unsigned char bytes[4096];} backing[4];
static sl_sepol_memory_attempt rows[4][16];
int main(void){
 static sl_sepol_memory_owner owners[4];static ebitmap_t destinations[4],inputs[4];
 const size_t capacities[4]={3,6,7,16};
 for(unsigned i=0;i<4;i++){
  sl_sepol_memory_owner *owner=&owners[i];owner->bytes=backing[i].bytes;owner->capacity=sizeof backing[i].bytes;owner->attempts=rows[i];owner->attempt_capacity=capacities[i];atomic_init(&owner->claimed,0);assert(sl_sepol_memory_begin(owner)==0);
  ebitmap_t *dst=&destinations[i],*input=&inputs[i];
  dst->node=sl_sepol_source_calloc(1,sizeof *dst->node,__FILE__,__LINE__);input->node=sl_sepol_source_calloc(1,sizeof *input->node,__FILE__,__LINE__);assert(dst->node&&input->node);
  dst->node->map=1;input->node->map=2;dst->highbit=input->highbit=MAPSIZE;ebitmap_node_t *old=dst->node;
  int result=ebitmap_union(dst,input);assert(result==(i==3?0:-ENOMEM));
  assert(old->map==1&&input->node->map==2&&rows[i][2].result);
  ebitmap_node_t *replacement=rows[i][2].result;assert(replacement->map==3);
  if(i==0)assert(dst->node==old&&dst->highbit==MAPSIZE&&owner->first_failure&&!rows[i][2].logically_released);
  if(i==1)assert(!dst->node&&!dst->highbit&&owner->first_failure&&replacement->map==3);
  if(i==2)assert(dst->node==replacement&&!dst->highbit&&owner->first_failure&&rows[i][6].operation==SL_SEPOL_POINTER_MUTATION&&rows[i][6].new_pointer==replacement&&rows[i][6].returned);
  if(i==3)assert(dst->node==replacement&&dst->highbit==MAPSIZE&&!owner->first_failure&&rows[i][7].operation==SL_SEPOL_SCALAR_MUTATION&&rows[i][7].returned);
  assert(sl_sepol_memory_finish(owner)==(i==3?0:-1));
 }
 assert(destinations[0].node!=destinations[2].node&&owners[0].first_failure&&owners[2].first_failure&&!owners[3].first_failure);
 return 0;
}
