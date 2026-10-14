#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_migration_labels(sl_linux_migration_labels *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->primary_result=o->result=-EINVAL;
 sl_linux_public_startup *s=o->startup_original;sl_linux_policy_parse *p=o->parse_original;
 if(!s||!s->migration_labels||o->policy_index>=s->migration_label_capacity||o!=&s->migration_labels[o->policy_index]||
    s->migration_label_capacity!=s->actual_policy_capacity||s->actual_parse_capacity!=s->actual_policy_capacity||!s->actual_parses||o->policy_index>=s->mounts.selinux_count||
    p!=&s->actual_parses[o->policy_index]||!p->returned||p->result||p->status_result||!p->actual_snapshot||
    o->after_labels.attempted||o->after_labels.status_original)goto done;
 o->after_labels.status_original=&p->actual_snapshot->status;
 if(!s->actual_policy_capacity||!s->actual_controls||!s->actual_control_contexts||
    s->actual_control_capacity>ULONG_MAX/s->actual_policy_capacity||
    s->actual_control_context_capacity!=s->actual_control_capacity*s->actual_policy_capacity||
    o->observed_capacity!=s->actual_control_capacity||!o->labels.observed||o->labels.observed_count||
    o->labels.targets.custody.policy_original||o->labels.attempted)goto sampled;
 for(unsigned long i=0;i<o->observed_capacity;i++)if(o->labels.observed[i])goto sampled;
 /* Complete source roster validation precedes writing the observed pointers.
  * Returned success counts never substitute for the fixed custody frontier. */
 for(unsigned long i=0;i<s->actual_control_capacity;i++){
  sl_linux_cgroup_control_label *c=&s->actual_controls[i];if(!c->attempted)continue;
  sl_linux_selinux_context *context=&s->actual_control_contexts[i*s->actual_policy_capacity+o->policy_index];
  if(!c->returned||c->result||!c->framing_complete||!context->control_returned||context->control_result||
     !context->returned||context->result||context->status_result||context->actual_parse!=p||context->actual_control!=c||
     !context->context_original||context->type!=context->context_original->type||
     !sl_linux_equal_selinux_status(&p->after_parse,&context->after_context))goto sampled;
 }
 for(unsigned long i=0;i<s->actual_control_capacity;i++)if(s->actual_controls[i].attempted){
  if(o->labels.observed_count==o->observed_capacity)goto sampled;
  o->labels.observed[o->labels.observed_count++]=s->actual_control_contexts[i*s->actual_policy_capacity+o->policy_index].context_original;
 }
 o->labels.targets.custody.policy_original=&p->parser.policy;
 o->primary_result=sl_selinux_interpret_cgroup_genfs_types(&o->labels);
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_labels);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&p->after_parse,&o->after_labels))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
long sl_linux_interpret_migration_access(sl_linux_migration_access *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->primary_result=o->result=-EINVAL;
 sl_linux_public_startup *s=o->startup_original;sl_linux_migration_labels *labels=o->labels_original;
 if(!s||s->migration_label_capacity>ULONG_MAX/3||s->migration_access_capacity!=3*s->migration_label_capacity||
    !s->migration_access||!s->migration_labels||o->kind>2||o->policy_index>=s->migration_label_capacity||
    o!=&s->migration_access[3*o->policy_index+o->kind]||labels!=&s->migration_labels[o->policy_index]||
    !labels->returned||labels->result||labels->primary_result||labels->status_result||!labels->parse_original||
    !labels->labels.returned||labels->labels.result||o->after_access.attempted||o->after_access.status_original)goto done;
 o->after_access.status_original=labels->after_labels.status_original;
 if(o->inventory.custody.policy_original||o->inventory.custody.target_type||o->inventory.creation_original||
    labels->labels.targets.custody.actual_policy!=&labels->parse_original->parser.policy)goto sampled;
 o->inventory.custody.policy_original=&labels->parse_original->parser.policy;
 o->primary_result=sl_selinux_interpret_target_permission_sources(&o->inventory,
  o->kind==2?&labels->labels.filesystem:&labels->labels.targets,o->kind==2?sl_selinux_name_filesystem:sl_selinux_name_file,
  o->kind?sl_selinux_name_relabelfrom:sl_selinux_name_write,o->kind==2?sl_selinux_name_associate:o->kind?sl_selinux_name_relabelto:sl_selinux_name_append);
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_access);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&labels->after_labels,&o->after_access))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}