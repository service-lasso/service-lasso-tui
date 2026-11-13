#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_archive_precreation(sl_linux_public_startup *s){
 if(!s||s->archive_precreation_attempted)return -EINVAL;
 s->archive_precreation_attempted=1;s->archive_precreation_result=-EINVAL;
 unsigned long p=s->actual_parse_capacity;
 if(s->archive_files.attempted||!s->archive_directory.returned||s->archive_directory.result||!p||p>ULONG_MAX/SL_ARCHIVE_PURPOSE_COUNT||!s->actual_parses||!s->archive_contexts||s->archive_context_capacity!=3*p||!s->archive_accesses||s->archive_access_capacity!=SL_ARCHIVE_PURPOSE_COUNT*p||!s->actual_contexts||s->actual_context_capacity!=p||!s->actual_candidate_roles||s->actual_candidate_role_capacity!=p||!s->actual_candidate_payloads||s->actual_candidate_payload_capacity!=p||!s->mounts.selinux_count||s->mounts.selinux_count>p)goto done;
 /* Every prepared root, including unused capacity, is checked before native
  * label acquisition or parser/policy consumption. No new roots are enrolled. */
 for(unsigned long i=0;i<3*p;i++){
  sl_linux_selinux_context *o=&s->archive_contexts[i];
  if(o->attempted||o->archive_attempted||o->archive_original!=&s->archive_labels[i/p]||o->parse_original!=&s->actual_parses[i%p])goto done;
 }
 for(unsigned long i=0;i<SL_ARCHIVE_PURPOSE_COUNT*p;i++){
  sl_linux_archive_access *o=&s->archive_accesses[i];unsigned long purpose=i/p,policy=i%p,kind=purpose<6?0:purpose<12?1:2;
  if(o->attempted||o->startup_original!=s||o->purpose!=purpose||o->policy_index!=policy||o->context_original!=&s->archive_contexts[kind*p+policy]||o->owner_original!=&s->actual_contexts[policy]||o->roles_original!=&s->actual_candidate_roles[policy]||o->payload_original!=&s->actual_candidate_payloads[policy].reachable)goto done;
  if(purpose>=SL_ARCHIVE_CREATION_PURPOSE_FIRST&&(!s->archive_creations||s->archive_creation_capacity!=p||o->creation_original!=&s->archive_creations[policy]))goto done;
 }
 s->actual_archive_contexts=s->archive_contexts;s->actual_archive_context_capacity=s->archive_context_capacity;
 if(!s->archive_creations||s->archive_creation_capacity!=p||!s->archive_fscreate_contexts||s->archive_fscreate_context_capacity!=p)goto done;
 for(unsigned long policy=0;policy<p;policy++){
  sl_linux_archive_creation *o=&s->archive_creations[policy];sl_linux_selinux_context *f=&s->archive_fscreate_contexts[policy];
  if(o->attempted||o->startup_original!=s||o->policy_index!=policy||o->directory_original!=&s->archive_contexts[2*p+policy]||o->owner_original!=&s->actual_contexts[policy]||o->fscreate_original!=f||f->attempted||f->self_attempted||f->parse_original!=&s->actual_parses[policy])goto done;
 }
 s->archive_fscreate_result=sl_linux_observe_selinux_fscreate(&s->archive_fscreate);s->archive_precreation_result=s->archive_fscreate_result;if(s->archive_precreation_result)goto done;
 s->archive_precreation_result=sl_linux_observe_archive_directory_before_creation(s);if(s->archive_precreation_result)goto done;
 for(unsigned long policy=0;policy<s->mounts.selinux_count;policy++){
  s->archive_precreation_result=sl_linux_interpret_archive_context(&s->actual_archive_contexts[2*p+policy]);if(s->archive_precreation_result)goto done;
  if(s->archive_fscreate.extent){s->archive_precreation_result=sl_linux_interpret_selinux_self(&s->archive_fscreate_contexts[policy],&s->archive_fscreate);if(s->archive_precreation_result)goto done;}
  s->archive_precreation_result=sl_linux_interpret_archive_creation(&s->archive_creations[policy]);if(s->archive_precreation_result)goto done;
 }
 for(unsigned long purpose=12;purpose<SL_ARCHIVE_PURPOSE_COUNT;purpose++)for(unsigned long policy=0;policy<s->mounts.selinux_count;policy++){
  s->archive_precreation_result=sl_linux_interpret_archive_access(&s->archive_accesses[purpose*p+policy]);if(s->archive_precreation_result)goto done;
 }
 s->archive_precreation_result=0;
 done:s->archive_precreation_returned=1;return s->archive_precreation_result;
 /* This phase supplies precreation observations only. Actual initial ROOT,
  * creation label/transition and continuing exclusion still govern effects. */
}
