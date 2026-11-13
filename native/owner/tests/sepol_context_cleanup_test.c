/* UNRUN: actual pinned owned context/bitmap cleanup source, not parser/native proof. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/context.h>
static union {max_align_t alignment;unsigned char bytes[8192];} backing[3];
static sl_sepol_memory_attempt rows[3][32];
static context_struct_t *prepare(sl_sepol_memory_owner *owner,unsigned slot,size_t capacity){
 memset(owner,0,sizeof *owner);memset(&backing[slot],0,sizeof backing[slot]);memset(rows[slot],0,sizeof rows[slot]);
 owner->bytes=backing[slot].bytes;owner->capacity=sizeof backing[slot].bytes;owner->attempts=rows[slot];owner->attempt_capacity=capacity;atomic_init(&owner->claimed,0);assert(sl_sepol_memory_begin(owner)==0);
 context_struct_t *c=sl_sepol_source_calloc(1,sizeof *c,__FILE__,__LINE__);assert(c);
 c->user=7;c->role=8;c->type=9;
 for(unsigned i=0;i<2;i++){c->range.level[i].sens=10+i;c->range.level[i].cat.node=sl_sepol_source_calloc(1,sizeof(ebitmap_node_t),__FILE__,__LINE__);assert(c->range.level[i].cat.node);c->range.level[i].cat.highbit=MAPSIZE;c->range.level[i].cat.node->map=1;}
 return c;
}
int main(void){
 sl_sepol_memory_owner owners[3];sl_sepol_memory_owner *current=&owners[0];context_struct_t *c=prepare(current,0,32);ebitmap_node_t *first=c->range.level[0].cat.node,*second=c->range.level[1].cat.node;
 context_destroy(c);assert(!c->user&&!c->role&&!c->type&&!c->range.level[0].sens&&!c->range.level[1].sens&&!c->range.level[0].cat.node&&!c->range.level[1].cat.node);
 assert(first->map==1&&second->map==1);unsigned retained=0;for(size_t i=0;i<current->attempt_count;i++)if(rows[0][i].operation==SL_SEPOL_POINTER_MUTATION&&(rows[0][i].old_pointer==first||rows[0][i].old_pointer==second))retained++;
 assert(retained==2&&sl_sepol_memory_finish(current)==0);
 current=&owners[1];c=prepare(current,1,6);first=c->range.level[0].cat.node;second=c->range.level[1].cat.node;context_destroy(c);
 assert(current->first_failure&&!c->user&&!c->role&&!c->type&&c->range.level[0].cat.node==first&&c->range.level[1].cat.node==second&&c->range.level[0].sens==10&&c->range.level[1].sens==11);
 assert(sl_sepol_memory_finish(current)!=0);
 current=&owners[2];c=prepare(current,2,10);second=c->range.level[1].cat.node;context_destroy(c);
 assert(current->first_failure&&!c->range.level[0].cat.node&&!c->range.level[0].sens&&c->range.level[1].cat.node==second&&c->range.level[1].sens==11&&c->range.level[1].cat.highbit==MAPSIZE);
 assert(sl_sepol_memory_finish(current)!=0);return 0;
}