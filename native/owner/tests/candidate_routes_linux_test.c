/* UNRUN source relation cases. Controlled roots are not authorization/native proof. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_routes_linux.h"
#include <assert.h>
#include <errno.h>
static int samples,secondary;
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;s->result=secondary;return secondary;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../candidate_routes_linux.c"
typedef struct {
 sl_linux_candidate_routes root;sl_linux_candidate_roles roles;
 sl_linux_control_access control;sl_linux_context_reachability payload;
 policydb_t policy;type_datum_t type;type_datum_t *types[1];unsigned reached[1];
 sl_selinux_source_access access[1];sl_candidate_role_binding binding[2];
 sl_candidate_route_source rows[1];sl_candidate_record evidence;unsigned char statement[1];
} fixture;
static void prepare(fixture *f){
 f->root.roles_original=&f->roles;f->root.control_original=&f->control;f->root.payload_original=&f->payload;f->root.sources=f->rows;f->root.capacity=1;
 f->roles.returned=f->control.returned=f->payload.returned=1;f->roles.policy_original=&f->policy;f->roles.actual_bindings=f->binding;f->roles.count=1;
 f->policy.p_types.nprim=1;f->policy.type_val_to_struct=f->types;f->types[0]=&f->type;
 f->control.inventory.returned=1;f->control.inventory.custody.actual_policy=&f->policy;f->control.inventory.actual_sources=f->access;f->control.inventory.source_count=1;
 f->access[0].type=1;f->access[0].type_original=&f->type;f->access[0].possible=1;
 f->payload.graph.returned=1;f->payload.graph.actual_policy=&f->policy;f->payload.graph.actual_reached=f->reached;f->payload.graph.actual_type_capacity=1;
 f->binding[0].returned=1;f->binding[0].type_original=&f->type;f->binding[0].evidence_original=&f->evidence;f->binding[0].statement_original=(sl_candidate_span){f->statement,1};
 f->binding[0].actor_meaning_original=(sl_candidate_span){(const unsigned char *)"service-lasso.maintenance.whole-type.v1",sizeof("service-lasso.maintenance.whole-type.v1")-1};
 f->binding[0].episode_meaning_original=(sl_candidate_span){(const unsigned char *)"service-lasso.maintenance.enforcement-until-disposition.v1",sizeof("service-lasso.maintenance.enforcement-until-disposition.v1")-1};
}
int main(void){fixture cases[8]={0};for(unsigned i=0;i<8;i++){
 fixture *f=&cases[i];prepare(f);secondary=i==5?-77:0;
 if(i==1||i==5)f->reached[0]=2;
 if(i==2)f->roles.count=0;
 if(i==3)f->binding[0].actor_meaning_original.extent--;
 if(i==4)f->root.capacity=0;
 if(i==6||i==7){f->binding[1]=f->binding[0];f->binding[1].user_value=2;f->binding[1].role_value=2;f->roles.count=2;if(i==7)f->binding[1].episode_meaning_original.extent--;}
 int before=samples;long result=sl_linux_interpret_candidate_routes(&f->root);assert(f->root.returned&&samples==before+1);
 if(!i||i==6||i==7)assert(!result&&f->rows[0].first_role_original==f->binding&&f->rows[0].evidence_original==&f->evidence&&f->rows[0].source_original==f->access);
 else assert(result==-EINVAL);
 if(i==6)assert(f->rows[0].matching_roles==2&&f->rows[0].first_role_original==&f->binding[0]&&f->rows[0].role_original==&f->binding[1]);
 if(i==7)assert(f->rows[0].matching_roles==1&&f->rows[0].unsupported_scope_roles==1);
 if(i==5)assert(f->root.primary_result==-EINVAL&&f->root.status_result==-77&&f->rows[0].payload_reachable==2);
 before=samples;assert(sl_linux_interpret_candidate_routes(&f->root)==-EINVAL&&samples==before);
}return 0;}