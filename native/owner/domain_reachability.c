#include "domain_reachability.h"
#include <limits.h>
int sl_selinux_interpret_reachability(sl_selinux_reachability *o){
 if(!o||o->attempted)return -1;o->attempted=1;o->result=-1;
 o->actual_policy=o->policy_original;o->actual_seed=o->seed;o->actual_reached=o->reached;o->actual_type_capacity=o->type_capacity;o->actual_steps=o->steps;o->actual_step_capacity=o->step_capacity;
 policydb_t *p=o->actual_policy;unsigned long n=p?p->p_types.nprim:0;
 if(!p||p->policy_type!=POLICY_KERN||p->target_platform!=SEPOL_TARGET_SELINUX||!n||n>UINT16_MAX||!p->type_val_to_struct||!o->actual_seed||o->actual_seed>n||!o->actual_reached||o->actual_type_capacity<n||!o->actual_steps||!o->actual_step_capacity||o->step_count||o->processed||o->matrix.custody.policy_original||o->matrix.attempted)goto done;
 for(unsigned long i=0;i<o->actual_type_capacity;i++)if(o->actual_reached[i])goto done;
 for(unsigned long i=0;i<o->actual_step_capacity;i++){sl_selinux_reach_step *s=&o->actual_steps[i];if(s->source_original||s->target_original||s->edge_original||s->source||s->target||s->source_before||s->source_after||s->target_before||s->target_after)goto done;}
 type_datum_t *seed=p->type_val_to_struct[o->actual_seed-1];if(!seed||seed->s.value!=o->actual_seed||seed->flavor!=TYPE_TYPE)goto done;
 o->matrix.custody.policy_original=p;o->matrix_result=sl_selinux_interpret_transition_matrix(&o->matrix);if(o->matrix_result)goto done;
 if(o->matrix.types!=n)goto done;o->actual_reached[o->actual_seed-1]=1;
 /* 1 means discovered; 2 means its complete edge row has been examined.
  * No source is processed twice. Each target examination retains one step. */
 for(unsigned long count=0;count<n;count++){
  unsigned long source=0;for(;source<n;source++)if(o->actual_reached[source]==1)break;
  if(source==n){o->result=0;break;}
  for(unsigned long target=0;target<n;target++){
   if(o->step_count==o->actual_step_capacity)goto done;
   sl_selinux_transition_edge *edge=&o->matrix.actual_edges[source*n+target];sl_selinux_reach_step *s=&o->actual_steps[o->step_count++];
   s->edge_original=edge;s->source_original=edge->source_original;s->target_original=edge->target_original;s->source=source+1;s->target=target+1;s->source_before=o->actual_reached[source];s->target_before=o->actual_reached[target];
   if((edge->transition||edge->dyntransition)&&!s->target_before)o->actual_reached[target]=1;
   s->target_after=o->actual_reached[target];s->source_after=target==n-1?2:o->actual_reached[source];
  }
  o->actual_reached[source]=2;o->processed++;
 }
 /* Processing all n sources also establishes no undiscovered queued source. */
 if(o->processed==n)o->result=0;
 done:o->returned=1;return o->result;
}