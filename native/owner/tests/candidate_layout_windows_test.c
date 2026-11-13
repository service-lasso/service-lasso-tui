/* Pure representation fixture, UNRUN here. Synthetic source/cost rows are not
 * genuine profile, reservation, operation, family or native issuer evidence. */
#include "../candidate_layout_windows.h"
#include <assert.h>
#include <string.h>
typedef struct fixture {
 sl_win_candidate_layout plan;
 sl_win_candidate_layout_source source;
 sl_win_candidate_layout_item items[SL_WIN_LAYOUT_ROLE_COUNT],copies[SL_WIN_LAYOUT_ROLE_COUNT];
 sl_win_candidate_cost_item costs[SL_WIN_LAYOUT_ROLE_COUNT],cost_copies[SL_WIN_LAYOUT_ROLE_COUNT];
 sl_win_candidate_source_span spans[SL_WIN_LAYOUT_ROLE_COUNT];
 unsigned char originals[SL_WIN_LAYOUT_ROLE_COUNT],actors[8],profile[8],abi[8],overlap[8];
} fixture;
static void prepare(fixture *f){
 memset(f,0,sizeof(*f));
 for(unsigned i=0;i<SL_WIN_LAYOUT_ROLE_COUNT;i++){
  f->spans[i]=(sl_win_candidate_source_span){&f->originals[i],1,i%SL_WIN_COST_CATEGORY_COUNT,i,0};
  f->items[i].source_original=&f->spans[i];f->items[i].count=1;f->items[i].element_extent=3;f->items[i].alignment=4;f->items[i].role=i;
  f->costs[i].source_original=&f->spans[i];f->costs[i].role=i;f->costs[i].reserved_bytes=3;
  f->costs[i].overlap_original=f->overlap;f->costs[i].overlap_extent=sizeof(f->overlap);
 }
 f->source.items_original=f->items;f->source.item_count=SL_WIN_LAYOUT_ROLE_COUNT;
 f->source.costs_original=f->costs;f->source.cost_count=SL_WIN_LAYOUT_ROLE_COUNT;
 f->source.actors_original=(sl_win_candidate_source_span){f->actors,sizeof(f->actors),SL_WIN_COST_CONTROL,0,0};
 f->source.profile_original=(sl_win_candidate_source_span){f->profile,sizeof(f->profile),SL_WIN_COST_QUERY,0,0};
 f->source.abi_original=(sl_win_candidate_source_span){f->abi,sizeof(f->abi),SL_WIN_COST_CONTROL,0,0};
 f->plan.source_original=&f->source;f->plan.copies_original=f->copies;f->plan.copy_capacity=SL_WIN_LAYOUT_ROLE_COUNT;
 f->plan.cost_copies_original=f->cost_copies;f->plan.cost_capacity=SL_WIN_LAYOUT_ROLE_COUNT;
}
int main(void){
 fixture f;prepare(&f);
 assert(SL_WIN_LAYOUT_TURNS==21&&SL_WIN_LAYOUT_FACTORY_CONTROL==22);
 assert(sl_win_plan_candidate_layout(&f.plan));
 assert(f.plan.bytes==4*(SL_WIN_LAYOUT_ROLE_COUNT-1)+3&&f.plan.alignment==4);
 assert(f.copies[SL_WIN_LAYOUT_FACTORY_CONTROL].offset==88); /* 22 distinct preceding regions. */
 assert(f.plan.reserved_bytes==3*SL_WIN_LAYOUT_ROLE_COUNT);
 assert(sl_win_candidate_layout_matches_original(&f.plan));
 assert(!sl_win_candidate_layout_excludes_output(&f.plan,f.actors,sizeof(f.actors)));
 assert(!sl_win_candidate_layout_excludes_output(&f.plan,f.copies,sizeof(f.copies)));
 assert(!sl_win_plan_candidate_layout(&f.plan));
 sl_win_candidate_layout_source replacement=f.source;
 f.plan.source_original=&replacement;assert(!sl_win_candidate_layout_matches_original(&f.plan));
 f.plan.source_original=&f.source;
 f.spans[4].extent=2;assert(!sl_win_candidate_layout_matches_original(&f.plan));
 f.spans[4].extent=1;assert(sl_win_candidate_layout_matches_original(&f.plan));
 f.costs[4].kernel_bytes=1;assert(!sl_win_candidate_layout_matches_original(&f.plan));
 prepare(&f);f.items[3].role=2;assert(!sl_win_plan_candidate_layout(&f.plan)&&f.plan.failed_index==3);
 prepare(&f);f.source.item_count--;assert(!sl_win_plan_candidate_layout(&f.plan)&&!f.plan.attempted);
 prepare(&f);f.source.cost_count--;assert(!sl_win_plan_candidate_layout(&f.plan)&&!f.plan.attempted);
 prepare(&f);f.items[2].alignment=3;assert(!sl_win_plan_candidate_layout(&f.plan));
 prepare(&f);f.items[2].count=SIZE_MAX;f.items[2].element_extent=2;assert(!sl_win_plan_candidate_layout(&f.plan));
 prepare(&f);f.costs[1].reserved_bytes=1;assert(!sl_win_plan_candidate_layout(&f.plan));
 prepare(&f);f.costs[0].reserved_bytes=UINT64_MAX;assert(!sl_win_plan_candidate_layout(&f.plan));
 prepare(&f);for(unsigned i=0;i<SL_WIN_LAYOUT_ROLE_COUNT;i++)f.spans[i].category=0;
 assert(!sl_win_plan_candidate_layout(&f.plan)); /* Missing whole actor cost classes refuse. */
 prepare(&f);f.plan.copies_original=f.items;
 sl_win_candidate_layout_item before=f.items[0];
 assert(!sl_win_plan_candidate_layout(&f.plan)&&!f.plan.attempted);
 assert(!memcmp(&before,&f.items[0],sizeof(before)));
 prepare(&f);f.copies[0].extent=1;assert(!sl_win_plan_candidate_layout(&f.plan)&&!f.plan.attempted&&f.copies[0].extent==1);
 prepare(&f);f.costs[0].source_original=&f.spans[1];assert(!sl_win_plan_candidate_layout(&f.plan));
 prepare(&f);f.source.abi_original.original=&f.plan;
 assert(!sl_win_plan_candidate_layout(&f.plan)&&!f.plan.attempted);
 return 0;
}
