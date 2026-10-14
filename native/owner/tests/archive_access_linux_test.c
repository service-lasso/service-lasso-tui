/* UNRUN classifier with controlled inventory/status. Not actual policy,
 * independently authorized maintenance, enforcement, source closure or seal. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
typedef struct {sl_linux_public_startup startup;sl_linux_archive_access access[25];sl_linux_selinux_context context,owner;sl_linux_policy_parse parse;sl_linux_candidate_roles roles;sl_linux_context_reachability payload;sl_candidate_role_binding bindings[2];type_datum_t types[3],*type_ptrs[3];unsigned reached[3];sl_selinux_source_access inventory[3];sl_linux_archive_source sources[3];} episode;
static episode cases[8];static episode *current;static unsigned mode,inventory_calls,status_calls;
static int inventory(sl_selinux_access_sources *o,const char *class_name){
 inventory_calls++;assert(!strcmp(class_name,mode==7?"dir":"file"));o->attempted=o->returned=1;o->result=mode==6?-73:0;o->actual_sources=current->inventory;o->source_count=3;o->custody.actual_policy=&current->parse.parser.policy;
 for(unsigned i=0;i<3;i++){o->actual_sources[i].type=i+1;o->actual_sources[i].type_original=&current->types[i];o->actual_sources[i].possible=(i==(mode==1?1:mode==4?0:2))?1:0;}
 return o->result;
}
int sl_selinux_interpret_permission_sources(sl_selinux_access_sources *o,const char *c,const char *p){assert(!strcmp(p,"setattr"));return inventory(o,c);}
int sl_selinux_interpret_permission_pair_sources(sl_selinux_access_sources *o,const char *c,const char *a,const char *b){assert(!strcmp(a,"write")&&!strcmp(b,"append"));return inventory(o,c);}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){status_calls++;s->attempted=s->returned=1;return mode==5?-75:mode==6?-74:0;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../archive_access_linux.c"
static const unsigned char whole[]="service-lasso.maintenance.whole-type.v1",episode_meaning[]="service-lasso.maintenance.enforcement-until-disposition.v1",partial[]="partial";
int main(void){for(mode=0;mode<8;mode++){
 episode *e=current=&cases[mode];sl_linux_public_startup *s=&e->startup;unsigned purpose=mode==7?12:mode==5?1:0;sl_linux_archive_access *o=&e->access[purpose];s->archive_accesses=e->access;s->archive_access_capacity=25;s->actual_parse_capacity=1;s->candidate_owner_entry_returned=s->candidate_ptrace_returned=s->candidate_file_entry_returned=1;
 o->startup_original=s;o->purpose=purpose;o->context_original=&e->context;o->owner_original=&e->owner;o->roles_original=&e->roles;o->payload_original=&e->payload;o->sources=e->sources;o->capacity=3;
 e->context.archive_returned=e->context.returned=1;e->context.actual_parse=&e->parse;e->context.type=3;e->owner.returned=1;e->owner.actual_parse=&e->parse;e->owner.type=1;
 e->roles.returned=1;e->roles.policy_original=&e->parse.parser.policy;e->roles.actual_bindings=e->bindings;e->roles.count=mode==2?0:2;
 e->parse.parser.policy.p_types.nprim=3;e->parse.parser.policy.type_val_to_struct=e->type_ptrs;for(unsigned i=0;i<3;i++){e->type_ptrs[i]=&e->types[i];e->types[i].s.value=i+1;}
 e->payload.returned=e->payload.graph.returned=1;e->payload.graph.actual_policy=&e->parse.parser.policy;e->payload.graph.actual_reached=e->reached;e->payload.graph.actual_type_capacity=3;e->reached[1]=1;
 for(unsigned i=0;i<2;i++){e->bindings[i].returned=1;e->bindings[i].type_original=&e->types[2];e->bindings[i].actor_meaning_original=(sl_candidate_span){mode==3?partial:whole,mode==3?sizeof(partial)-1:sizeof(whole)-1};e->bindings[i].episode_meaning_original=(sl_candidate_span){episode_meaning,sizeof(episode_meaning)-1};}
 inventory_calls=status_calls=0;long result=sl_linux_interpret_archive_access(o);assert(o->returned&&inventory_calls==1&&status_calls==1);
 if(mode==0||mode==7)assert(!result&&o->sources[2].relation.matching_roles==2&&o->sources[2].relation.first_role_original==&e->bindings[0]&&o->sources[2].relation.role_original==&e->bindings[1]);
 if(mode==1)assert(result==-EINVAL&&o->sources[1].relation.payload_reachable&&o->sources[1].relation.result==-EINVAL);
 if(mode==2||mode==3)assert(result==-EINVAL&&!o->sources[2].relation.matching_roles);
 if(mode==4)assert(!result&&o->sources[0].conditional_closed_owner&&o->sources[0].owner_original==&e->owner&&!o->sources[0].relation.matching_roles);
 if(mode==5)assert(result==-75&&!o->primary_result&&o->status_result==-75);
 if(mode==6)assert(result==-73&&o->inventory_result==-73&&o->primary_result==-73&&o->status_result==-74);
 unsigned before=inventory_calls;assert(sl_linux_interpret_archive_access(o)==-EINVAL&&inventory_calls==before);
 }return 0;}
