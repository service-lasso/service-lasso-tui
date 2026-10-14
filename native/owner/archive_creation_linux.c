#include "public_startup_linux.h"
#include <errno.h>
long sl_linux_interpret_archive_creation(sl_linux_archive_creation *o){
 if(!o||o->attempted)return -EINVAL;
 sl_linux_public_startup *s=o->startup_original;
 if(!s||!s->actual_parse_capacity||!s->archive_creations||s->archive_creation_capacity!=s->actual_parse_capacity||o->policy_index>=s->archive_creation_capacity||o!=&s->archive_creations[o->policy_index])return -EINVAL;
 o->attempted=1;o->primary_result=o->result=-EINVAL;
 sl_linux_selinux_context *directory=o->directory_original,*owner=o->owner_original;
 if(!directory||!owner||!directory->returned||directory->result||directory->status_result||!directory->archive_returned||directory->archive_result||!owner->returned||owner->result||owner->status_result||!directory->actual_parse||directory->actual_parse!=owner->actual_parse||!directory->type||!owner->type||!s->archive_fscreate.returned||s->archive_fscreate.result||s->archive_fscreate_result)goto done;
 if(!sl_linux_equal_selinux_status(&directory->after_context,&owner->after_context)||o->after_creation.attempted||o->after_creation.status_original||o->types.custody.policy_original||o->types.custody.source_type||o->types.custody.target_type)goto done;
 o->after_creation.status_original=directory->after_context.status_original;
 o->types.custody.policy_original=&directory->actual_parse->parser.policy;
 o->types.custody.source_type=owner->type;o->types.custody.target_type=directory->type;
 if(s->archive_fscreate.extent){
  sl_linux_selinux_context *context=o->fscreate_original;
  if(!context||!context->returned||context->result||context->status_result||context->actual_parse!=directory->actual_parse||context->actual_self!=&s->archive_fscreate||!context->self_returned||context->self_result||!sl_linux_equal_selinux_status(&directory->after_context,&context->after_context)){o->primary_result=-EINVAL;goto sampled;}
  policydb_t *policy=o->types.custody.policy_original;
  if(!context->type||context->type>policy->p_types.nprim||!policy->type_val_to_struct){o->primary_result=-EINVAL;goto sampled;}
  o->types.explicit_type=context->type;o->types.explicit_original=policy->type_val_to_struct[context->type-1];
 }
 o->primary_result=sl_selinux_interpret_creation_types(&o->types);
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_creation);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&directory->after_context,&o->after_creation))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
