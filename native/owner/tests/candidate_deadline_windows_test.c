#ifdef _WIN32
/* UNRUN native-source fixture entry, deliberately no standalone main and no
 * fabricated clock/arena/constructor. A separately admitted genuine owner must
 * supply fresh paid q + complete prepared source before this can execute.
 * These assertions qualify binding correspondence only, never that supplier. */
#include "../candidate_deadline_windows.h"
#include <assert.h>
#include <string.h>
int sl_win_candidate_deadline_source_fixture(sl_win_candidate_deadline_sequence *q,
 sl_win_candidate_deadline_row *scratch,SIZE_T scratch_capacity){
 assert(q&&q->source_original&&!q->attempted);
 const sl_win_candidate_deadline_source *s=q->source_original;
 const sl_win_candidate_preparation *p=s->preparation_original;
 const sl_win_candidate_layout_item *v=&p->plan_original->copies_original[SL_WIN_LAYOUT_DEADLINE_CUSTODY];
 assert(v->element_extent==sizeof(sl_win_candidate_deadline_step)&&v->count==s->row_count&&v->count>1);
 assert(scratch&&scratch_capacity==s->row_count&&scratch_capacity<=SIZE_MAX/sizeof(*scratch));
 sl_win_candidate_deadline_step *steps=(void *)((unsigned char *)p->backing_original+v->offset);
 sl_win_candidate_deadline_source foreign_source=*s;
 struct slcli_product_windows_operation_caller foreign_caller=*s->caller_original;
 foreign_source.caller_original=&foreign_caller;q->source_original=&foreign_source;
 assert(!sl_win_bind_prepared_candidate_deadline(q)&&!q->attempted);
 foreign_source=*s;foreign_source.row_count--;q->source_original=&foreign_source;
 assert(!sl_win_bind_prepared_candidate_deadline(q)&&!q->attempted);
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));
 foreign_source=*s;foreign_source.rows_original=scratch;q->source_original=&foreign_source;
 scratch[1]=scratch[0];scratch[1].ordinal=1;
 assert(!sl_win_bind_prepared_candidate_deadline(q)&&!q->attempted);
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[0].owner_extent--;
 assert(!sl_win_bind_prepared_candidate_deadline(q)&&!q->attempted);
 memcpy(scratch,s->rows_original,scratch_capacity*sizeof(*scratch));scratch[1].ordinal=0;
 assert(!sl_win_bind_prepared_candidate_deadline(q)&&!q->attempted);
 q->source_original=s;
 /* This is a dirty UNOBSERVED output cell, not resetting an actual clock. */
 steps[0].before.started=1;
 assert(!sl_win_bind_prepared_candidate_deadline(q)&&!q->attempted);
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
