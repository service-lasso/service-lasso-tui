/* UNRUN controlled parsed-object graph; no parser/kernel/profile proof. */
#include <assert.h>
#include <string.h>
#include "../bound_domains_linux.h"
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
static int status_failure;
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){s->attempted=s->returned=1;s->actual_status=s->status_original;s->result=status_failure;return s->result;}
#include "../bound_domains_linux.c"
static sl_linux_bound_domains o;static sl_linux_selinux_context context;static sl_linux_policy_parse parse;static sl_linux_lsm_self self;
static sl_linux_selinux_snapshot snapshot;static context_struct_t owner;static type_datum_t types[5];static type_datum_t *pointers[5];
static sl_linux_domain_entry entries[5];static sl_linux_domain_step steps[8];
static void ready(void){
 memset(&o,0,sizeof o);memset(&context,0,sizeof context);memset(&parse,0,sizeof parse);memset(&self,0,sizeof self);memset(&owner,0,sizeof owner);
 memset(&snapshot,0,sizeof snapshot);status_failure=0;parse.actual_snapshot=&snapshot;memset(types,0,sizeof types);memset(entries,0,sizeof entries);memset(steps,0,sizeof steps);
 for(unsigned i=0;i<5;i++){types[i].s.value=i+1;types[i].flavor=TYPE_TYPE;pointers[i]=&types[i];}
 types[1].bounds=1;types[2].bounds=2;types[4].flavor=TYPE_ATTRIB;
 owner.user=owner.role=owner.type=1;self.framing_complete=1;
 context.self_attempted=context.self_returned=context.attempted=context.returned=1;context.actual_self=&self;context.actual_parse=&parse;
 context.context_original=&owner;context.user=context.role=context.type=1;
 parse.returned=parse.parser.returned=1;parse.parser.policy.policy_type=POLICY_KERN;parse.parser.policy.target_platform=SEPOL_TARGET_SELINUX;
 parse.parser.policy.p_types.nprim=5;parse.parser.policy.type_val_to_struct=pointers;
 o.context_original=&context;o.entries=entries;o.capacity=5;o.steps=steps;o.step_capacity=8;
}
int main(void){
 ready();assert(sl_linux_interpret_bound_domains(&o)==0&&o.count==5&&o.bounded_count==2&&o.step_count==4);
 assert(entries[0].kind==SL_DOMAIN_OWNER&&entries[1].kind==SL_DOMAIN_BOUNDED&&entries[2].kind==SL_DOMAIN_BOUNDED&&entries[3].kind==SL_DOMAIN_UNBOUNDED&&entries[4].kind==SL_DOMAIN_ATTRIBUTE);
 assert(entries[2].first_step==1&&entries[2].step_count==2&&steps[1].type_original==&types[2]&&steps[2].type_original==&types[1]);
 assert(sl_linux_interpret_bound_domains(&o)==-1&&o.count==5&&steps[2].type_original==&types[1]);
 ready();o.step_capacity=4;assert(sl_linux_interpret_bound_domains(&o)==0&&o.step_count==4);
 ready();o.step_capacity=3;assert(sl_linux_interpret_bound_domains(&o)==-1&&o.count==4&&o.step_count==3&&entries[3].type_original==&types[3]);
 ready();types[1].bounds=3;types[2].bounds=2;assert(sl_linux_interpret_bound_domains(&o)==-1&&o.count==2&&o.step_count==2&&steps[1].type_original==&types[2]);
 ready();types[1].bounds=6;assert(sl_linux_interpret_bound_domains(&o)==-1&&o.step_count==1&&steps[0].bounds==6);
 ready();pointers[2]=NULL;assert(sl_linux_interpret_bound_domains(&o)==-1&&o.count==3&&entries[2].index==3&&!entries[2].type_original);
 ready();o.capacity=4;assert(sl_linux_interpret_bound_domains(&o)==-1&&!o.count&&!o.step_count);
 ready();status_failure=1;assert(sl_linux_interpret_bound_domains(&o)==-1&&o.count==5&&o.after_domains.returned&&o.status_result==1);
 ready();context.type=2;assert(sl_linux_interpret_bound_domains(&o)==-1&&!o.count);
 ready();parse.result=-7;assert(sl_linux_interpret_bound_domains(&o)==-1&&!o.count&&o.actual_context==&context);
 return 0;
}