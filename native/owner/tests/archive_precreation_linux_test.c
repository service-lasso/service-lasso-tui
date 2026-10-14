/* UNRUN real precreation phase with controlled label/context/purpose consumers.
 * These observations are not initial ROOT authorization or a filesystem seal. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,labels,contexts,accesses,queries,self_contexts,creations_called;
static sl_linux_public_startup *current;
long sl_linux_observe_selinux_fscreate(sl_linux_lsm_self *o){assert(o==&current->archive_fscreate&&!current->archive_files.attempted);queries++;o->attempted=o->returned=1;o->extent=mode==6?3:0;o->result=mode==5?-64:0;return o->result;}
long sl_linux_observe_archive_directory_before_creation(sl_linux_public_startup *s){assert(s==current&&!s->archive_files.attempted);labels++;return mode==1?-61:0;}
int sl_linux_interpret_archive_context(sl_linux_selinux_context *o){assert(o==&current->archive_contexts[4]);contexts++;o->archive_attempted=o->archive_returned=o->attempted=o->returned=1;o->archive_result=o->result=mode==2?-62:0;return o->result;}
long sl_linux_interpret_archive_access(sl_linux_archive_access *o){assert(o==&current->archive_accesses[(12+accesses)*2]&&creations_called==1);accesses++;o->attempted=o->returned=1;o->result=mode==3&&accesses==3?-63:mode==8&&accesses==13?-66:0;return o->result;}
#include "../archive_precreation_linux.c"
typedef struct {sl_linux_public_startup s;sl_linux_policy_parse parses[2];sl_linux_selinux_context archive[6],owner[2];sl_linux_archive_access purposes[50];sl_linux_candidate_roles roles[2];sl_linux_candidate_payload_context payload[2];} episode;
static episode cases[9];
static sl_linux_archive_creation creations[9][2];static sl_linux_selinux_context fscontexts[9][2];
int sl_linux_interpret_selinux_self(sl_linux_selinux_context *o,sl_linux_lsm_self *self){assert(mode==6&&o==&current->archive_fscreate_contexts[0]&&self==&current->archive_fscreate);self_contexts++;o->self_attempted=o->self_returned=o->attempted=o->returned=1;o->actual_self=self;return 0;}
long sl_linux_interpret_archive_creation(sl_linux_archive_creation *o){assert(o==&current->archive_creations[0]&&contexts==1&&self_contexts==(mode==6?1:0));creations_called++;o->attempted=o->returned=1;return o->result=mode==7?-65:0;}
int main(void){for(mode=0;mode<9;mode++){
 episode *e=&cases[mode];sl_linux_public_startup *s=current=&e->s;unsigned long p=2;
 s->archive_creations=creations[mode];s->archive_creation_capacity=p;s->archive_fscreate_contexts=fscontexts[mode];s->archive_fscreate_context_capacity=p;for(unsigned long j=0;j<p;j++){sl_linux_archive_creation *c=&creations[mode][j];c->startup_original=s;c->policy_index=j;c->directory_original=&e->archive[2*p+j];c->owner_original=&e->owner[j];c->fscreate_original=&fscontexts[mode][j];fscontexts[mode][j].parse_original=&e->parses[j];}s->archive_directory.returned=1;s->actual_parse_capacity=p;s->actual_parses=e->parses;s->archive_contexts=e->archive;s->archive_context_capacity=6;s->archive_accesses=e->purposes;s->archive_access_capacity=50;s->actual_contexts=e->owner;s->actual_context_capacity=p;s->actual_candidate_roles=e->roles;s->actual_candidate_role_capacity=p;s->actual_candidate_payloads=e->payload;s->actual_candidate_payload_capacity=p;s->mounts.selinux_count=1;
 for(unsigned long i=0;i<6;i++){e->archive[i].archive_original=&s->archive_labels[i/p];e->archive[i].parse_original=&e->parses[i%p];}
 for(unsigned long i=0;i<50;i++){unsigned long purpose=i/p,policy=i%p,kind=purpose<6?0:purpose<12?1:2;sl_linux_archive_access *o=&e->purposes[i];if(purpose>=19)o->creation_original=&creations[mode][policy];o->startup_original=s;o->purpose=purpose;o->policy_index=policy;o->context_original=&e->archive[kind*p+policy];o->owner_original=&e->owner[policy];o->roles_original=&e->roles[policy];o->payload_original=&e->payload[policy].reachable;}
 if(mode==4)e->purposes[37].context_original=0;
 queries=labels=contexts=accesses=self_contexts=creations_called=0;long result=sl_linux_interpret_archive_precreation(s);
 assert(s->archive_precreation_returned&&!s->archive_files.attempted);
 assert(result==(mode==0||mode==6?0:mode==1?-61:mode==2?-62:mode==3?-63:mode==5?-64:mode==7?-65:mode==8?-66:-EINVAL));
 assert(queries==(mode==4?0:1)&&labels==(mode==4||mode==5?0:1)&&contexts==(mode==1||mode==4||mode==5?0:1));
 assert(accesses==(mode==0||mode==6||mode==8?13:mode==3?3:0)&&self_contexts==(mode==6?1:0)&&creations_called==(mode==0||mode==3||mode>=6?1:0));
 for(unsigned i=0;i<24;i++)assert(!e->purposes[i].attempted);
 assert(!e->archive[0].attempted&&!e->archive[1].attempted&&!e->archive[2].attempted&&!e->archive[3].attempted&&!e->archive[5].attempted);
 unsigned prior=accesses;assert(sl_linux_interpret_archive_precreation(s)==-EINVAL&&accesses==prior);
 }return 0;}
