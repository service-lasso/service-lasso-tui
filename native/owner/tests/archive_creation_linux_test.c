/* UNRUN actual wrapper with controlled policy-union/status boundaries. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,unions,samples;
static sl_linux_archive_creation *current;
int sl_selinux_interpret_creation_types(sl_selinux_creation_types *o){
 assert(o==&current->types&&o->custody.policy_original==&current->directory_original->actual_parse->parser.policy);
 unions++;o->attempted=o->returned=1;
 if(mode==1)assert(o->explicit_type==3&&o->explicit_original==o->custody.policy_original->type_val_to_struct[2]);
 else assert(!o->explicit_type&&!o->explicit_original);
 return o->result=mode==2||mode==4?-71:0;
}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){assert(s==&current->after_creation);samples++;s->attempted=s->returned=1;return s->result=mode==3||mode==4?-72:0;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return mode!=5;}
#include "../archive_creation_linux.c"
typedef struct {sl_linux_public_startup startup;sl_linux_archive_creation creation;sl_linux_selinux_context directory,owner,explicit_context;sl_linux_policy_parse parse;type_datum_t type;type_datum_t *refs[3];sl_linux_selinux_status status;} episode;
int main(void){static episode cases[7];for(mode=0;mode<7;mode++){
 episode *e=&cases[mode];sl_linux_public_startup *s=&e->startup;sl_linux_archive_creation *o=current=&e->creation;
 s->actual_parse_capacity=s->archive_creation_capacity=1;s->archive_creations=o;s->archive_fscreate.returned=1;
 o->startup_original=s;o->directory_original=&e->directory;o->owner_original=&e->owner;o->fscreate_original=&e->explicit_context;
 e->directory.returned=e->directory.archive_returned=e->owner.returned=1;e->directory.type=2;e->owner.type=1;e->directory.actual_parse=e->owner.actual_parse=&e->parse;
 e->directory.after_context.status_original=e->owner.after_context.status_original=&e->status;
 if(mode==1||mode==6){s->archive_fscreate.extent=4;e->explicit_context.returned=e->explicit_context.self_returned=1;e->explicit_context.actual_self=&s->archive_fscreate;e->explicit_context.actual_parse=&e->parse;e->explicit_context.type=3;e->explicit_context.after_context.status_original=&e->status;e->parse.parser.policy.p_types.nprim=3;e->parse.parser.policy.type_val_to_struct=e->refs;e->refs[2]=&e->type;e->type.s.value=3;}
 if(mode==6)e->explicit_context.actual_self=0;
 unions=samples=0;long result=sl_linux_interpret_archive_creation(o);
 assert(o->returned&&result==(mode==2||mode==4?-71:mode==3?-72:mode==5||mode==6?-EINVAL:0));
 assert(unions==(mode>=5?0:1)&&samples==(mode==5?0:1));
 if(mode==4)assert(o->primary_result==-71&&o->status_result==-72&&o->result==-71);
 if(mode==6)assert(!o->types.attempted&&o->primary_result==-EINVAL&&o->fscreate_original==&e->explicit_context);
 unsigned prior=unions;assert(sl_linux_interpret_archive_creation(o)==-EINVAL&&unions==prior);
 }assert(cases[4].creation.primary_result==-71&&cases[4].creation.status_result==-72);return 0;}
