#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "containing_charge_linux.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
static int charge_disjoint(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int charge_fresh(const void *p,unsigned long n){const unsigned char *b=p;for(unsigned long i=0;i<n;i++)if(b[i])return 0;return 1;}
static int charge_plan_exclusion(const void *out,unsigned long bytes,const sl_linux_candidate_layout_plan *p){
 if(!charge_disjoint(out,bytes,p,sizeof(*p))||!p->attempted||!p->returned||p->result||
    !p->actual_quantities||p->actual_quantities!=p->quantities_original||
    !p->actual_candidate||p->actual_candidate!=p->candidate_original||
    !charge_disjoint(out,bytes,p->actual_quantities,sizeof(*p->actual_quantities))||
    !charge_disjoint(out,bytes,p->actual_candidate,sizeof(*p->actual_candidate)))return 0;
 const sl_candidate_profile *c=p->actual_candidate;
 if(!c->returned||c->result||p->source_original!=c->source||p->source_extent!=c->source_extent||
    c->original.bytes!=c->source||c->original.extent!=c->source_extent||
    p->protected_rows!=c->actual_capacities[3]||!p->source_extent||
    !charge_disjoint(out,bytes,p->source_original,p->source_extent)||
    memcmp(&p->prepared_quantities,p->actual_quantities,sizeof(p->prepared_quantities)))return 0;
 for(unsigned i=0;i<5;i++){
  unsigned long n=c->actual_capacities[i];
  if(n>ULONG_MAX/sizeof(sl_candidate_record)||(n&&!charge_disjoint(out,bytes,c->actual_rows[i],n*sizeof(sl_candidate_record))))return 0;
 }
 return 1;
}
static int charge_same_source(const sl_linux_containing_source *a,const sl_linux_containing_source *b){
 return a->source_graph_original==b->source_graph_original&&a->source_graph_extent==b->source_graph_extent&&
 a->domain_original==b->domain_original&&a->domain_extent==b->domain_extent&&
 a->available_bytes==b->available_bytes&&a->page_size==b->page_size&&a->items_original==b->items_original&&a->count==b->count;
}
static int charge_same_item(const sl_linux_charge_item *a,const sl_linux_charge_item *b){
 return a->actor_original==b->actor_original&&a->actor_extent==b->actor_extent&&
 a->basis_original==b->basis_original&&a->basis_extent==b->basis_extent&&a->bytes==b->bytes&&a->category==b->category;
}
int sl_linux_containing_charge_matches_original(const sl_linux_containing_charge *o){
 if(!o||!o->attempted||!o->returned||o->result||!o->plan_returned||o->plan_result||
    !o->actual_source||o->actual_source!=o->source_original||!o->actual_plan||o->actual_plan!=o->plan_original||
    !o->actual_copies||o->actual_copies!=o->copies_original||!o->actual_capacity||o->actual_capacity!=o->capacity||
    o->count!=o->actual_capacity||o->count>ULONG_MAX/sizeof(*o->actual_copies)||
    !charge_same_source(o->actual_source,&o->source_snapshot)||o->source_snapshot.count!=o->count||
    !o->source_snapshot.available_bytes||!o->source_snapshot.page_size||(o->source_snapshot.page_size&(o->source_snapshot.page_size-1)))return 0;
 unsigned long bytes=o->count*sizeof(*o->actual_copies);
 const sl_linux_candidate_layout_plan *p=o->actual_plan,*checked=&o->checked_plan;
 if(!charge_disjoint(o,sizeof(*o),o->actual_copies,bytes)||
    !charge_disjoint(o,sizeof(*o),o->actual_source,sizeof(*o->actual_source))||
    !charge_disjoint(o->actual_copies,bytes,o->actual_source,sizeof(*o->actual_source))||
    !charge_plan_exclusion(o,sizeof(*o),p)||!charge_plan_exclusion(o->actual_copies,bytes,p)||
    !o->source_snapshot.items_original||
    !o->source_snapshot.domain_original||!o->source_snapshot.domain_extent||
    !o->source_snapshot.source_graph_original||!o->source_snapshot.source_graph_extent||
    !charge_disjoint(o,sizeof(*o),o->source_snapshot.domain_original,o->source_snapshot.domain_extent)||
    !charge_disjoint(o->actual_copies,bytes,o->source_snapshot.domain_original,o->source_snapshot.domain_extent)||
    !charge_disjoint(o,sizeof(*o),o->source_snapshot.source_graph_original,o->source_snapshot.source_graph_extent)||
    !charge_disjoint(o->actual_copies,bytes,o->source_snapshot.source_graph_original,o->source_snapshot.source_graph_extent)||
    !charge_disjoint(o,sizeof(*o),o->source_snapshot.items_original,bytes)||
    !charge_disjoint(o->actual_copies,bytes,o->source_snapshot.items_original,bytes)||
    !checked->attempted||!checked->returned||checked->result||
    checked->quantities_original!=p->actual_quantities||checked->actual_quantities!=p->actual_quantities||
    checked->candidate_original!=p->actual_candidate||checked->actual_candidate!=p->actual_candidate||
    checked->source_original!=p->source_original||checked->source_extent!=p->source_extent||checked->protected_rows!=p->protected_rows||
    memcmp(&checked->prepared_quantities,&p->prepared_quantities,sizeof(p->prepared_quantities))||
    memcmp(&checked->layout,&p->layout,sizeof(p->layout)))return 0;
 unsigned long raw=checked->layout.bytes,page=o->source_snapshot.page_size,padding=(page-raw%page)%page;
 if(!raw||padding>ULONG_MAX-raw||o->arena_bytes!=raw+padding)return 0;
 unsigned long total=o->arena_bytes,categories[SL_CHARGE_CATEGORY_COUNT]={0};
 for(unsigned long i=0;i<o->count;i++){
  const sl_linux_charge_item *item=&o->actual_copies[i];
  if(!item->actor_original||!item->actor_extent||!item->basis_original||!item->basis_extent||
     !charge_disjoint(o,sizeof(*o),item->actor_original,item->actor_extent)||
     !charge_disjoint(o->actual_copies,bytes,item->actor_original,item->actor_extent)||
     !charge_disjoint(o,sizeof(*o),item->basis_original,item->basis_extent)||
     !charge_disjoint(o->actual_copies,bytes,item->basis_original,item->basis_extent)||
     !item->bytes||item->category>=SL_CHARGE_CATEGORY_COUNT||
     !charge_same_item(item,&o->source_snapshot.items_original[i])||
     item->bytes>ULONG_MAX-total||item->bytes>ULONG_MAX-categories[item->category])return 0;
  total+=item->bytes;categories[item->category]+=item->bytes;
 }
 return total==o->total&&total<=o->source_snapshot.available_bytes&&
 o->remaining==o->source_snapshot.available_bytes-total&&!memcmp(categories,o->categories,sizeof(categories));
 /* Fixed returning arithmetic/source matcher only; it authenticates neither
  * the prior domain nor completeness/actual lifetimes of its source roster. */
}
int sl_linux_containing_charge_excludes_output(const sl_linux_containing_charge *o,const void *out,unsigned long extent){
 if(!extent||!sl_linux_containing_charge_matches_original(o)||
    !charge_disjoint(out,extent,o,sizeof(*o)))return 0;
 const sl_linux_containing_source *s=o->actual_source;
 unsigned long bytes=o->count*sizeof(*o->actual_copies);
 if(!charge_disjoint(out,extent,s,sizeof(*s))||
    !charge_disjoint(out,extent,o->actual_copies,bytes)||
    !charge_disjoint(out,extent,s->items_original,bytes)||
    !charge_disjoint(out,extent,s->domain_original,s->domain_extent)||
    !charge_disjoint(out,extent,s->source_graph_original,s->source_graph_extent)||
    !charge_plan_exclusion(out,extent,o->actual_plan))return 0;
 for(unsigned long i=0;i<o->count;i++){
  const sl_linux_charge_item *item=&o->actual_copies[i];
  if(!charge_disjoint(out,extent,item->actor_original,item->actor_extent)||
     !charge_disjoint(out,extent,item->basis_original,item->basis_extent))return 0;
 }
 return 1;
}
long sl_linux_construct_containing_charge(sl_linux_containing_charge *o){
 if(!o||!o->source_original||!o->plan_original||!o->copies_original||!o->capacity||
    o->capacity>ULONG_MAX/sizeof(*o->copies_original)||sizeof(*o)>UINTPTR_MAX-(uintptr_t)o)return -EINVAL;
 const sl_linux_containing_source *s=o->source_original;
 const sl_linux_candidate_layout_plan *p=o->plan_original;
 unsigned long bytes=o->capacity*sizeof(*o->copies_original);
 if(!charge_disjoint(o,sizeof(*o),s,sizeof(*s))||!charge_disjoint(o,sizeof(*o),o->copies_original,bytes)||
    !charge_disjoint(o->copies_original,bytes,s,sizeof(*s))||
    !charge_fresh(&o->actual_source,sizeof(*o)-offsetof(sl_linux_containing_charge,actual_source))||
    !charge_fresh(o->copies_original,bytes)||!s->items_original||s->count!=o->capacity||
    !s->source_graph_original||!s->source_graph_extent||!s->domain_original||!s->domain_extent||
    !s->available_bytes||!s->page_size||(s->page_size&(s->page_size-1))||
    !charge_plan_exclusion(o,sizeof(*o),p)||!charge_plan_exclusion(o->copies_original,bytes,p)||
    !charge_disjoint(o,sizeof(*o),s->items_original,bytes)||!charge_disjoint(o->copies_original,bytes,s->items_original,bytes)||
    !charge_disjoint(o,sizeof(*o),s->domain_original,s->domain_extent)||!charge_disjoint(o->copies_original,bytes,s->domain_original,s->domain_extent)||
    !charge_disjoint(o,sizeof(*o),s->source_graph_original,s->source_graph_extent)||!charge_disjoint(o->copies_original,bytes,s->source_graph_original,s->source_graph_extent))return -EINVAL;
 for(unsigned long i=0;i<s->count;i++){
  const sl_linux_charge_item *item=&s->items_original[i];
  if(!item->actor_original||!item->actor_extent||!item->basis_original||!item->basis_extent||!item->bytes||item->category>=SL_CHARGE_CATEGORY_COUNT||
     !charge_disjoint(o,sizeof(*o),item->actor_original,item->actor_extent)||!charge_disjoint(o->copies_original,bytes,item->actor_original,item->actor_extent)||
     !charge_disjoint(o,sizeof(*o),item->basis_original,item->basis_extent)||!charge_disjoint(o->copies_original,bytes,item->basis_original,item->basis_extent))return -EINVAL;
 }
 o->attempted=1;o->result=-EINVAL;o->actual_source=s;o->actual_plan=p;
 o->actual_copies=o->copies_original;o->actual_capacity=o->capacity;o->source_snapshot=*s;
 o->checked_plan.quantities_original=p->actual_quantities;o->checked_plan.candidate_original=p->actual_candidate;
 o->plan_result=sl_linux_plan_candidate_arena(&o->checked_plan);o->plan_returned=1;
 if(o->plan_result||!o->checked_plan.returned||o->checked_plan.result||memcmp(&o->checked_plan.layout,&p->layout,sizeof(p->layout)))goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++)o->actual_copies[i]=s->items_original[i];
 o->count=o->actual_capacity;
 unsigned long raw=o->checked_plan.layout.bytes,page=o->source_snapshot.page_size;
 unsigned long padding=(page-raw%page)%page;
 if(!raw||padding>ULONG_MAX-raw){o->result=-EOVERFLOW;goto done;}
 o->arena_bytes=raw+padding;o->total=o->arena_bytes;
 for(unsigned long i=0;i<o->count;i++){
  const sl_linux_charge_item *item=&o->actual_copies[i];
  if(!item->bytes||item->category>=SL_CHARGE_CATEGORY_COUNT||!charge_same_item(&s->items_original[i],item))goto done;
  if(item->bytes>ULONG_MAX-o->total||item->bytes>ULONG_MAX-o->categories[item->category]){o->result=-EOVERFLOW;goto done;}
  o->total+=item->bytes;o->categories[item->category]+=item->bytes;
 }
 if(!charge_same_source(s,&o->source_snapshot))goto done;
 for(unsigned long i=0;i<o->count;i++)if(!charge_same_item(&s->items_original[i],&o->actual_copies[i]))goto done;
 if(o->total>o->source_snapshot.available_bytes){o->result=-ENOSPC;goto done;}
 o->remaining=o->source_snapshot.available_bytes-o->total;o->result=0;
done:o->returned=1;if(!o->result&&!sl_linux_containing_charge_matches_original(o))o->result=-EINVAL;return o->result;
 /* Retains all original inputs/copies/geometry on refusal. No native allocation,
  * domain transfer, actor authentication, reclamation or usable entry grant. */
}
int sl_linux_charge_attachment_matches_original(const sl_linux_charge_attachment *o,const sl_linux_containing_charge *charge){
 return o&&charge&&o->attempted==1&&o->returned==1&&o->charge_original==charge&&
  sl_linux_containing_charge_excludes_output(charge,o,sizeof(*o));
}
int sl_linux_attach_containing_charge(sl_linux_charge_attachment *o,const sl_linux_containing_charge *charge){
 if(!o||!charge||!sl_linux_containing_charge_excludes_output(charge,o,sizeof(*o))||
    !charge_fresh(o,sizeof(*o)))return 0;
 o->attempted=1;o->charge_original=charge;o->returned=1;
 return sl_linux_charge_attachment_matches_original(o,charge);
 /* Association only: no source/domain authentication or new credit. */
}
#endif
