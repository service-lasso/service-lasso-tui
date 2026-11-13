/* UNRUN controlled ptrace relation. No kernel/actor or loaded-entry authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_entry_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
static int inventory_failure,status_failure;static unsigned proc_read;static unsigned inventories,samples;
int sl_selinux_interpret_permission_sources(sl_selinux_access_sources *s,const char *c,const char *p){assert(!strcmp(c,proc_read?"file":"process")&&!strcmp(p,proc_read?"read":"ptrace"));inventories++;s->attempted=s->returned=1;s->actual_sources=s->sources;s->source_count=3;s->result=inventory_failure;return inventory_failure;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;s->result=status_failure;return status_failure;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../candidate_entry_linux.c"
typedef struct {sl_linux_candidate_ptrace root;sl_linux_candidate_roles roles;sl_linux_selinux_context owner;sl_linux_lsm_self self;sl_linux_policy_parse parse;sl_linux_context_reachability payload;type_datum_t types[3];type_datum_t *refs[3];unsigned reached[3];sl_selinux_source_access access[3];sl_candidate_route_source rows[3];sl_candidate_role_binding binding[1];sl_candidate_record evidence;unsigned char statement[1];} fixture;
static void prepare(fixture *f){
 f->root.roles_original=&f->roles;f->root.owner_original=&f->owner;f->root.payload_original=&f->payload;f->root.inventory.sources=f->access;f->root.sources=f->rows;f->root.capacity=3;
 f->roles.returned=1;f->roles.policy_original=&f->parse.parser.policy;f->roles.actual_bindings=f->binding;f->roles.count=1;
 f->parse.parser.policy.p_types.nprim=3;f->parse.parser.policy.type_val_to_struct=f->refs;
 for(unsigned i=0;i<3;i++){f->refs[i]=&f->types[i];f->types[i].s.value=i+1;f->access[i].type=i+1;f->access[i].type_original=f->refs[i];}
 f->owner.self_attempted=f->owner.self_returned=f->owner.returned=1;f->owner.actual_self=&f->self;f->owner.actual_parse=&f->parse;f->owner.type=1;
 f->payload.returned=1;f->payload.graph.actual_policy=&f->parse.parser.policy;f->payload.graph.actual_reached=f->reached;f->payload.graph.actual_type_capacity=3;f->reached[1]=2;
 f->binding[0].returned=1;f->binding[0].type_original=&f->types[2];f->binding[0].evidence_original=&f->evidence;f->binding[0].statement_original=(sl_candidate_span){f->statement,1};
 f->binding[0].actor_meaning_original=(sl_candidate_span){(const unsigned char *)"service-lasso.maintenance.whole-type.v1",sizeof("service-lasso.maintenance.whole-type.v1")-1};
 f->binding[0].episode_meaning_original=(sl_candidate_span){(const unsigned char *)"service-lasso.maintenance.enforcement-until-disposition.v1",sizeof("service-lasso.maintenance.enforcement-until-disposition.v1")-1};f->access[0].possible=1;
}
int main(void){fixture cases[14]={0};for(unsigned episode=0;episode<14;episode++){unsigned i=episode%7;proc_read=episode/7;
 fixture *f=&cases[episode];prepare(f);inventory_failure=i==5?-71:0;status_failure=i>=5?-72:0;
 if(i==1||i==2){f->access[1].possible=1;if(i==2)f->reached[1]=0;}
 if(i==3||i==4){f->access[2].possible=1;if(i==4)f->binding[0].actor_meaning_original.extent--;}
 unsigned ib=inventories,sb=samples;long result=proc_read?sl_linux_interpret_candidate_owner_proc_read(&f->root):sl_linux_interpret_candidate_ptrace(&f->root);assert(f->root.returned&&inventories==ib+1&&samples==sb+1);
 if(!i||i==3)assert(!result);else if(i==5)assert(result==-71&&f->root.inventory_result==-71&&f->root.status_result==-72&&!f->root.count);else if(i==6)assert(result==-72&&!f->root.primary_result);else assert(result==-EINVAL);
 if(i==3)assert(f->rows[2].role_original==f->binding&&f->rows[2].evidence_original==&f->evidence);
 assert(!strcmp(f->root.class_original,proc_read?"file":"process"));
 ib=inventories;assert((proc_read?sl_linux_interpret_candidate_owner_proc_read(&f->root):sl_linux_interpret_candidate_ptrace(&f->root))==-EINVAL&&inventories==ib);
}return 0;}