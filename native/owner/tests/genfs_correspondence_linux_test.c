/* Source-only controlled correspondence cases, UNRUN. Not ROOT authentication. */
#include <assert.h>
#include <errno.h>
#include "../genfs_correspondence_linux.h"
static long status_failure;static int status_equal=1;
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){s->attempted=s->returned=1;return status_failure;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return status_equal;}
#include "../genfs_correspondence_linux.c"
typedef struct {
 sl_candidate_profile candidate;sl_linux_candidate_policy policy;sl_linux_policy_parse parse;
 sl_linux_genfs_correspondence owner;sl_linux_genfs_declaration retained[2];
 sl_candidate_record evidence[1],routes[2];unsigned char identities[128],interpretation[136],link[24],statement[3];
 sl_linux_selinux_status status;
} episode;
static void prepare(episode *e){
 static const unsigned char meaning[]="service-lasso.linux.cgroup-genfs-initial-cohort.v1";
 e->candidate.returned=1;e->candidate.actual_rows[0]=e->evidence;e->candidate.counts[0]=1;e->candidate.actual_rows[3]=e->routes;e->candidate.counts[3]=1;
 e->candidate.policy_identity=(sl_candidate_span){e->identities,32};e->candidate.kernel_interpretation=(sl_candidate_span){e->identities+32,32};e->candidate.source_identity=(sl_candidate_span){e->identities+64,32};e->candidate.layout_identity=(sl_candidate_span){e->identities+96,32};
 e->interpretation[0]=1;e->link[16]=3;e->evidence[0].field_count=7;e->evidence[0].fields[6]=(sl_candidate_span){e->statement,3};
 e->routes[0].field_count=3;e->routes[0].fields[0]=(sl_candidate_span){e->link,24};e->routes[0].fields[1]=(sl_candidate_span){meaning,sizeof(meaning)-1};e->routes[0].fields[2]=(sl_candidate_span){e->interpretation,136};
 e->policy.returned=1;e->policy.actual_parse=&e->parse;e->policy.actual_candidate=&e->candidate;e->policy.declared_identity=e->candidate.policy_identity;e->policy.kernel_interpretation_original=e->candidate.kernel_interpretation;e->policy.after_binding.status_original=&e->status;
 e->owner.policy_original=&e->policy;e->owner.declarations=e->retained;e->owner.capacity=2;
}
int main(void){
 episode cases[7]={0};for(unsigned i=0;i<7;i++)prepare(&cases[i]);
 assert(sl_linux_interpret_genfs_correspondence(&cases[0].owner)==0);assert(cases[0].retained[0].statement_original.bytes==cases[0].statement);
 cases[1].interpretation[8]=1;assert(sl_linux_interpret_genfs_correspondence(&cases[1].owner)==-EINVAL);assert(cases[1].retained[0].identity_checked[0]);
 cases[2].link[8]=4;assert(sl_linux_interpret_genfs_correspondence(&cases[2].owner)==-EINVAL);
 cases[3].candidate.counts[3]=2;cases[3].routes[1]=cases[3].routes[0];assert(sl_linux_interpret_genfs_correspondence(&cases[3].owner)==0);assert(cases[3].owner.count==2&&cases[3].retained[0].declaration_original!=cases[3].retained[1].declaration_original);
 cases[4].interpretation[4]=1;status_failure=-77;assert(sl_linux_interpret_genfs_correspondence(&cases[4].owner)==-EINVAL);assert(cases[4].owner.status_result==-77);status_failure=0;
 status_equal=0;assert(sl_linux_interpret_genfs_correspondence(&cases[5].owner)==-EINVAL);assert(cases[5].owner.primary_result==0);status_equal=1;
 cases[6].candidate.counts[3]=0;assert(sl_linux_interpret_genfs_correspondence(&cases[6].owner)==-EINVAL);
 assert(sl_linux_interpret_genfs_correspondence(&cases[1].owner)==-EINVAL);assert(cases[1].retained[0].result==-EINVAL);
 return 0;
}
