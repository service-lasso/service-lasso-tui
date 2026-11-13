/* UNRUN actual aggregate caller with controlled owned-parser consumer.
 * This does not qualify context parsing, policy/kernel or actor authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
typedef struct {sl_linux_public_startup startup;sl_linux_selinux_context contexts[6];sl_linux_policy_parse parses[2];} episode;
static episode cases[4];static episode *current;static unsigned mode,calls;
int sl_linux_interpret_archive_context(sl_linux_selinux_context *o){
 unsigned kind=calls++;assert(o==&current->contexts[kind*2]&&o->archive_original==&current->startup.archive_labels[kind]&&o->parse_original==&current->parses[0]);o->attempted=o->returned=o->archive_attempted=o->archive_returned=1;o->archive_result=o->result=(mode==1&&kind==1)?-71:0;return o->result;
}
#include "../archive_context_linux.c"
int main(void){for(mode=0;mode<4;mode++){
 episode *e=current=&cases[mode];sl_linux_public_startup *s=&e->startup;s->archive_labels_returned=1;s->actual_parses=e->parses;s->actual_parse_capacity=2;s->mounts.selinux_count=1;s->archive_contexts=e->contexts;s->archive_context_capacity=6;
 for(unsigned i=0;i<6;i++){e->contexts[i].archive_original=&s->archive_labels[i/2];e->contexts[i].parse_original=&e->parses[i%2];}
 if(mode==2)e->contexts[5].parse_original=&e->parses[0];if(mode==3)s->actual_parses=0;calls=0;long result=sl_linux_interpret_archive_contexts(s);
 assert(s->archive_contexts_returned&&s->actual_archive_contexts==e->contexts&&s->actual_archive_context_capacity==6);
 if(!mode)assert(!result&&calls==3&&s->archive_context_count==5&&!e->contexts[1].attempted&&!e->contexts[3].attempted&&!e->contexts[5].attempted);
 if(mode==1)assert(result==-71&&calls==2&&s->archive_context_count==3&&e->contexts[2].result==-71&&!e->contexts[4].attempted);
 if(mode>=2)assert(result==-EINVAL&&!calls&&!s->archive_context_count);
 unsigned before=calls;assert(sl_linux_interpret_archive_contexts(s)==-EINVAL&&calls==before);
 }return 0;}
