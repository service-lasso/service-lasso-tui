#ifdef _WIN32
#include "candidate_deadline_windows.h"
#include "debug_root_windows.h"
#include "argument_root_windows.h"
#include "argument_reader_cancel_windows.h"
#include <stdint.h>
static int away(const void *a,SIZE_T n,const void *b,SIZE_T m){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;return x&&y&&n&&m&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static int fresh(const void *p,SIZE_T n){const unsigned char *b=p;for(SIZE_T i=0;i<n;i++)if(b[i])return 0;return 1;}
static SIZE_T owner_size(unsigned k){
 switch(k){
 case SL_WIN_DEADLINE_STARTUP:return sizeof(sl_win_public_startup);
 case SL_WIN_DEADLINE_DEBUG_EVENT:return sizeof(sl_win_debug_event_observation);
 case SL_WIN_DEADLINE_MAPPED_ROOT:return sizeof(sl_win_mapped_root_observation);
 case SL_WIN_DEADLINE_ROOT_WRITE:return sizeof(sl_win_root_write);
 case SL_WIN_DEADLINE_DEBUG_RELEASE:return sizeof(sl_win_debug_release);
 case SL_WIN_DEADLINE_ARGUMENT_ROOT:return sizeof(sl_win_argument_root_source);
 case SL_WIN_DEADLINE_READER_CANCEL:return sizeof(sl_win_argument_reader_cancel);
 case SL_WIN_DEADLINE_DUPLEX_COMPLETION:return sizeof(sl_win_duplex_completion);
 case SL_WIN_DEADLINE_DUPLEX_SETTLEMENT:return sizeof(sl_win_duplex_settlement);
 case SL_WIN_DEADLINE_MODULE_QUERY:return sizeof(sl_win_module_query);
 case SL_WIN_DEADLINE_SERVICE:return sizeof(sl_win_service_observation);
 case SL_WIN_DEADLINE_SEND_TURN:return sizeof(sl_win_argument_send_turn);
 default:return 0;
 }
}
static int row(const sl_win_candidate_deadline_row *r,SIZE_T ordinal){
 ULONG_PTR begin=(ULONG_PTR)r->owner_original,call=(ULONG_PTR)r->native_row_original;
 return r->ordinal==ordinal&&r->owner_extent==owner_size(r->kind)&&r->owner_extent&&begin&&call&&
 r->owner_extent<=UINTPTR_MAX-begin&&call>=begin&&call<=begin+r->owner_extent&&
 sizeof(sl_win_call)<=begin+r->owner_extent-call&&call%_Alignof(sl_win_call)==0;
}
static int row_equal(const sl_win_candidate_deadline_row *a,const sl_win_candidate_deadline_row *b){
 return a->native_row_original==b->native_row_original&&a->owner_original==b->owner_original&&a->owner_extent==b->owner_extent&&
 a->kind==b->kind&&a->occurrence==b->occurrence&&a->ordinal==b->ordinal;
}
static int source(const sl_win_candidate_deadline_source *s,sl_win_candidate_deadline_step **steps){
 if(!s||!s->preparation_original||!s->caller_original||!s->rows_original||!s->row_count||s->row_count>SIZE_MAX/sizeof(*s->rows_original))return 0;
 const sl_win_candidate_preparation *p=s->preparation_original;
 if(!p->attempted||!p->returned||!p->result||!sl_win_candidate_layout_matches_original(p->plan_original)||
    !p->backing_original||p->backing_extent!=p->plan_original->bytes||p->backing_extent>UINTPTR_MAX-(ULONG_PTR)p->backing_original||
    (ULONG_PTR)p->backing_original%p->plan_original->alignment||
    p->startup_original!=(void *)((unsigned char *)p->backing_original+p->plan_original->copies_original[SL_WIN_LAYOUT_STARTUP].offset)||
    p->factory_original!=(void *)((unsigned char *)p->backing_original+p->plan_original->copies_original[SL_WIN_LAYOUT_FACTORY_CONTROL].offset))return 0;
 const sl_win_candidate_layout_item *v=&p->plan_original->copies_original[SL_WIN_LAYOUT_DEADLINE_CUSTODY];
 if(v->count!=s->row_count||v->element_extent!=sizeof(**steps)||v->alignment<_Alignof(sl_win_candidate_deadline_step)||
    v->offset%_Alignof(sl_win_candidate_deadline_step))return 0;
 *steps=(void *)((unsigned char *)p->backing_original+v->offset);
 return (ULONG_PTR)*steps%_Alignof(sl_win_candidate_deadline_step)==0;
}
static int caller(const struct slcli_product_windows_operation_caller *c){
 return c&&c->initial_source_original&&c->arena_original&&c->memory_original&&c->arena_original->owner==c->memory_original&&
 c->deadline_original&&slcli_source_deadline_original_matches(c->arena_original,c->deadline_original,c)&&
 slcli_native_clock_windows_matches_original(c->deadline_original->origin);
}
static int live(const sl_win_candidate_deadline_source *s){
 struct slcli_arena *a=s->caller_original->arena_original;
 const sl_win_candidate_preparation *p=s->preparation_original;
 if(!slcli_arena_live_block(a,p->backing_original,p->backing_extent))return 0;
 for(SIZE_T i=0;i<s->row_count;i++)if(!slcli_arena_live_range(a,s->rows_original[i].owner_original,s->rows_original[i].owner_extent))return 0;
 return 1;
}
static int initial_away(const struct slcli_product_storage_windows_source *s,const void *out,SIZE_T n){
 if(!s||!s->region_count||s->region_count>SIZE_MAX/sizeof(*s->regions_original)||!s->source_graph_count||
    s->source_graph_count>SIZE_MAX/sizeof(*s->source_graph_original)||
    !away(out,n,s,sizeof(*s))||!away(out,n,s->initial_origin_original,sizeof(*s->initial_origin_original))||
    !away(out,n,s->initial_prior_original,sizeof(*s->initial_prior_original))||
    !away(out,n,s->regions_original,s->region_count*sizeof(*s->regions_original))||
    !away(out,n,s->profile_original,sizeof(*s->profile_original))||
    !away(out,n,s->effective_original,sizeof(*s->effective_original))||!away(out,n,s->primary_original,sizeof(*s->primary_original))||
    !away(out,n,s->initial_source_original,sizeof(*s->initial_source_original))||
    !away(out,n,s->source_graph_original,s->source_graph_count*sizeof(*s->source_graph_original)))return 0;
 for(SIZE_T i=0;i<s->source_graph_count;i++){
  const struct slcli_product_initial_windows_source_span *v=&s->source_graph_original[i];
  if(!away(out,n,v->original,v->bytes)||!away(out,n,v->actor_original,v->actor_bytes)||
     !away(out,n,v->occurrence_original,v->occurrence_bytes))return 0;
 }return 1;
}
static int excludes(const sl_win_candidate_deadline_source *s,const void *out,SIZE_T n){
 const struct slcli_product_windows_operation_caller *c=s->caller_original;
 if(!away(out,n,s,sizeof(*s))||!away(out,n,s->rows_original,s->row_count*sizeof(*s->rows_original))||
    !away(out,n,s->preparation_original,sizeof(*s->preparation_original))||
    !sl_win_candidate_layout_excludes_output(s->preparation_original->plan_original,out,n)||
    !away(out,n,c,sizeof(*c))||!initial_away(c->initial_source_original,out,n)||
    !away(out,n,c->arena_original,sizeof(*c->arena_original))||!away(out,n,c->memory_original,sizeof(*c->memory_original))||
    !slcli_source_deadline_away(c->deadline_original,out,n))return 0;
 for(SIZE_T i=0;i<s->row_count;i++){
  if(!row(&s->rows_original[i],i)||!away(out,n,s->rows_original[i].owner_original,s->rows_original[i].owner_extent))return 0;
 }return 1;
}
int sl_win_bind_prepared_candidate_deadline(sl_win_candidate_deadline_sequence *q){
 sl_win_candidate_deadline_step *steps;
 if(!q||q->attempted||q->returned||!q->source_original||!source(q->source_original,&steps)||!caller(q->source_original->caller_original)||!live(q->source_original))return 0;
 const sl_win_candidate_deadline_source *s=q->source_original;const struct slcli_product_windows_operation_caller *c=s->caller_original;
 if(s->row_count>SIZE_MAX/sizeof(*steps))return 0;
 SIZE_T n=s->row_count*sizeof(*steps);
 if(!excludes(s,q,sizeof(*q))||!excludes(s,steps,n)||!away(q,sizeof(*q),steps,n)||
    !away(q,sizeof(*q),s->preparation_original->backing_original,s->preparation_original->backing_extent)||
    !slcli_arena_live_range(c->arena_original,q,sizeof(*q))||
    !slcli_arena_live_range(c->arena_original,c,sizeof(*c))||!slcli_arena_live_range(c->arena_original,steps,n)||
    !fresh(steps,n)||q->actual_source||!fresh(&q->source_snapshot,sizeof(q->source_snapshot))||q->arena_original||q->memory_original||q->initial_source_original||
    q->deadline_original||q->origin_original||q->origin_tick||q->expires||q->steps_original||q->capacity||q->result||
    !fresh(q->origin_raw,sizeof(q->origin_raw))||!fresh(q->origin_status,sizeof(q->origin_status))||!fresh(q->origin_args,sizeof(q->origin_args)))return 0;
 for(SIZE_T i=0;i<s->row_count;i++){
  if(!fresh(s->rows_original[i].native_row_original,sizeof(sl_win_call)))return 0;
  for(SIZE_T j=0;j<i;j++)if(s->rows_original[j].native_row_original==s->rows_original[i].native_row_original||
    (s->rows_original[j].kind==s->rows_original[i].kind&&s->rows_original[j].occurrence==s->rows_original[i].occurrence))return 0;
 }
 q->attempted=1;q->actual_source=s;q->source_snapshot=*s;q->arena_original=c->arena_original;q->memory_original=c->memory_original;
 q->initial_source_original=c->initial_source_original;
 q->deadline_original=c->deadline_original;q->origin_original=c->deadline_original->origin;q->origin_tick=q->origin_original->tick;q->expires=c->deadline_original->expires;
 for(unsigned i=0;i<2;i++){q->origin_raw[i]=q->origin_original->raw[i];q->origin_status[i]=q->origin_original->status[i];q->origin_args[i]=q->origin_original->args[i];}
 q->steps_original=steps;q->capacity=s->row_count;
 for(SIZE_T i=0;i<q->capacity;i++){
  steps[i].sequence_original=q;steps[i].source_original=&s->rows_original[i];steps[i].source_snapshot=s->rows_original[i];
  steps[i].deadline_original=q->deadline_original;steps[i].caller_original=c;
 }
 q->returned=1;q->result=1;return 1;
}
int sl_win_candidate_deadline_matches_original(const sl_win_candidate_deadline_sequence *q){
 sl_win_candidate_deadline_step *steps;
 if(!q||!q->attempted||!q->returned||!q->result||q->source_original!=q->actual_source||!q->source_original)return 0;
 const sl_win_candidate_deadline_source *s=q->source_original;
 if(s->preparation_original!=q->source_snapshot.preparation_original||s->caller_original!=q->source_snapshot.caller_original||
    s->rows_original!=q->source_snapshot.rows_original||s->row_count!=q->source_snapshot.row_count||!source(s,&steps)||!caller(s->caller_original)||!live(s))return 0;
 const struct slcli_product_windows_operation_caller *c=s->caller_original;
 if(c->arena_original!=q->arena_original||c->memory_original!=q->memory_original||c->initial_source_original!=q->initial_source_original||c->deadline_original!=q->deadline_original||
    q->deadline_original->origin!=q->origin_original||q->deadline_original->expires!=q->expires||q->origin_original->tick!=q->origin_tick||
    steps!=q->steps_original||s->row_count!=q->capacity||!excludes(s,q,sizeof(*q))||!excludes(s,steps,q->capacity*sizeof(*steps))||
    !away(q,sizeof(*q),s->preparation_original->backing_original,s->preparation_original->backing_extent)||
    !slcli_arena_live_range(c->arena_original,q,sizeof(*q))||!slcli_arena_live_range(c->arena_original,c,sizeof(*c))||
    !slcli_arena_live_range(c->arena_original,steps,q->capacity*sizeof(*steps)))return 0;
 for(unsigned i=0;i<2;i++)if(q->origin_raw[i]!=q->origin_original->raw[i]||q->origin_status[i]!=q->origin_original->status[i]||q->origin_args[i]!=q->origin_original->args[i])return 0;
 for(SIZE_T i=0;i<q->capacity;i++){
  if(steps[i].sequence_original!=q||steps[i].source_original!=&s->rows_original[i]||!row_equal(&steps[i].source_snapshot,&s->rows_original[i])||
     steps[i].deadline_original!=q->deadline_original||steps[i].caller_original!=c)return 0;
 }
 return 1; /* Not clock validity of later cuts, dispatch permission, or closure. */
}
#endif
