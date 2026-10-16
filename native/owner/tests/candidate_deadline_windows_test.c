#ifdef _WIN32
/* UNRUN native-source fixture entry, deliberately no standalone main and no
 * fabricated clock/arena/constructor. A separately admitted genuine owner must
 * supply fresh paid q + complete prepared source before this can execute.
 * These assertions qualify binding correspondence only, never that supplier. */
#include "../candidate_deadline_windows.h"
#include "../argument_root_windows.h"
#include <assert.h>
#include <string.h>
static int separate(const void *a,SIZE_T n,const void *b,SIZE_T m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x&&y&&n&&m&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static void no_write_refusal(sl_win_candidate_deadline_sequence *q,sl_win_candidate_deadline_step *steps,
 SIZE_T n,unsigned char *snapshot){
 unsigned char control[sizeof(*q)];memcpy(control,q,sizeof(control));memcpy(snapshot,steps,n);
 assert(!sl_win_bind_prepared_candidate_deadline(q));
 assert(!memcmp(control,q,sizeof(control))&&!memcmp(snapshot,steps,n));
}
static void source_aliases(const struct slcli_product_storage_windows_source *original,const void *out,SIZE_T n){
 /* Actual complete shared predicate, with private metadata copies only. This
  * never mutates the genuine source/caller or constructs native authority. */
 assert(slcli_product_windows_initial_source_excludes_output(original,out,n));
 struct slcli_product_storage_windows_source s=*original;
 struct slcli_bq_provider_expectations e=*original->initial_source_original;
 s.initial_source_original=&e;
 e.requirements_per_command=out;assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 e=*original->initial_source_original;e.subjects=out;assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 e=*original->initial_source_original;e.principal.windows_sid=out;e.principal.windows_sid_bytes=1;
 assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 e=*original->initial_source_original;e.producer.entry_symbol=out;e.producer.entry_symbol_bytes=1;
 assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 for(SIZE_T i=0;i<original->initial_source_original->subject_count;i++){
  e=*original->initial_source_original;struct slcli_bq_expected_subject subject=e.subjects[i];
  subject.reader_role=out;subject.reader_role_bytes=1;e.subjects=&subject;e.subject_count=1;
  assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 }
 e=*original->initial_source_original;
 struct slcli_bq_expected_principal principal=*original->effective_original;
 principal.windows_sid=out;principal.windows_sid_bytes=1;s.effective_original=&principal;
 assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 s=*original;principal=*original->primary_original;principal.windows_sid=out;principal.windows_sid_bytes=1;s.primary_original=&principal;
 assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 for(SIZE_T i=0;i<original->source_graph_count;i++)for(unsigned part=0;part<3;part++){
  s=*original;struct slcli_product_initial_windows_source_span graph=original->source_graph_original[i];
  if(!part){graph.original=out;graph.bytes=1;}else if(part==1){graph.actor_original=out;graph.actor_bytes=1;}else{graph.occurrence_original=out;graph.occurrence_bytes=1;}
  s.source_graph_original=&graph;s.source_graph_count=1;
  assert(!slcli_product_windows_initial_source_excludes_output(&s,out,n));
 }
 /* Existing source pointees are themselves forbidden output spans. */
 assert(!slcli_product_windows_initial_source_excludes_output(original,original,sizeof(*original)));
 assert(!slcli_product_windows_initial_source_excludes_output(original,original->initial_source_original->requirements_per_command,1));
 assert(!slcli_product_windows_initial_source_excludes_output(original,original->initial_source_original->subjects,1));
 assert(!slcli_product_windows_initial_source_excludes_output(original,original->initial_source_original->principal.windows_sid,1));
 assert(!slcli_product_windows_initial_source_excludes_output(original,original->effective_original->windows_sid,1));
 assert(!slcli_product_windows_initial_source_excludes_output(original,original->primary_original->windows_sid,1));
 if(original->initial_source_original->producer.entry_symbol_bytes)
  assert(!slcli_product_windows_initial_source_excludes_output(original,original->initial_source_original->producer.entry_symbol,1));
 for(SIZE_T i=0;i<original->initial_source_original->subject_count;i++)if(original->initial_source_original->subjects[i].reader_role_bytes)
  assert(!slcli_product_windows_initial_source_excludes_output(original,original->initial_source_original->subjects[i].reader_role,1));
}
int sl_win_candidate_deadline_source_fixture(sl_win_candidate_deadline_sequence *q,
 sl_win_candidate_deadline_row *scratch,SIZE_T scratch_capacity,unsigned char *snapshot,SIZE_T snapshot_bytes){
 assert(q&&q->source_original&&!q->attempted);
 const sl_win_candidate_deadline_source *s=q->source_original;
 const sl_win_candidate_preparation *p=s->preparation_original;
 const sl_win_candidate_layout_item *v=&p->plan_original->copies_original[SL_WIN_LAYOUT_DEADLINE_CUSTODY];
 assert(v->element_extent==sizeof(sl_win_candidate_deadline_step)&&v->count==s->row_count&&v->count>1);
 assert(scratch&&scratch_capacity==s->row_count&&scratch_capacity<=SIZE_MAX/sizeof(*scratch));
 unsigned kinds=0;
 for(SIZE_T i=0;i<s->row_count;i++){assert(s->rows_original[i].kind<SL_WIN_DEADLINE_ROW_KIND_COUNT);kinds|=1u<<s->rows_original[i].kind;}
 assert(kinds==(1u<<SL_WIN_DEADLINE_ROW_KIND_COUNT)-1); /* Whole supported-kind fixture coverage. */
 sl_win_candidate_deadline_step *steps=(void *)((unsigned char *)p->backing_original+v->offset);
 SIZE_T step_bytes=s->row_count*sizeof(*steps);assert(snapshot&&snapshot_bytes==step_bytes);
 SIZE_T row_bytes=scratch_capacity*sizeof(*scratch);
 assert(separate(scratch,row_bytes,p->backing_original,p->backing_extent)&&separate(snapshot,step_bytes,p->backing_original,p->backing_extent));
 assert(separate(scratch,row_bytes,q,sizeof(*q))&&separate(snapshot,step_bytes,q,sizeof(*q))&&separate(scratch,row_bytes,snapshot,step_bytes));
 assert(separate(scratch,row_bytes,s,sizeof(*s))&&separate(snapshot,step_bytes,s,sizeof(*s)));
 assert(separate(scratch,row_bytes,s->rows_original,row_bytes)&&separate(snapshot,step_bytes,s->rows_original,row_bytes));
 assert(sl_win_candidate_layout_excludes_output(p->plan_original,scratch,row_bytes)&&sl_win_candidate_layout_excludes_output(p->plan_original,snapshot,step_bytes));
 assert(slcli_product_windows_initial_source_excludes_output(s->caller_original->initial_source_original,scratch,row_bytes));
 assert(slcli_product_windows_initial_source_excludes_output(s->caller_original->initial_source_original,snapshot,step_bytes));
 source_aliases(s->caller_original->initial_source_original,q,sizeof(*q));
 source_aliases(s->caller_original->initial_source_original,steps,step_bytes);
 sl_win_candidate_deadline_source foreign_source=*s;
 struct slcli_product_windows_operation_caller foreign_caller=*s->caller_original;
 foreign_source.caller_original=&foreign_caller;q->source_original=&foreign_source;
 no_write_refusal(q,steps,step_bytes,snapshot);
 foreign_source=*s;foreign_source.row_count--;q->source_original=&foreign_source;
 no_write_refusal(q,steps,step_bytes,snapshot);
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));
 foreign_source=*s;foreign_source.rows_original=scratch;q->source_original=&foreign_source;
 scratch[1]=scratch[0];scratch[1].ordinal=1;
 no_write_refusal(q,steps,step_bytes,snapshot);
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[0].owner_extent--;
 no_write_refusal(q,steps,step_bytes,snapshot);
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[1].ordinal=0;
 no_write_refusal(q,steps,step_bytes,snapshot);
 /* Every supplied genuine owner kind is adversarially retested, without
  * changing any native object, source original, or original clock. */
 for(SIZE_T i=0;i<s->row_count;i++){
  memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));
  scratch[i].native_row_original=(sl_win_call *)scratch[i].owner_original; /* aligned nonmember */
  no_write_refusal(q,steps,step_bytes,snapshot);
  memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));
  scratch[i].member=(scratch[i].member+1)%SL_WIN_DCALL_MEMBER_COUNT;
  no_write_refusal(q,steps,step_bytes,snapshot);
  memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[i].slot=SIZE_MAX;
  no_write_refusal(q,steps,step_bytes,snapshot);
  memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[i].occurrence=SIZE_MAX;
  no_write_refusal(q,steps,step_bytes,snapshot);
  memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[i].owner_original=s->caller_original;
  no_write_refusal(q,steps,step_bytes,snapshot);
 }
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[0].kind=SL_WIN_DEADLINE_ROW_KIND_COUNT;
 scratch[0].owner_original=(unsigned char *)p->backing_original+p->plan_original->copies_original[SL_WIN_LAYOUT_ARGUMENT_ROOTS].offset;
 scratch[0].owner_extent=sizeof(sl_win_argument_root_source);scratch[0].native_row_original=(sl_win_call *)scratch[0].owner_original;
 no_write_refusal(q,steps,step_bytes,snapshot); /* No ARGUMENT_ROOT call kind exists. */
 q->source_original=s;
 /* This is a dirty UNOBSERVED output cell, not resetting an actual clock. */
 steps[0].before.started=1;
 no_write_refusal(q,steps,step_bytes,snapshot);
 steps[0].before.started=0;
 assert(sl_win_bind_prepared_candidate_deadline(q));
 assert(sl_win_candidate_deadline_matches_original(q));
 assert(!sl_win_bind_prepared_candidate_deadline(q));
 sl_win_candidate_deadline_sequence copied_control=*q;
 assert(!sl_win_candidate_deadline_matches_original(&copied_control));
 q->source_original=&foreign_source;assert(!sl_win_candidate_deadline_matches_original(q));q->source_original=s;
 struct slcli_source_deadline_original equal_expiry=*q->deadline_original;
 const struct slcli_source_deadline_original *original_deadline=q->deadline_original;
 q->deadline_original=&equal_expiry;assert(!sl_win_candidate_deadline_matches_original(q));q->deadline_original=original_deadline;
 struct slcli_native_clock equal_tick=*q->origin_original;
 const struct slcli_native_clock *original_origin=q->origin_original;
 q->origin_original=&equal_tick;assert(!sl_win_candidate_deadline_matches_original(q));q->origin_original=original_origin;
 q->origin_raw[0]^=1;assert(!sl_win_candidate_deadline_matches_original(q));q->origin_raw[0]^=1;
 q->expires--;assert(!sl_win_candidate_deadline_matches_original(q));q->expires++;
 steps[0].source_snapshot.ordinal++;assert(!sl_win_candidate_deadline_matches_original(q));steps[0].source_snapshot.ordinal--;
 assert(sl_win_candidate_deadline_matches_original(q));
 for(SIZE_T i=0;i<q->capacity;i++){
  struct slcli_native_clock blank;memset(&blank,0,sizeof(blank));
  assert(!memcmp(&steps[i].before,&blank,sizeof(blank))&&!memcmp(&steps[i].after,&blank,sizeof(blank)));
 }
 return 1;
}
#endif
