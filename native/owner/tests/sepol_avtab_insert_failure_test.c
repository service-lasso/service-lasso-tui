/* UNRUN actual owned AVTAB partial-insert -> destructor -> next-owner boundary.
 * Link pinned owned library/redirects; no target execution admission. */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <sepol/policydb/avtab.h>
#include "../sepol_source_owner.h"
static union{max_align_t alignment;unsigned char bytes[8192];} backing[3];
static sl_sepol_memory_attempt attempts[3][32];
static void begin(sl_sepol_memory_owner *o,unsigned slot,size_t count){memset(o,0,sizeof *o);o->bytes=backing[slot].bytes;o->capacity=sizeof backing[slot].bytes;o->attempts=attempts[slot];o->attempt_capacity=count;atomic_init(&o->claimed,0);assert(!sl_sepol_memory_begin(o));}
int main(void){sl_sepol_memory_owner owners[3];avtab_t tables[2]={{0}};avtab_ptr_t *buckets[2];sl_sepol_memory_attempt *failures[2];avtab_key_t key={.source_type=1,.target_type=1,.target_class=1,.specified=AVTAB_ALLOWED};avtab_datum_t datum={.data=9};
 for(unsigned i=0;i<2;i++){begin(&owners[i],i,i?3:2);assert(!avtab_init(&tables[i])&&!avtab_alloc(&tables[i],1));buckets[i]=tables[i].htable;assert(avtab_insert(&tables[i],&key,&datum)!=0);failures[i]=owners[i].first_failure;assert(failures[i]==&owners[i].exhaustion&&tables[i].nel==0&&tables[i].htable==buckets[i]);if(!i)assert(failures[i]->operation==SL_SEPOL_POINTER_MUTATION);else{assert(failures[i]->operation==SL_SEPOL_SCALAR_MUTATION&&failures[i]->field_original==&tables[i].nel);unsigned found=0;for(unsigned j=0;j<tables[i].nslot;j++)if(buckets[i][j]){assert(buckets[i][j]->sl_source_owner==&owners[i]&&buckets[i][j]->datum.data==9);found++;}assert(found==1);}size_t prior=owners[i].attempt_count;avtab_destroy(&tables[i]);assert(tables[i].htable==buckets[i]&&owners[i].attempt_count==prior&&owners[i].first_failure==failures[i]);assert(sl_sepol_memory_finish(&owners[i])!=0);}
 begin(&owners[2],2,32);for(unsigned i=0;i<2;i++){assert(!avtab_search(&tables[i],&key)&&!avtab_search_node(&tables[i],&key)&&avtab_insert(&tables[i],&key,&datum)!=0&&avtab_init(&tables[i])!=0);avtab_destroy(&tables[i]);assert(tables[i].htable==buckets[i]&&owners[i].first_failure==failures[i]&&tables[i].sl_source_owner==&owners[i]);}assert(!owners[2].attempt_count&&!owners[2].failed&&!sl_sepol_memory_finish(&owners[2]));return 0;}