/* UNRUN: actual semantic MLS/constraint/conditional failure propagation. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/policydb.h>
#include <sepol/policydb/conditional.h>
#include <sepol/policydb/constraint.h>
static union {max_align_t align;unsigned char bytes[4096];} backing[3];
static sl_sepol_memory_attempt rows[3][2];
int main(void){
 sl_sepol_memory_owner owners[3]={{0}};mls_semantic_range_t range={0};constraint_expr_t constraint={0};cond_node_t conditional={0};
 for(unsigned i=0;i<3;i++){
  sl_sepol_memory_owner *o=&owners[i];o->bytes=backing[i].bytes;o->capacity=sizeof backing[i].bytes;o->attempts=rows[i];o->attempt_capacity=2;atomic_init(&o->claimed,0);assert(!sl_sepol_memory_begin(o));void *first,*second;
  if(!i){mls_semantic_cat_t *a=sl_sepol_source_calloc(1,sizeof *a,__FILE__,__LINE__),*b=sl_sepol_source_calloc(1,sizeof *b,__FILE__,__LINE__);assert(a&&b);first=a;second=b;range.level[0].cat=a;range.level[1].cat=b;mls_semantic_range_destroy(&range);assert(range.level[0].cat==a&&range.level[1].cat==b);}
  else if(i==1){ebitmap_node_t *a=sl_sepol_source_calloc(1,sizeof *a,__FILE__,__LINE__);type_set_t *b=sl_sepol_source_calloc(1,sizeof *b,__FILE__,__LINE__);assert(a&&b);first=a;second=b;constraint.names.node=a;constraint.names.highbit=MAPSIZE;constraint.type_names=b;constraint_expr_destroy(&constraint);assert(constraint.names.node==a&&constraint.names.highbit==MAPSIZE&&constraint.type_names==b);}
  else{cond_expr_t *a=sl_sepol_source_calloc(1,sizeof *a,__FILE__,__LINE__),*b=sl_sepol_source_calloc(1,sizeof *b,__FILE__,__LINE__);assert(a&&b);first=a;second=b;a->next=b;conditional.expr=a;cond_node_destroy(&conditional);assert(conditional.expr==a&&a->next==b);}
  assert(first&&second&&o->first_failure==&o->exhaustion&&o->exhaustion.input==first&&o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->attempt_count==2&&!rows[i][0].logically_released&&!rows[i][1].logically_released);assert(sl_sepol_memory_finish(o)!=0);
 }
 assert(range.level[1].cat&&constraint.type_names&&conditional.expr->next);return 0;
}