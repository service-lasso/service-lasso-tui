#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_archive_contexts(sl_linux_public_startup *s){
 if(!s||s->archive_contexts_attempted)return -EINVAL;
 s->archive_contexts_attempted=1;s->archive_contexts_result=-EINVAL;
 s->actual_archive_contexts=s->archive_contexts;s->actual_archive_context_capacity=s->archive_context_capacity;
 if(!s->archive_labels_returned||s->archive_labels_result||!s->actual_parses||!s->actual_parse_capacity||s->actual_parse_capacity>ULONG_MAX/3||!s->actual_archive_contexts||s->actual_archive_context_capacity!=3*s->actual_parse_capacity||!s->mounts.selinux_count||s->mounts.selinux_count>s->actual_parse_capacity)goto done;
 for(unsigned long i=0;i<s->actual_archive_context_capacity;i++){
  sl_linux_selinux_context *context=&s->actual_archive_contexts[i];
  if(((context->attempted||context->archive_attempted)&&!(i/s->actual_parse_capacity==2&&i%s->actual_parse_capacity<s->mounts.selinux_count&&s->archive_precreation_returned&&!s->archive_precreation_result&&context->archive_returned&&!context->archive_result&&context->returned&&!context->result&&!context->status_result))||context->archive_original!=&s->archive_labels[i/s->actual_parse_capacity]||context->parse_original!=&s->actual_parses[i%s->actual_parse_capacity])goto done;
 }
 for(unsigned kind=0;kind<3;kind++)for(unsigned long policy=0;policy<s->mounts.selinux_count;policy++){
  unsigned long index=kind*s->actual_parse_capacity+policy;s->archive_context_count=index+1;
  s->archive_contexts_result=s->actual_archive_contexts[index].archive_attempted?s->actual_archive_contexts[index].archive_result:sl_linux_interpret_archive_context(&s->actual_archive_contexts[index]);if(s->archive_contexts_result)goto done;
  if(kind<2){
   if(!s->archive_creations||s->archive_creation_capacity!=s->actual_parse_capacity){s->archive_contexts_result=-EINVAL;goto done;}
   sl_linux_archive_creation *creation=&s->archive_creations[policy];sl_linux_selinux_context *context=&s->actual_archive_contexts[index];
   if(!creation->returned||creation->result||creation->status_result||!creation->types.returned||creation->types.result||creation->types.custody.actual_policy!=&context->actual_parse->parser.policy||!context->type||context->type>creation->types.actual_capacity||!creation->types.actual_types||!creation->types.actual_referents||!creation->types.actual_types[context->type-1]||creation->types.actual_referents[context->type-1]!=context->actual_parse->parser.policy.type_val_to_struct[context->type-1]||!sl_linux_equal_selinux_status(&creation->after_creation,&context->after_context)){s->archive_contexts_result=-EINVAL;goto done;}
  }
 }
 s->archive_contexts_result=0;
 done:s->archive_contexts_returned=1;return s->archive_contexts_result;
 /* Complete fixed roster remains owned after any partial context failure. */
}
