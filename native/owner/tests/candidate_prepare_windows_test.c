#ifdef _WIN32
/* UNRUN pure prebinding fixture. No Windows API, process, reservation, native
 * issuer, genuine source/profile/family or activation is represented here. */
#include "../candidate_prepare_windows.h"
#include "../debug_root_windows.h"
#include "../argument_root_windows.h"
#include "../argument_reader_cancel_windows.h"
#include "../candidate_profile.h"
#include <assert.h>
#include <string.h>
typedef struct fixture {
 sl_win_candidate_layout plan;sl_win_candidate_layout_source source;
 sl_win_candidate_layout_item items[SL_WIN_LAYOUT_ROLE_COUNT],copies[SL_WIN_LAYOUT_ROLE_COUNT];
 sl_win_candidate_cost_item costs[SL_WIN_LAYOUT_ROLE_COUNT],costs_copy[SL_WIN_LAYOUT_ROLE_COUNT];
 sl_win_candidate_source_span spans[SL_WIN_LAYOUT_ROLE_COUNT];
 unsigned char originals[SL_WIN_LAYOUT_ROLE_COUNT],source_bytes[3],overlap;
 union {max_align_t align;unsigned char bytes[131072];} payload;
 sl_win_candidate_preparation preparation;
} fixture;
static void typed(fixture *f,unsigned role,size_t count,size_t unit,size_t alignment){
 f->items[role].count=count;f->items[role].element_extent=unit;f->items[role].alignment=alignment;
 f->costs[role].reserved_bytes=count*unit;
}
static void setup(fixture *f){
 memset(f,0,sizeof(*f));
 for(unsigned i=0;i<SL_WIN_LAYOUT_ROLE_COUNT;i++){
  f->spans[i]=(sl_win_candidate_source_span){&f->originals[i],1,i%SL_WIN_COST_CATEGORY_COUNT,i,0};
  f->items[i].source_original=&f->spans[i];f->items[i].role=i;
  f->costs[i].source_original=&f->spans[i];f->costs[i].role=i;f->costs[i].overlap_original=&f->overlap;f->costs[i].overlap_extent=1;
  typed(f,i,1024,1,_Alignof(max_align_t));
 }
 #define T(role,type,count) typed(f,role,count,sizeof(type),_Alignof(type))
 T(SL_WIN_LAYOUT_STARTUP,sl_win_public_startup,1);T(SL_WIN_LAYOUT_FACTORY_CONTROL,sl_win_argument_sender_factory,1);
 T(SL_WIN_LAYOUT_PIPE_NAME,WCHAR,128);T(SL_WIN_LAYOUT_ARGUMENT_OUTPUT,WCHAR,128);T(SL_WIN_LAYOUT_MODULE_ARENA,WCHAR,128);
 T(SL_WIN_LAYOUT_MODULE_QUERIES,sl_win_module_query,2);T(SL_WIN_LAYOUT_COMMAND,WCHAR,128);
 T(SL_WIN_LAYOUT_CONNECTIONS,sl_win_duplex_completion,2);T(SL_WIN_LAYOUT_SETTLEMENTS,sl_win_duplex_settlement,2);
 T(SL_WIN_LAYOUT_SERVICES,sl_win_service_observation,2);T(SL_WIN_LAYOUT_TURNS,sl_win_argument_send_turn,2);
 T(SL_WIN_LAYOUT_DEBUG_EVENTS,sl_win_debug_event_observation,2);T(SL_WIN_LAYOUT_MAPPED_ROOTS,sl_win_mapped_root_observation,2);
 T(SL_WIN_LAYOUT_ROOT_WRITES,sl_win_root_write,2);T(SL_WIN_LAYOUT_DEBUG_RELEASES,sl_win_debug_release,2);
 T(SL_WIN_LAYOUT_ARGUMENT_ROOTS,sl_win_argument_root_source,2);T(SL_WIN_LAYOUT_READER_CANCELS,sl_win_argument_reader_cancel,2);
 T(SL_WIN_LAYOUT_PROFILE_CONTROL,sl_candidate_profile,1);T(SL_WIN_LAYOUT_PROFILE_ROWS,sl_candidate_record,5);
 #undef T
 f->source.items_original=f->items;f->source.item_count=SL_WIN_LAYOUT_ROLE_COUNT;
 f->source.costs_original=f->costs;f->source.cost_count=SL_WIN_LAYOUT_ROLE_COUNT;
 f->source.actors_original=(sl_win_candidate_source_span){&f->source_bytes[0],1,0,0,0};
 f->source.profile_original=(sl_win_candidate_source_span){&f->source_bytes[1],1,1,0,0};
 f->source.abi_original=(sl_win_candidate_source_span){&f->source_bytes[2],1,0,0,0};
 f->plan.source_original=&f->source;f->plan.copies_original=f->copies;f->plan.copy_capacity=SL_WIN_LAYOUT_ROLE_COUNT;
 f->plan.cost_copies_original=f->costs_copy;f->plan.cost_capacity=SL_WIN_LAYOUT_ROLE_COUNT;
}
static void planned(fixture *f){
 assert(sl_win_plan_candidate_layout(&f->plan));assert(f->plan.bytes<=sizeof(f->payload.bytes));
 f->preparation.plan_original=&f->plan;f->preparation.backing_original=f->payload.bytes;
 f->preparation.backing_extent=f->plan.bytes;f->preparation.writer_stack_original=4096;
}
int main(void){
 static fixture f;setup(&f);planned(&f);
 assert(sl_win_prepare_candidate_views(&f.preparation));
 sl_win_public_startup *o=f.preparation.startup_original;
 assert(o==(void *)(f.payload.bytes+f.copies[SL_WIN_LAYOUT_STARTUP].offset));
 assert(o->arguments.module_queries==(void *)(f.payload.bytes+f.copies[SL_WIN_LAYOUT_MODULE_QUERIES].offset));
 assert(o->controller.entry.birth.security[5].backing==(void *)(f.payload.bytes+f.copies[SL_WIN_LAYOUT_SECURITY_5].offset));
 assert(o->argument_sequence.turns==NULL&&!o->attempted&&!o->controller.entry.birth.attempted);
 assert(!f.preparation.factory_original->attempted); /* Preparation does not grant factory/startup entry. */
 assert(!sl_win_prepare_candidate_views(&f.preparation));
 setup(&f);typed(&f,SL_WIN_LAYOUT_SERVICES,2,1,1);planned(&f);
 assert(!sl_win_prepare_candidate_views(&f.preparation)&&!f.preparation.attempted);
 for(size_t i=0;i<f.plan.bytes;i++)assert(f.payload.bytes[i]==0); /* No partial native-view write. */
 setup(&f);planned(&f);f.payload.bytes[f.plan.bytes-1]=1;
 assert(!sl_win_prepare_candidate_views(&f.preparation)&&!f.preparation.attempted);
 setup(&f);planned(&f);f.preparation.backing_original=f.copies;f.preparation.backing_extent=f.plan.bytes;
 assert(!sl_win_prepare_candidate_views(&f.preparation)&&!f.preparation.attempted);
 setup(&f);typed(&f,SL_WIN_LAYOUT_COMMAND,1,sizeof(WCHAR),_Alignof(WCHAR));planned(&f);
 assert(!sl_win_prepare_candidate_views(&f.preparation)&&!f.preparation.attempted);
 return 0;
}
#endif
