/* UNRUN actual preexec caller connection, controlled downstream observations. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,order[15],count;static sl_linux_public_startup *current;
static void step(unsigned n){order[count++]=n;}
static unsigned prerequisite_calls;
long sl_linux_interpret_control_roster(sl_linux_control_roster *o){assert(o==&current->control_roster);prerequisite_calls++;return 0;}
long sl_linux_interpret_genfs_correspondence(sl_linux_genfs_correspondence *o){assert(o==current->genfs_correspondences&&o->policy_original==current->actual_candidate_policies);prerequisite_calls++;return 0;}
long sl_linux_interpret_startup_migration_routes(sl_linux_public_startup *s){assert(s==current);prerequisite_calls++;return 0;}
long sl_linux_bind_startup_candidate_roles(sl_linux_public_startup *s){assert(s==current);step(1);return 0;}
long sl_linux_interpret_startup_candidate_routes(sl_linux_public_startup *s){assert(s==current);step(2);return 0;}
long sl_linux_interpret_startup_candidate_ptrace(sl_linux_public_startup *s){assert(s==current);step(3);return 0;}
long sl_linux_interpret_startup_candidate_owner_entry(sl_linux_public_startup *s){assert(s==current);step(4);return 0;}
long sl_linux_interpret_startup_candidate_file_entry(sl_linux_public_startup *s){assert(s==current);step(5);return 0;}
long sl_linux_acquire_archive_directory(sl_linux_archive_directory *o){assert(o==&current->archive_directory);step(6);return 0;}
long sl_linux_interpret_archive_precreation(sl_linux_public_startup *s){assert(s==current);step(7);return 0;}
long sl_linux_create_archive_files(sl_linux_archive_files *o){assert(o==&current->archive_files);step(8);return 0;}
long sl_linux_acquire_archive_read_view(sl_linux_archive_read_view *o){assert(o==&current->final_reader&&o->startup_original==current);step(9);o->attempted=o->returned=1;o->result=mode==0?-97:0;return o->result;}
long sl_linux_observe_archive_labels(sl_linux_public_startup *s){assert(s==current);step(10);return mode==3?-101:0;}
long sl_linux_interpret_archive_contexts(sl_linux_public_startup *s){assert(s==current);step(11);return mode==4?-103:0;}
long sl_linux_interpret_archive_accesses(sl_linux_public_startup *s){assert(s==current);step(12);return mode==5?-105:0;}
long sl_linux_bind_candidate_target(sl_linux_candidate_target *o){assert(o==&current->candidate_target);step(13);return mode==1?-99:0;}
long sl_linux_observe_family_events(sl_linux_family_events *o){assert(o==&current->initial_family);step(14);o->framing_complete=1;return 0;}
#include "../candidate_startup_preexec_linux.c"
typedef struct {sl_linux_public_startup s;sl_linux_public_controller c;sl_linux_candidate_preexec preexec;sl_linux_candidate_payload_context payload;sl_linux_selinux_context context;sl_linux_policy_parse parse;sl_linux_candidate_policy policy;sl_linux_genfs_correspondence cohort;} fixture;
static fixture cases[6];
int main(void){for(mode=0;mode<6;mode++){
 fixture *f=&cases[mode];sl_linux_public_startup *s=current=&f->s;s->controller_original=&f->c;f->c.startup_original=s;s->returned=1;s->stage=SL_STARTUP_ACQUIRED;s->candidate_preexec=&f->preexec;s->candidate_preexec_capacity=s->actual_policy_capacity=s->mounts.selinux_count=s->actual_parse_capacity=s->actual_context_capacity=s->actual_candidate_payload_capacity=1;s->actual_contexts=&f->context;s->actual_candidate_payloads=&f->payload;s->actual_parses=&f->parse;f->preexec.owner_original=&f->context;f->preexec.payload_original=&f->payload;f->preexec.child_context.parse_original=&f->parse;s->final_reader.startup_original=s;s->actual_candidate_policies=&f->policy;s->actual_candidate_policy_capacity=1;s->genfs_correspondences=&f->cohort;s->genfs_correspondence_capacity=1;f->cohort.policy_original=&f->policy;count=prerequisite_calls=0;
 long r=sl_linux_bind_controller_preexec(s);assert(prerequisite_calls==3);assert(s->candidate_preexec_returned&&s->final_reader.returned&&s->final_reader.result==(mode==0?-97:0));assert(count==(mode==0?9:mode==1?13:mode==3?10:mode==4?11:mode==5?12:14));for(unsigned i=0;i<count;i++)assert(order[i]==i+1);
 if(mode!=2)assert(r==(mode==0?-97:mode==1?-99:mode==3?-101:mode==4?-103:-105)&&!f->c.candidate_preexec_original);else assert(!r&&f->c.candidate_preexec_original==&f->preexec);
 unsigned before=count;assert(sl_linux_bind_controller_preexec(s)==-EINVAL&&count==before);
 }return 0;}
