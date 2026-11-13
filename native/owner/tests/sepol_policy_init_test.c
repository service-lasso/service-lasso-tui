/* Source-only regression: UNRUN. Link against the selected owned allocator
 * redirects for every reachable libsepol translation unit, not host libsepol.
 * Arena sizes are regression inputs, never product fit/admission defaults. */
#include "../sepol_source_owner.h"
#include <sepol/policydb/policydb.h>
#include <assert.h>
#include <stdalign.h>
#include <string.h>
enum { ARENA_BYTES=131072,CAPACITY_STEP=127,CASES=(ARENA_BYTES-1)/CAPACITY_STEP+1,ATTEMPTS=4096 };
static alignas(max_align_t) unsigned char bytes[CASES][ARENA_BYTES];
static sl_sepol_memory_attempt attempts[CASES][ATTEMPTS];
static sl_sepol_memory_owner owners[CASES];static policydb_t policies[CASES];
int main(void){
 unsigned failed=0;size_t slot=0;
 /* Every sweep input retains distinct source-fresh roots/backing, including
  * failed initializers. No earlier owner/partial policy is memset-reused. */
 for(size_t capacity=1;capacity<=ARENA_BYTES;capacity+=CAPACITY_STEP,slot++){
  sl_sepol_memory_owner *owner=&owners[slot];policydb_t *policy=&policies[slot];
  atomic_init(&owner->claimed,0);owner->bytes=bytes[slot];owner->capacity=capacity;owner->attempts=attempts[slot];owner->attempt_capacity=ATTEMPTS;
  assert(sl_sepol_memory_begin(owner)==0);int result=policydb_init(policy);assert(policy->sl_source_owner==owner);
  if(owner->first_failure){assert(result!=0);failed++;}else{assert(result==0);policydb_destroy(policy);}
  int finish=sl_sepol_memory_finish(owner);if(owner->first_failure)assert(finish==-1);else assert(finish==0);
 }
 assert(failed&&slot==CASES);return 0;
}