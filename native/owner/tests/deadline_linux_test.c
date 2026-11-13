/* UNRUN actual deadline bodies with synthetic native clock originals.
 * No OS-clock/provider/caller authority or containing finance is proved. */
#include "../deadline_linux.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/syscall.h>
static struct timespec now;static long clock_result;static unsigned calls;
static sl_linux_deadline_source *change_source;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 assert(number==SYS_clock_gettime&&a==CLOCK_MONOTONIC&&b&&!d&&!e&&!f&&!g);
 calls++;c->source=source;c->number=number;
 long arguments[6]={a,b,d,e,f,g};memcpy(c->arguments,arguments,sizeof arguments);
 atomic_store(&c->entered,1);*(struct timespec*)b=now;
 if(change_source)change_source->duration_ns++;
 c->result=clock_result;atomic_store(&c->returned,1);return clock_result;
}
#include "../deadline_linux.c"
int main(void){
 static unsigned char operation[8];static sl_linux_deadline roots[6];
 sl_linux_deadline_source source={operation,sizeof operation,2};
 sl_linux_deadline_source original_source=source;
 now=(struct timespec){10,999999999};
 assert(sl_linux_construct_deadline(&roots[0],&original_source)==0);
 assert(roots[0].started.tv_sec==10&&roots[0].end.tv_sec==11&&roots[0].end.tv_nsec==1);
 assert(roots[0].source_original==&original_source&&roots[0].operation_original==operation);
 unsigned before=calls;unsigned char original[sizeof(roots[0])];memcpy(original,&roots[0],sizeof original);
 assert(sl_linux_construct_deadline(&roots[0],&source)==-EINVAL&&calls==before);
 assert(!memcmp(original,&roots[0],sizeof original));
 clock_result=-EIO;now=(struct timespec){20,3};
 assert(sl_linux_construct_deadline(&roots[1],&source)==-EIO);
 assert(roots[1].returned&&roots[1].raw_clock==-EIO&&roots[1].started.tv_sec==20);
 clock_result=0;now=(struct timespec){10,1000000000};
 assert(sl_linux_construct_deadline(&roots[2],&source)==-EINVAL&&roots[2].returned);
 source.duration_ns=1000000000UL;now=(struct timespec){LONG_MAX,0};
 assert(sl_linux_construct_deadline(&roots[3],&source)==-EINVAL&&roots[3].returned);
 source.duration_ns=0;before=calls;
 assert(sl_linux_construct_deadline(&roots[4],&source)==-EINVAL&&calls==before&&!roots[4].attempted);
 source.duration_ns=2;source.operation_original=&roots[5];source.operation_extent=sizeof roots[5];
 assert(sl_linux_construct_deadline(&roots[5],&source)==-EINVAL&&calls==before&&!roots[5].attempted);
 /* Distinct source tuples do not renew the original root. Malformed fixture
  * mutations below must refuse before or after the actual synthetic call. */
 static sl_linux_deadline_observation observations[8];
 source.operation_original=operation;source.operation_extent=sizeof operation;source.duration_ns=999999;
 now=(struct timespec){10,999999999};
 observations[0].deadline_original=&roots[0];
 assert(sl_linux_observe_deadline(&observations[0])==0);
 assert(observations[0].remaining.tv_sec==0&&observations[0].remaining.tv_nsec==2);
 observations[1].deadline_original=&roots[0];now=(struct timespec){11,1};
 assert(sl_linux_observe_deadline(&observations[1])==1&&observations[1].expired);
 observations[2].deadline_original=&roots[0];now=(struct timespec){10,999999998};
 assert(sl_linux_observe_deadline(&observations[2])==-EINVAL&&!observations[2].expired);
 observations[3].deadline_original=&roots[0];clock_result=-EINTR;now=(struct timespec){12,3};
 assert(sl_linux_observe_deadline(&observations[3])==-EINTR&&observations[3].raw_clock==-EINTR&&!observations[3].expired);
 observations[4].deadline_original=&roots[1];before=calls;
 assert(sl_linux_observe_deadline(&observations[4])==-EINVAL&&calls==before&&!observations[4].attempted);
 unsigned char observed[sizeof observations[0]];memcpy(observed,&observations[0],sizeof observed);
 assert(sl_linux_observe_deadline(&observations[0])==-EINVAL&&calls==before);
 assert(!memcmp(observed,&observations[0],sizeof observed));
 clock_result=0;observations[5].deadline_original=&roots[0];original_source.duration_ns=3;
 assert(sl_linux_observe_deadline(&observations[5])==-EINVAL&&calls==before&&!observations[5].attempted);
 original_source.duration_ns=2;observations[6].deadline_original=&roots[0];roots[0].end.tv_nsec=2;
 assert(sl_linux_observe_deadline(&observations[6])==-EINVAL&&calls==before&&!observations[6].attempted);
 roots[0].end.tv_nsec=1;observations[7].deadline_original=&roots[0];now=(struct timespec){10,999999999};change_source=&original_source;
 assert(sl_linux_observe_deadline(&observations[7])==-EINVAL&&calls==before+1&&observations[7].returned&&!observations[7].expired);
 change_source=0;
 assert(roots[0].duration_ns==2&&roots[0].end.tv_sec==11&&roots[0].end.tv_nsec==1);
 /* Separate fixture-owned root/source for sequence enrollment. No production
  * once-state or earlier deadline occurrence is reset or consumed again. */
 static sl_linux_deadline sequence_root;
 static sl_linux_deadline_source sequence_source;
 sequence_source=(sl_linux_deadline_source){operation,sizeof operation,2};
 now=(struct timespec){10,999999999};
 assert(sl_linux_construct_deadline(&sequence_root,&sequence_source)==0);
 static sl_linux_deadline_observation roster[3];
 static sl_linux_deadline_sequence sequence;
 sequence.root_original=&sequence_root;sequence.observations_original=roster;sequence.capacity=3;
 assert(sl_linux_prepare_deadline_sequence(&sequence)==0&&sequence.prepared);
 before=calls;assert(sl_linux_prepare_deadline_sequence(&sequence)==-EINVAL&&calls==before);
 now=(struct timespec){11,0};
 assert(sl_linux_observe_next_deadline(&sequence)==0&&sequence.next==1);
 now=(struct timespec){10,999999999};
 assert(sl_linux_observe_next_deadline(&sequence)==-EINVAL&&sequence.next==2&&sequence.failed);
 assert(sequence.first_failure==&roster[1]&&roster[1].returned&&roster[1].raw_clock==0);
 assert(roster[0].observed.tv_sec==11&&roster[0].observed.tv_nsec==0&&!roster[2].attempted);
 before=calls;assert(sl_linux_observe_next_deadline(&sequence)==-EINVAL&&calls==before);
 static sl_linux_deadline_observation short_roster[1];
 static sl_linux_deadline_sequence short_sequence;
 short_sequence.root_original=&sequence_root;short_sequence.observations_original=short_roster;short_sequence.capacity=1;
 assert(sl_linux_prepare_deadline_sequence(&short_sequence)==-EINVAL&&!short_sequence.attempted);
 static sl_linux_deadline short_root;
 now=(struct timespec){10,999999999};assert(sl_linux_construct_deadline(&short_root,&sequence_source)==0);
 short_sequence.root_original=&short_root;
 assert(sl_linux_prepare_deadline_sequence(&short_sequence)==0);
 now=(struct timespec){10,999999999};assert(sl_linux_observe_next_deadline(&short_sequence)==0);
 before=calls;assert(sl_linux_observe_next_deadline(&short_sequence)==-ENOSPC&&short_sequence.exhausted&&calls==before);
 /* Actual paired boundary body, synthetic clock originals only; UNRUN. */
 static sl_linux_deadline pair_root;
 static sl_linux_deadline_source pair_source;
 static sl_linux_deadline_observation pair_rows[4];
 static sl_linux_deadline_sequence pair_sequence;
 static sl_linux_deadline_boundary pair,late,exhausted,insufficient;
 pair_source=(sl_linux_deadline_source){operation,sizeof operation,2};
 now=(struct timespec){10,999999999};
 assert(!sl_linux_construct_deadline(&pair_root,&pair_source));
 pair_sequence.root_original=&pair_root;pair_sequence.observations_original=pair_rows;pair_sequence.capacity=4;
 assert(!sl_linux_prepare_deadline_sequence(&pair_sequence));
 assert(!sl_linux_begin_deadline_boundary(&pair,&pair_sequence)&&pair.ready&&pair_sequence.next==1);
 assert(pair.before==&pair_rows[0]&&pair.after==&pair_rows[1]&&!pair.after_attempted);
 before=calls;assert(sl_linux_begin_deadline_boundary(&pair,&pair_sequence)==-EINVAL&&calls==before);
 now=(struct timespec){11,0};assert(!sl_linux_end_deadline_boundary(&pair)&&pair.returned&&pair_sequence.next==2);
 assert(sl_linux_deadline_boundary_matches_original(&pair));
 pair.after->remaining.tv_nsec++;assert(!sl_linux_deadline_boundary_matches_original(&pair));pair.after->remaining.tv_nsec--;
 before=calls;assert(sl_linux_end_deadline_boundary(&pair)==-EINVAL&&calls==before);
 assert(!sl_linux_begin_deadline_boundary(&late,&pair_sequence));
 now=(struct timespec){11,1};assert(sl_linux_end_deadline_boundary(&late)==1&&late.returned&&late.after->expired);
 assert(late.before->result==0&&late.after_result==1&&pair_sequence.expired);
 before=calls;assert(sl_linux_begin_deadline_boundary(&exhausted,&pair_sequence)==-EINVAL&&calls==before&&!exhausted.attempted);
 before=calls;assert(sl_linux_begin_deadline_boundary(&insufficient,&short_sequence)==-EINVAL&&calls==before&&!insufficient.attempted);
 return 0;
}
