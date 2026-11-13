/* Supported candidate constraint: primary type IDs must fit the pinned
 * libsepol avtab_key_t uint16_t TE source/target representation. This is
 * not a universal kernel policy type-count ceiling. */
#include "bound_domains_linux.h"
#include <limits.h>
int sl_linux_interpret_bound_domains(sl_linux_bound_domains *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;
 o->actual_context=o->context_original;o->actual_entries=o->entries;o->actual_capacity=o->capacity;
 o->actual_steps=o->steps;o->actual_step_capacity=o->step_capacity;
 sl_linux_selinux_context *c=o->actual_context;
 if(!c||!c->self_attempted||!c->self_returned||c->self_result||!c->attempted||!c->returned||c->result||
    !c->actual_self||!c->actual_self->framing_complete||!c->actual_parse||!c->context_original||
    !c->type||c->type!=c->context_original->type||c->user!=c->context_original->user||c->role!=c->context_original->role||
    !o->actual_entries||!o->actual_capacity||!o->actual_steps||!o->actual_step_capacity||o->count||o->step_count||o->returned||o->after_domains.attempted||o->after_domains.status_original)goto done;
 sl_linux_policy_parse *parsed=c->actual_parse;
 if(!parsed->actual_snapshot||!parsed->returned||parsed->result||parsed->status_result||!parsed->parser.returned||parsed->parser.result||
    !sl_linux_equal_selinux_status(&parsed->after_parse,&c->after_context))goto done;
 o->after_domains.status_original=&parsed->actual_snapshot->status;
 policydb_t *p=&parsed->parser.policy;o->actual_policy=p;o->owner_type=c->type;
 if(p->policy_type!=POLICY_KERN||p->target_platform!=SEPOL_TARGET_SELINUX||!p->p_types.nprim||
    p->p_types.nprim>UINT16_MAX||p->p_types.nprim>o->actual_capacity||o->owner_type>p->p_types.nprim||!p->type_val_to_struct)goto done;
 for(unsigned long i=0;i<o->actual_capacity;i++)if(o->actual_entries[i].type_original||o->actual_entries[i].index||o->actual_entries[i].kind||o->actual_entries[i].step_count)goto done;
 for(unsigned long i=0;i<o->actual_step_capacity;i++)if(o->actual_steps[i].entry_original||o->actual_steps[i].type_original||o->actual_steps[i].index)goto done;
 o->owner_original=p->type_val_to_struct[o->owner_type-1];
 if(!o->owner_original||o->owner_original->s.value!=o->owner_type||o->owner_original->flavor!=TYPE_TYPE)goto done;
 for(unsigned index=1;index<=p->p_types.nprim;index++){
  sl_linux_domain_entry *e=&o->actual_entries[o->count++];type_datum_t *type=p->type_val_to_struct[index-1];
  e->index=index;e->type_original=type;e->first_step=o->step_count;
  if(!type||type->s.value!=index)goto done;
  e->observed_bounds=type->bounds;e->observed_flavor=type->flavor;
  if(type->flavor==TYPE_ATTRIB){e->kind=SL_DOMAIN_ATTRIBUTE;continue;}
  if(type->flavor!=TYPE_TYPE)goto done;
  if(index==o->owner_type){e->kind=SL_DOMAIN_OWNER;continue;}
  unsigned current=index;
  for(;;){
   type_datum_t *node=p->type_val_to_struct[current-1];
   if(!node||node->s.value!=current||node->flavor!=TYPE_TYPE)goto done;
   for(unsigned long j=e->first_step;j<o->step_count;j++)if(o->actual_steps[j].type_original==node)goto done;
   if(o->step_count==o->actual_step_capacity)goto done;
   sl_linux_domain_step *step=&o->actual_steps[o->step_count++];
   step->entry_original=e;step->type_original=node;step->index=current;step->bounds=node->bounds;e->step_count++;
   if(!node->bounds){e->kind=SL_DOMAIN_UNBOUNDED;break;}
   if(node->bounds>p->p_types.nprim)goto done;
   if(node->bounds==o->owner_type){e->kind=SL_DOMAIN_BOUNDED;o->bounded_count++;break;}
   current=node->bounds;
  }
 }
 o->status_result=sl_linux_sample_selinux_status(&o->after_domains);
 if(o->status_result||!sl_linux_equal_selinux_status(&c->after_context,&o->after_domains)){if(!o->status_result)o->status_result=-1;goto done;}
 o->result=0;
 done:o->returned=1;return o->result;
 /* Each candidate still needs complete actor/control/OFD/broker/exec/MLS,
  * nonpermissive/kernel/maintenance and native family/profile interpretation.
  * No unique candidate, gate write, setcurrent or custody disposition occurs. */
}