#ifdef _WIN32
#include "../query_profile_windows.h"
#include <assert.h>

/* Called only by a future admitted genuine-input harness. This file creates
 * no principal, held-file, source, nativecase, clock, arena or reservation.
 * All controls/copies/scratch are separately owned/excluded before invocation.
 * These vectors have NOT been compiled or executed by the author. */
static void clone_bytes(void *out,const void *in,size_t n){unsigned char *a=out;const unsigned char *b=in;for(size_t i=0;i<n;i++)a[i]=b[i];}
static int equal_bytes(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return 0;return 1;}

void sl_win_query_profile_source_refusal_vectors(
 const sl_win_query_profile_source *genuine,
 sl_win_query_semantic_row copies[SL_WIN_QUERY_ROLE_COUNT],
 sl_win_query_derivation *fresh,unsigned char *before,size_t before_extent){
 sl_win_query_profile_source changed;
 assert(genuine&&genuine->row_count==SL_WIN_QUERY_ROLE_COUNT&&copies&&fresh&&before);
 assert(before_extent==sizeof(*fresh));
 assert(sl_win_query_source_excludes_output(genuine,copies,sizeof(*copies)*SL_WIN_QUERY_ROLE_COUNT));
 assert(sl_win_query_source_excludes_output(genuine,&changed,sizeof(changed)));
 assert(sl_win_query_source_excludes_output(genuine,before,before_extent));
 clone_bytes(&changed,genuine,sizeof(changed));
 clone_bytes(copies,genuine->rows_original,sizeof(*copies)*SL_WIN_QUERY_ROLE_COUNT);
 changed.rows_original=copies;
 const sl_win_query_profile_source *original_source=fresh->source_original;
 fresh->source_original=&changed;
 /* A wrong role/arm is refused before hash/output or any native occurrence. */
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++){
  unsigned old=copies[i].arm;copies[i].arm=3;
  clone_bytes(before,fresh,sizeof(*fresh));
  assert(!sl_win_derive_query_profile(fresh));assert(equal_bytes(before,fresh,sizeof(*fresh)));
  copies[i].arm=old;
 }
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++){
  unsigned old=copies[i].role;copies[i].role=(unsigned)((i+1)%SL_WIN_QUERY_ROLE_COUNT);
  clone_bytes(before,fresh,sizeof(*fresh));
  assert(!sl_win_derive_query_profile(fresh));assert(equal_bytes(before,fresh,sizeof(*fresh)));
  copies[i].role=old;
 }
 changed.row_count=SL_WIN_QUERY_ROLE_COUNT-1;
 clone_bytes(before,fresh,sizeof(*fresh));
 assert(!sl_win_derive_query_profile(fresh));assert(equal_bytes(before,fresh,sizeof(*fresh)));
 changed.row_count=SL_WIN_QUERY_ROLE_COUNT;
 /* Exact declared transitive source byte/actor/occurrence spans are all
  * forbidden outputs, even when they alias a larger source-selected range. */
 for(size_t i=0;i<genuine->row_count;i++)for(size_t j=0;j<genuine->rows_original[i].input_count;j++){
  const struct slcli_product_initial_windows_source_span *s=&genuine->rows_original[i].inputs_original[j];
  assert(!sl_win_query_source_excludes_output(genuine,s,sizeof(*s)));
  assert(!sl_win_query_source_excludes_output(genuine,s->original,s->bytes));
  assert(!sl_win_query_source_excludes_output(genuine,s->actor_original,s->actor_bytes));
  assert(!sl_win_query_source_excludes_output(genuine,s->occurrence_original,s->occurrence_bytes));
 }
 fresh->source_original=original_source;
}

void sl_win_query_profile_genuine_source_vectors(
 sl_win_query_derivation *genuine_fresh,sl_win_query_profile_encoding *encoding,
 const uint64_t expected_dimensions[SL_WIN_QUERY_ROLE_COUNT],
 const unsigned char *expected_profile,size_t expected_profile_extent){
 assert(genuine_fresh&&encoding&&expected_dimensions&&expected_profile&&expected_profile_extent);
 assert(sl_win_derive_query_profile(genuine_fresh));
 assert(sl_win_query_derivation_matches_original(genuine_fresh));
 assert(genuine_fresh->dimensions[SL_WIN_QUERY_ENDPOINT_SECURITY]);
 assert(genuine_fresh->dimensions[SL_WIN_QUERY_PROCESS_SECURITY]);
 assert(genuine_fresh->dimensions[SL_WIN_QUERY_TOKEN_ACCESS]);
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++)assert(genuine_fresh->dimensions[i]==expected_dimensions[i]);
 /* A mutable derived quantity or a changed retained source-copy byte cannot
  * stand in for the original semantic/source relation. Only private outputs
  * are changed; the real native/source originals are untouched. */
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++){
  uint64_t old=genuine_fresh->dimensions[i];genuine_fresh->dimensions[i]=old==UINT64_MAX?old-1:old+1;
  assert(!sl_win_query_derivation_matches_original(genuine_fresh));
  genuine_fresh->dimensions[i]=old;assert(sl_win_query_derivation_matches_original(genuine_fresh));
 }
 for(size_t i=0;i<genuine_fresh->snapshot_count;i++){
  sl_win_query_input_snapshot *v=&genuine_fresh->snapshots_original[i];unsigned char old=v->bytes_original[0];
  v->bytes_original[0]^=1;assert(!sl_win_query_derivation_matches_original(genuine_fresh));
  v->bytes_original[0]=old;assert(sl_win_query_derivation_matches_original(genuine_fresh));
 }
 /* MAX across every principal case, never MAX costs or a single example. */
 for(size_t i=0;i<=SL_WIN_QUERY_TOKEN_ACCESS;i++){
  const sl_win_query_principal_geometry *g=&genuine_fresh->source_original->rows_original[i].source.principal;
  uint64_t maximum=0;
  for(size_t j=0;j<g->case_count;j++){
   uint64_t n=i==SL_WIN_QUERY_TOKEN_ACCESS?g->cases_original[j].access_extent:g->cases_original[j].descriptor_extent;
   if(n>maximum)maximum=n;
  }
  assert(maximum==genuine_fresh->dimensions[i]);
 }
 assert(encoding->derivation_original==genuine_fresh);
 assert(sl_win_encode_query_profile(encoding));
 /* Complete externally retained expected source vector, independent of this
  * encoder; no implementation-generated oracle or synthetic native issuer. */
 assert(encoding->output_used==expected_profile_extent);
 assert(equal_bytes(encoding->output_original,expected_profile,expected_profile_extent));
 assert(encoding->output_used&&encoding->output_original[encoding->output_used-1]=='\n');
 assert(sl_win_query_derivation_matches_original(genuine_fresh));
 for(size_t i=0;i<encoding->hash_capacity;i++){
  assert(encoding->hashes_original[i].attempted&&encoding->hashes_original[i].returned);
  assert(!encoding->hashes_original[i].result);
 }
 /* Caller retains all real inputs and output-file custody after these pure
  * vectors; a return here settles no native borrower or family. */
}
#endif
