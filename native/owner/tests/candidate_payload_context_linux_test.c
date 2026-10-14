/* UNRUN controlled declaration/call-order cases, no actual child authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_payload_context_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned contexts,graphs,samples;static int context_failure,graph_failure,status_failure;
int sl_linux_interpret_selinux_context(sl_linux_selinux_context *c){contexts++;c->attempted=c->returned=1;c->actual_parse=c->parse_original;c->actual_body=c->body;c->actual_extent=c->extent;c->type=1;c->result=context_failure;return context_failure;}
int sl_selinux_interpret_reachability(sl_selinux_reachability *g){graphs++;g->attempted=g->returned=1;g->actual_policy=g->policy_original;g->actual_seed=g->seed;g->result=graph_failure;return graph_failure;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){samples++;s->attempted=s->returned=1;s->result=status_failure;return status_failure;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../candidate_payload_context_linux.c"
typedef struct {sl_linux_candidate_payload_context root;sl_linux_candidate_policy policy;sl_linux_policy_parse parse;sl_candidate_profile candidate;sl_candidate_record evidence[1],entries[2];unsigned char link[24],body[4],desired[3];} fixture;
static void prepare(fixture *f){
 f->root.policy_original=&f->policy;f->policy.returned=1;f->policy.actual_candidate=&f->candidate;f->policy.actual_parse=&f->parse;
 f->candidate.returned=1;f->candidate.actual_rows[0]=f->evidence;f->candidate.counts[0]=1;f->candidate.actual_rows[2]=f->entries;f->candidate.counts[2]=1;
 f->link[8]=1;f->link[16]=2;f->evidence[0].fields[6]=(sl_candidate_span){f->body,4};
 f->entries[0].field_count=3;f->entries[0].fields[0]=(sl_candidate_span){f->link,24};f->entries[0].fields[1]=(sl_candidate_span){(const unsigned char *)"service-lasso.payload-context.v1",sizeof("service-lasso.payload-context.v1")-1};f->entries[0].fields[2]=(sl_candidate_span){f->desired,3};
}
int main(void){fixture cases[7]={0};for(unsigned i=0;i<7;i++){
 fixture *f=&cases[i];prepare(f);context_failure=i==3?-41:0;graph_failure=i==4?-42:0;status_failure=(i==4||i==5)?-43:0;
 if(i==1)f->link[0]=1;
 if(i==2)f->entries[0].fields[1].extent--;
 if(i==6){f->entries[1]=f->entries[0];f->candidate.counts[2]=2;}
 unsigned cb=contexts,gb=graphs,sb=samples;long result=sl_linux_interpret_candidate_payload_context(&f->root);
 assert(f->root.returned);
 if(i==1||i==2)assert(result==-EINVAL&&contexts==cb&&graphs==gb&&samples==sb);
 else if(i==3)assert(result==-41&&contexts==cb+1&&graphs==gb&&samples==sb);
 else{assert(contexts==cb+1&&graphs==gb+1&&samples==sb+1);if(i==4)assert(result==-42&&f->root.status_result==-43);else if(i==5)assert(result==-43);else assert(!result);assert(!f->root.context.self_attempted&&!f->root.context.actual_self&&f->root.context.actual_body==f->desired&&f->root.statement_original.bytes==f->body+1);}
 cb=contexts;assert(sl_linux_interpret_candidate_payload_context(&f->root)==-EINVAL&&contexts==cb);
}return 0;}