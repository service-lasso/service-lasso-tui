/* UNRUN matrix-result model; actual policy interpretation tested separately. */
#include "../domain_reachability.h"
#include <assert.h>
static int failure;static unsigned calls;
int sl_selinux_interpret_transition_matrix(sl_selinux_transition_matrix *m){
 calls++;m->attempted=m->returned=1;m->types=2;m->actual_edges=m->edges;m->result=failure?-7:0;
 for(unsigned from=0;from<2;from++)for(unsigned to=0;to<2;to++){sl_selinux_transition_edge *e=&m->edges[from*2+to];e->source_original=m->custody.policy_original->type_val_to_struct[from];e->target_original=m->custody.policy_original->type_val_to_struct[to];}
 m->edges[2].dyntransition=1;return m->result;
}
#include "../domain_reachability.c"
int main(void){
 policydb_t p={0};type_datum_t t[2]={{0}};type_datum_t *types[2]={&t[0],&t[1]};p.policy_type=POLICY_KERN;p.target_platform=SEPOL_TARGET_SELINUX;p.p_types.nprim=2;p.type_val_to_struct=types;t[0].s.value=1;t[1].s.value=2;
 sl_selinux_reachability roots[3]={0};sl_selinux_transition_edge edges[3][4]={0};sl_selinux_reach_step steps[3][4]={0};unsigned reached[3][2]={{0}};
 for(unsigned i=0;i<3;i++){
  sl_selinux_reachability *o=&roots[i];o->policy_original=&p;o->seed=2;o->matrix.edges=edges[i];o->matrix.edge_capacity=4;o->reached=reached[i];o->type_capacity=2;o->steps=steps[i];o->step_capacity=i==2?1:4;failure=i==1;calls=0;
  int result=sl_selinux_interpret_reachability(o);assert(o->returned&&(i?result<0:result==0)&&calls==1);
  if(!i){assert(reached[i][0]==2&&reached[i][1]==2&&o->processed==2&&o->step_count==4);assert(steps[i][0].edge_original==&edges[i][2]&&steps[i][0].target_original==&t[0]&&steps[i][0].target_after==1);}
  if(i==1)assert(o->matrix_result==-7&&!reached[i][1]);
  if(i==2)assert(o->step_count==1&&reached[i][1]==1&&reached[i][0]==1&&!o->processed);
  assert(sl_selinux_interpret_reachability(o)<0&&calls==1);
 }
 assert(reached[0][0]==2&&roots[0].result==0&&roots[1].matrix_result==-7);return 0;
}