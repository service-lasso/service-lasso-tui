/* UNRUN actual resulting-label consumer, controlled context/status boundary. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
static unsigned mode,calls;static sl_linux_public_startup *current;
int sl_linux_interpret_archive_context(sl_linux_selinux_context *o){
 assert(o==&current->archive_contexts[calls]);calls++;o->archive_attempted=o->archive_returned=o->attempted=o->returned=1;o->actual_parse=o->parse_original;o->type=1;return o->result=mode==4?-83:0;
}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return mode!=3;}
#include "../archive_context_linux.c"
typedef struct {sl_linux_public_startup startup;sl_linux_selinux_context contexts[3];sl_linux_policy_parse parse;sl_linux_archive_creation creation;type_datum_t type,*refs[1],*selected[1];unsigned membership[1];} episode;
int main(void){static episode cases[5];for(mode=0;mode<5;mode++){
 episode *e=&cases[mode];sl_linux_public_startup *s=current=&e->startup;
 s->archive_labels_returned=1;s->actual_parses=&e->parse;s->actual_parse_capacity=1;s->archive_contexts=e->contexts;s->archive_context_capacity=3;s->mounts.selinux_count=1;
 s->archive_creations=&e->creation;s->archive_creation_capacity=1;e->creation.returned=e->creation.types.returned=1;e->creation.types.custody.actual_policy=&e->parse.parser.policy;
 e->creation.types.actual_types=e->membership;e->creation.types.actual_referents=e->selected;e->creation.types.actual_capacity=1;
 e->membership[0]=mode==1?0:1;e->selected[0]=mode==2?0:&e->type;e->refs[0]=&e->type;e->parse.parser.policy.type_val_to_struct=e->refs;
 for(unsigned i=0;i<3;i++){e->contexts[i].archive_original=&s->archive_labels[i];e->contexts[i].parse_original=&e->parse;}
 calls=0;long r=sl_linux_interpret_archive_contexts(s);
 assert(s->archive_contexts_returned&&r==(mode==0?0:mode==4?-83:-EINVAL));assert(calls==(mode==0?3:1));
 assert(e->contexts[0].returned&&e->contexts[0].actual_parse==&e->parse&&s->archive_creations==&e->creation);
 if(mode==1)assert(!e->membership[0]);if(mode==2)assert(!e->selected[0]);
 unsigned prior=calls;assert(sl_linux_interpret_archive_contexts(s)==-EINVAL&&calls==prior);
 }assert(cases[0].startup.archive_contexts_result==0&&cases[1].startup.archive_contexts_result==-EINVAL);return 0;}
