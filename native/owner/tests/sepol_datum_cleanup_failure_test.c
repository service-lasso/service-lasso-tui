/* UNRUN: actual type/role/user cleanup sibling retention, no disposal grant. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/policydb.h>
static union {max_align_t alignment;unsigned char bytes[4096];} backing[3];
static sl_sepol_memory_attempt rows[3][2];
int main(void){
 sl_sepol_memory_owner owners[3]={{0}};type_set_t types={0};role_datum_t role={0};user_datum_t user={0};
 for(unsigned i=0;i<3;i++){
  sl_sepol_memory_owner *o=&owners[i];o->bytes=backing[i].bytes;o->capacity=sizeof backing[i].bytes;o->attempts=rows[i];o->attempt_capacity=2;atomic_init(&o->claimed,0);assert(!sl_sepol_memory_begin(o));
  ebitmap_node_t *first=sl_sepol_source_calloc(1,sizeof *first,__FILE__,__LINE__),*second=sl_sepol_source_calloc(1,sizeof *second,__FILE__,__LINE__);assert(first&&second);first->map=1;second->map=2;
  ebitmap_t *a=i==0?&types.types:i==1?&role.dominates:&user.roles.roles,*b=i==0?&types.negset:i==1?&role.cache:&user.cache;a->node=first;a->highbit=MAPSIZE;b->node=second;b->highbit=MAPSIZE;
  if(!i)type_set_destroy(&types);else if(i==1)role_datum_destroy(&role);else user_datum_destroy(&user);
  assert(o->first_failure==&o->exhaustion&&o->exhaustion.input==first&&o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->attempt_count==2);assert(a->node==first&&b->node==second&&a->highbit==MAPSIZE&&b->highbit==MAPSIZE&&first->map==1&&second->map==2);assert(sl_sepol_memory_finish(o)!=0);
 }
 assert(types.negset.node&&role.cache.node&&user.cache.node);return 0;
}