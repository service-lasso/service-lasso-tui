#ifndef SERVICE_LASSO_DEADLINE_LINUX_H
#define SERVICE_LASSO_DEADLINE_LINUX_H
#include "owner_linux.h"
#include <stddef.h>
#include <time.h>
/* Internal source caller input, not a public deadline/authority credential.
 * The actual enclosing caller must establish source ownership, original policy,
 * containing reservation and operation identity before constructing a root. */
typedef struct sl_linux_deadline_source {
 const void *operation_original;size_t operation_extent;
 unsigned long duration_ns;
} sl_linux_deadline_source;
typedef struct sl_linux_deadline {
 const sl_linux_deadline_source *source_original;
 const void *operation_original;size_t operation_extent;unsigned long duration_ns;
 sl_linux_call clock;struct timespec started,end;
 long raw_clock,result;unsigned attempted,returned;
 struct sl_linux_deadline_sequence *sequence_original;
} sl_linux_deadline;
typedef struct sl_linux_deadline_observation {
 const sl_linux_deadline *deadline_original,*actual_deadline;
 sl_linux_call clock;struct timespec observed,remaining;
 long raw_clock,result;unsigned attempted,returned,expired;
} sl_linux_deadline_observation;
typedef struct sl_linux_deadline_sequence {
 sl_linux_deadline *root_original;
 sl_linux_deadline_observation *observations_original;
 size_t capacity;
 sl_linux_deadline *actual_root;
 sl_linux_deadline_observation *actual_observations,*first_failure;
 size_t actual_capacity,next;
 long result;unsigned attempted,prepared,failed,expired,exhausted;
} sl_linux_deadline_sequence;
_Static_assert(sizeof(sl_linux_call)==80,"native deadline call original");
_Static_assert(sizeof(sl_linux_deadline_source)==24,"native deadline source extent");
_Static_assert(sizeof(sl_linux_deadline)==176,"native original deadline extent with once enrolled sequence");
_Static_assert(offsetof(sl_linux_deadline,clock)==32&&offsetof(sl_linux_deadline,started)==112&&offsetof(sl_linux_deadline,end)==128,"native deadline output offsets");
_Static_assert(sizeof(sl_linux_deadline_observation)==160,"native deadline observation extent");
_Static_assert(sizeof(sl_linux_deadline_sequence)==96&&offsetof(sl_linux_deadline_sequence,actual_root)==24,"native enrolled deadline roster extent");
_Static_assert(offsetof(sl_linux_deadline_observation,clock)==16&&offsetof(sl_linux_deadline_observation,observed)==96&&offsetof(sl_linux_deadline_observation,remaining)==112,"native deadline observation offsets");
/* Two already-financed rows, consumed by one native-operation caller. */
typedef struct sl_linux_deadline_boundary {
 sl_linux_deadline_sequence *sequence;
 sl_linux_deadline_observation *before,*after;
 size_t first;
 long before_result,after_result;
 unsigned attempted,ready,after_attempted,returned;
} sl_linux_deadline_boundary;
_Static_assert(sizeof(sl_linux_deadline_boundary)==64,"retained two-clock native boundary");
/* Representation/custody only: actual serial operation ownership is external. */
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *sequence,const void *output,size_t extent);
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *original,sl_linux_deadline_sequence *sequence);
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *original);
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *original);
long sl_linux_construct_deadline(sl_linux_deadline *original,const sl_linux_deadline_source *source);
long sl_linux_observe_deadline(sl_linux_deadline_observation *original);
long sl_linux_prepare_deadline_sequence(sl_linux_deadline_sequence *original);
long sl_linux_observe_next_deadline(sl_linux_deadline_sequence *original);
/* result0 remaining; result1 original expiry; negative failure. No native
 * operation/timeout may consume a row until its actual successful return.
 * Each observation is distinct; expiry never releases an unresolved graph. */
#endif
