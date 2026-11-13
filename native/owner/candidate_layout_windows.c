#include "candidate_layout_windows.h"
#include <limits.h>

static int away(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return x&&y&&an&&bn&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int span(const sl_win_candidate_source_span *s){
 return s&&s->original&&s->extent&&s->extent<=UINTPTR_MAX-(uintptr_t)s->original&&s->category<SL_WIN_COST_CATEGORY_COUNT;
}
static int same_span(const sl_win_candidate_source_span *a,const sl_win_candidate_source_span *b){
 return a==b; /* Live source occurrence, never semantic issuer authentication. */
}
static int span_equal(const sl_win_candidate_source_span *a,const sl_win_candidate_source_span *b){
 return a->original==b->original&&a->extent==b->extent&&a->category==b->category&&a->actor_index==b->actor_index&&a->occurrence_index==b->occurrence_index;
}
static int span_empty(const sl_win_candidate_source_span *v){return !v->original&&!v->extent&&!v->category&&!v->actor_index&&!v->occurrence_index;}
static int fresh(const void *p,size_t n){const unsigned char *b=p;for(size_t i=0;i<n;i++)if(b[i])return 0;return 1;}
static int add(uint64_t *sum,uint64_t n){if(n>UINT64_MAX-*sum)return 0;*sum+=n;return 1;}
static int source_away(const sl_win_candidate_layout_source *s,const void *out,size_t n){
 if(!away(out,n,s,sizeof(*s))||!away(out,n,s->items_original,s->item_count*sizeof(*s->items_original))||
    !away(out,n,s->costs_original,s->cost_count*sizeof(*s->costs_original))||
    !away(out,n,s->actors_original.original,s->actors_original.extent)||
    !away(out,n,s->profile_original.original,s->profile_original.extent)||
    !away(out,n,s->abi_original.original,s->abi_original.extent))return 0;
 for(size_t i=0;i<s->item_count;i++){
  const sl_win_candidate_source_span *v=s->items_original[i].source_original;
  if(!span(v)||!away(out,n,v,sizeof(*v))||!away(out,n,v->original,v->extent))return 0;
 }
 for(size_t i=0;i<s->cost_count;i++){
  const sl_win_candidate_cost_item *c=&s->costs_original[i];
  if(!span(c->source_original)||!away(out,n,c->source_original,sizeof(*c->source_original))||
     !away(out,n,c->source_original->original,c->source_original->extent)||
     !away(out,n,c->overlap_original,c->overlap_extent))return 0;
 }return 1;
}
static int source_shape(const sl_win_candidate_layout_source *s){
 return s&&s->items_original&&s->item_count==SL_WIN_LAYOUT_ROLE_COUNT&&
 s->item_count<=SIZE_MAX/sizeof(*s->items_original)&&s->costs_original&&s->cost_count>=s->item_count&&
 s->cost_count<=SIZE_MAX/sizeof(*s->costs_original)&&span(&s->actors_original)&&span(&s->profile_original)&&span(&s->abi_original);
}
int sl_win_plan_candidate_layout(sl_win_candidate_layout *p){
 const sl_win_candidate_layout_source *s;
 if(!p||p->attempted||p->returned||!(s=p->source_original)||!source_shape(s)||
    !p->copies_original||p->copy_capacity!=s->item_count||!p->cost_copies_original||p->cost_capacity!=s->cost_count)return 0;
 size_t in=s->item_count*sizeof(*p->copies_original),cn=s->cost_count*sizeof(*p->cost_copies_original);
 if(!source_away(s,p,sizeof(*p))||!source_away(s,p->copies_original,in)||!source_away(s,p->cost_copies_original,cn)||
    !away(p,sizeof(*p),p->copies_original,in)||!away(p,sizeof(*p),p->cost_copies_original,cn)||!away(p->copies_original,in,p->cost_copies_original,cn))return 0;
 if(p->actual_source||p->actual_copies||p->actual_cost_copies||!fresh(&p->source_snapshot,sizeof(p->source_snapshot))||!fresh(p->copies_original,in)||!fresh(p->cost_copies_original,cn)||
    p->bytes||p->alignment||p->failed_index||p->resident_bytes||p->reserved_bytes||p->kernel_bytes||p->retained_failure_bytes||p->total_cost_bytes||p->result||
    !fresh(p->category_bytes,sizeof(p->category_bytes)))return 0;
 /* No output is touched before complete source/output exclusions above. */
 p->attempted=1;p->result=0;p->bytes=0;p->alignment=1;
 p->source_snapshot=*s;
 p->actual_source=s;p->actual_copies=p->copies_original;p->actual_cost_copies=p->cost_copies_original;
 p->resident_bytes=p->reserved_bytes=p->kernel_bytes=p->retained_failure_bytes=0;
 p->total_cost_bytes=0;
 for(unsigned c=0;c<SL_WIN_COST_CATEGORY_COUNT;c++)p->category_bytes[c]=0;
 for(size_t i=0;i<s->item_count;i++){
  const sl_win_candidate_layout_item *v=&s->items_original[i];p->failed_index=i;
  if(v->role!=i||!span(v->source_original)||!span_empty(&v->source_snapshot)||!v->count||!v->element_extent||v->offset||v->extent||
     !v->alignment||(v->alignment&(v->alignment-1))||v->count>SIZE_MAX/v->element_extent)goto done;
  size_t n=v->count*v->element_extent,pad=(0-p->bytes)&(v->alignment-1);
  if(pad>SIZE_MAX-p->bytes||n>SIZE_MAX-p->bytes-pad)goto done;
  p->copies_original[i]=*v;p->copies_original[i].source_snapshot=*v->source_original;p->copies_original[i].offset=p->bytes+pad;p->copies_original[i].extent=n;
  p->bytes+=pad+n;if(v->alignment>p->alignment)p->alignment=v->alignment;
 }
 for(size_t i=0;i<s->cost_count;i++){
  const sl_win_candidate_cost_item *v=&s->costs_original[i];uint64_t total=0;p->failed_index=i;
  if(v->role>=SL_WIN_LAYOUT_ROLE_COUNT||!span(v->source_original)||!span_empty(&v->source_snapshot)||!v->overlap_original||!v->overlap_extent||
     (i<s->item_count&&(v->role!=i||!same_span(v->source_original,s->items_original[i].source_original)))||
     (i<s->item_count&&v->resident_bytes<p->copies_original[i].extent&&v->reserved_bytes<p->copies_original[i].extent)||
     (!v->resident_bytes&&!v->reserved_bytes&&!v->kernel_bytes&&!v->retained_failure_bytes))goto done;
  if(!add(&p->resident_bytes,v->resident_bytes)||!add(&p->reserved_bytes,v->reserved_bytes)||
     !add(&p->kernel_bytes,v->kernel_bytes)||!add(&p->retained_failure_bytes,v->retained_failure_bytes)||
     !add(&total,v->resident_bytes)||!add(&total,v->reserved_bytes)||!add(&total,v->kernel_bytes)||
     !add(&total,v->retained_failure_bytes)||!add(&p->category_bytes[v->source_original->category],total)||!add(&p->total_cost_bytes,total))goto done;
  p->cost_copies_original[i]=*v;p->cost_copies_original[i].source_snapshot=*v->source_original;
 }
 for(unsigned c=0;c<SL_WIN_COST_CATEGORY_COUNT;c++)if(!p->category_bytes[c])goto done;
 p->failed_index=s->item_count;p->result=1;
done:p->returned=1;return p->result;
}
int sl_win_candidate_layout_matches_original(const sl_win_candidate_layout *p){
 const sl_win_candidate_layout_source *s;
 if(!p||!p->attempted||!p->returned||!p->result||!(s=p->source_original)||s!=p->actual_source||
    p->copies_original!=p->actual_copies||p->cost_copies_original!=p->actual_cost_copies||!source_shape(s)||
    p->copy_capacity!=s->item_count||p->cost_capacity!=s->cost_count||!p->copies_original||!p->cost_copies_original)return 0;
 if(s->items_original!=p->source_snapshot.items_original||s->item_count!=p->source_snapshot.item_count||
    s->costs_original!=p->source_snapshot.costs_original||s->cost_count!=p->source_snapshot.cost_count||
    !span_equal(&s->actors_original,&p->source_snapshot.actors_original)||!span_equal(&s->profile_original,&p->source_snapshot.profile_original)||
    !span_equal(&s->abi_original,&p->source_snapshot.abi_original))return 0;
 size_t bytes=0,alignment=1;uint64_t totals[4]={0},categories[SL_WIN_COST_CATEGORY_COUNT]={0},all=0;
 for(size_t i=0;i<s->item_count;i++){
  const sl_win_candidate_layout_item *v=&s->items_original[i],*copy=&p->copies_original[i];
  if(v->role!=i||!span(v->source_original)||!span_empty(&v->source_snapshot)||!span_equal(v->source_original,&copy->source_snapshot)||!v->count||!v->element_extent||v->offset||v->extent||!v->alignment||
     (v->alignment&(v->alignment-1))||v->count>SIZE_MAX/v->element_extent)return 0;
  size_t n=v->count*v->element_extent,pad=(0-bytes)&(v->alignment-1);
  if(pad>SIZE_MAX-bytes||n>SIZE_MAX-bytes-pad||copy->source_original!=v->source_original||copy->role!=v->role||
     copy->count!=v->count||copy->element_extent!=v->element_extent||copy->alignment!=v->alignment||copy->offset!=bytes+pad||copy->extent!=n)return 0;
  bytes+=pad+n;if(v->alignment>alignment)alignment=v->alignment;
 }
 for(size_t i=0;i<s->cost_count;i++){
  const sl_win_candidate_cost_item *v=&s->costs_original[i],*c=&p->cost_copies_original[i];uint64_t total=0;
  if(v->role>=SL_WIN_LAYOUT_ROLE_COUNT||!span(v->source_original)||!span_empty(&v->source_snapshot)||!span_equal(v->source_original,&c->source_snapshot)||!v->overlap_original||!v->overlap_extent||
     (i<s->item_count&&(v->role!=i||v->source_original!=s->items_original[i].source_original))||
     (i<s->item_count&&v->resident_bytes<p->copies_original[i].extent&&v->reserved_bytes<p->copies_original[i].extent)||
     c->source_original!=v->source_original||c->role!=v->role||c->overlap_original!=v->overlap_original||c->overlap_extent!=v->overlap_extent||
     c->resident_bytes!=v->resident_bytes||c->reserved_bytes!=v->reserved_bytes||c->kernel_bytes!=v->kernel_bytes||c->retained_failure_bytes!=v->retained_failure_bytes||
     (!v->resident_bytes&&!v->reserved_bytes&&!v->kernel_bytes&&!v->retained_failure_bytes)||
     !add(&totals[0],v->resident_bytes)||!add(&totals[1],v->reserved_bytes)||!add(&totals[2],v->kernel_bytes)||!add(&totals[3],v->retained_failure_bytes)||
     !add(&total,v->resident_bytes)||!add(&total,v->reserved_bytes)||!add(&total,v->kernel_bytes)||!add(&total,v->retained_failure_bytes)||
     !add(&categories[v->source_original->category],total)||!add(&all,total))return 0;
 }
 for(unsigned c=0;c<SL_WIN_COST_CATEGORY_COUNT;c++)if(!categories[c]||categories[c]!=p->category_bytes[c])return 0;
 return bytes==p->bytes&&alignment==p->alignment&&all==p->total_cost_bytes&&totals[0]==p->resident_bytes&&totals[1]==p->reserved_bytes&&totals[2]==p->kernel_bytes&&totals[3]==p->retained_failure_bytes;
}
int sl_win_candidate_layout_excludes_output(const sl_win_candidate_layout *p,const void *out,size_t n){
 if(!sl_win_candidate_layout_matches_original(p))return 0;
 return away(out,n,p,sizeof(*p))&&away(out,n,p->copies_original,p->copy_capacity*sizeof(*p->copies_original))&&
 away(out,n,p->cost_copies_original,p->cost_capacity*sizeof(*p->cost_copies_original))&&source_away(p->source_original,out,n);
}
