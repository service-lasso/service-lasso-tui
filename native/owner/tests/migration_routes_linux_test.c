/* UNRUN actual classifier with distinct controlled completed-policy originals.
 * No actual authorization, policy/native admission or loaded-entry proof. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,samples;
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;return s->result=(mode==5||mode==6)?-91:0;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
int main(void){
 static sl_linux_public_startup startups[8];static sl_linux_migration_routes routes[8][3];static sl_linux_migration_access accesses[8];
 static sl_linux_candidate_roles roles[8];static sl_candidate_role_binding bindings[8][2];static sl_linux_selinux_context owners[8];static sl_linux_lsm_self selves[8];static sl_linux_policy_parse parses[8];
 static sl_linux_context_reachability payloads[8];static unsigned reached[8][3];static type_datum_t datums[8][3],*types[8][3];static sl_selinux_source_access sources[8][3];static sl_candidate_route_source classifications[8][3];
 static sl_candidate_record declarations[8][2],evidence[8][2];static const unsigned char statement[]="retained-qualified-statement";
 static const unsigned char actor[]="service-lasso.maintenance.whole-type.v1",episode[]="service-lasso.maintenance.enforcement-until-disposition.v1",partial[]="partial";
 for(mode=0;mode<8;mode++){
  sl_linux_public_startup *s=&startups[mode];sl_linux_migration_routes *o=&routes[mode][0];sl_linux_candidate_roles *r=&roles[mode];policydb_t *p=&parses[mode].parser.policy;
  p->p_types.nprim=3;p->type_val_to_struct=types[mode];s->migration_routes=routes[mode];s->migration_route_capacity=3;
  o->startup_original=s;o->access_original=&accesses[mode];o->roles_original=r;o->owner_original=&owners[mode];o->payload_original=&payloads[mode];o->sources=classifications[mode];o->capacity=3;
  r->returned=1;r->policy_original=p;r->actual_bindings=bindings[mode];r->count=mode==7?2:1;
  owners[mode].returned=owners[mode].self_returned=1;owners[mode].actual_self=&selves[mode];owners[mode].actual_parse=&parses[mode];owners[mode].type=1;
  payloads[mode].returned=payloads[mode].graph.returned=1;payloads[mode].graph.actual_policy=p;payloads[mode].graph.actual_reached=reached[mode];payloads[mode].graph.actual_type_capacity=3;reached[mode][1]=1;
  accesses[mode].returned=accesses[mode].inventory.returned=1;accesses[mode].inventory.custody.actual_policy=p;accesses[mode].inventory.source_count=accesses[mode].inventory.actual_source_capacity=3;accesses[mode].inventory.actual_sources=sources[mode];
  for(unsigned i=0;i<3;i++){datums[mode][i].s.value=i+1;datums[mode][i].flavor=TYPE_TYPE;types[mode][i]=&datums[mode][i];sources[mode][i].type=i+1;sources[mode][i].type_original=types[mode][i];}
  sources[mode][0].possible=1;if(mode==3)sources[mode][1].possible=1;if(mode==1||mode==2||mode==4||mode==5||mode==7)sources[mode][2].possible=1;
  for(unsigned i=0;i<2;i++){bindings[mode][i].returned=1;bindings[mode][i].declaration_original=&declarations[mode][i];bindings[mode][i].evidence_original=&evidence[mode][i];bindings[mode][i].statement_original=(sl_candidate_span){statement,sizeof(statement)-1};bindings[mode][i].type_original=&datums[mode][2];bindings[mode][i].actor_meaning_original=(sl_candidate_span){actor,sizeof(actor)-1};bindings[mode][i].episode_meaning_original=(sl_candidate_span){episode,sizeof(episode)-1};}
  if(mode==1||mode==5)r->count=0;if(mode==4)bindings[mode][0].actor_meaning_original=(sl_candidate_span){partial,sizeof(partial)-1};
  unsigned before=samples;long result=sl_linux_interpret_migration_routes(o);assert(o->returned&&samples==before+1);
  assert(result==(mode==0||mode==2||mode==7?0:mode==6?-91:-EINVAL));assert(o->status_result==(mode==5||mode==6?-91:0));
  if(mode==7)assert(classifications[mode][2].matching_roles==2&&classifications[mode][2].first_role_original==&bindings[mode][0]&&classifications[mode][2].role_original==&bindings[mode][1]);
  assert(sl_linux_interpret_migration_routes(o)==-EINVAL&&o->result==result&&samples==before+1);
 }
 return 0;
}