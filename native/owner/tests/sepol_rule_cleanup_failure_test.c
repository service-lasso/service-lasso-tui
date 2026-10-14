/* UNRUN actual owned rule/list failure cut; no native/disposal claim. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/policydb.h>
static union {max_align_t align;unsigned char bytes[4096];} backing;
static sl_sepol_memory_attempt rows[3];
int main(void){
 sl_sepol_memory_owner owner={0};owner.bytes=backing.bytes;owner.capacity=sizeof backing.bytes;owner.attempts=rows;owner.attempt_capacity=3;atomic_init(&owner.claimed,0);assert(!sl_sepol_memory_begin(&owner));
 avrule_t *rule=sl_sepol_source_calloc(1,sizeof *rule,__FILE__,__LINE__);ebitmap_node_t *first=sl_sepol_source_calloc(1,sizeof *first,__FILE__,__LINE__),*second=sl_sepol_source_calloc(1,sizeof *second,__FILE__,__LINE__);assert(rule&&first&&second);rule->stypes.types.node=first;rule->stypes.types.highbit=MAPSIZE;rule->ttypes.types.node=second;rule->ttypes.types.highbit=MAPSIZE;char source[]="retained-source";rule->source_filename=source;
 avrule_list_destroy(rule);assert(owner.first_failure==&owner.exhaustion&&owner.exhaustion.input==first&&owner.exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&owner.attempt_count==3);assert(rule->stypes.types.node==first&&rule->ttypes.types.node==second&&rule->source_filename==source&&!rows[0].logically_released&&!rows[1].logically_released&&!rows[2].logically_released);assert(sl_sepol_memory_finish(&owner)!=0);return 0;
}