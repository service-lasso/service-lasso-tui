/* UNRUN actual owned pinned context-record source. No parser/kernel authority. */
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include <sepol/context_record.h>
static union {max_align_t alignment;unsigned char bytes[8192];} backing[3];
static sl_sepol_memory_attempt rows[3][64];
static void begin(sl_sepol_memory_owner *o,unsigned slot,size_t cap){
 memset(o,0,sizeof *o);memset(&backing[slot],0,sizeof backing[slot]);memset(rows[slot],0,sizeof rows[slot]);o->bytes=backing[slot].bytes;o->capacity=sizeof backing[slot].bytes;o->attempts=rows[slot];o->attempt_capacity=cap;atomic_init(&o->claimed,0);assert(sl_sepol_memory_begin(o)==0);
}
int main(void){
 sl_sepol_memory_owner owners[3];sl_sepol_memory_owner *current=&owners[0];sepol_context_t *record=0;begin(current,0,64);
 assert(sepol_context_create(NULL,&record)==0&&record);assert(sepol_context_set_user(NULL,record,"old")==0);const char *old=sepol_context_get_user(record);
 assert(sepol_context_set_user(NULL,record,"new")==0&&!strcmp(sepol_context_get_user(record),"new")&&!strcmp(old,"old"));
 unsigned retained=0;for(size_t i=0;i<current->attempt_count;i++)if(rows[0][i].operation==SL_SEPOL_POINTER_MUTATION&&rows[0][i].old_pointer==old){assert(rows[0][i].returned);retained++;}assert(retained==1&&sl_sepol_memory_finish(current)==0);
 current=&owners[1];begin(current,1,6);record=0;assert(sepol_context_create(NULL,&record)==0);assert(sepol_context_set_user(NULL,record,"old")==0);old=sepol_context_get_user(record);
 assert(sepol_context_set_user(NULL,record,"new")<0&&sepol_context_get_user(record)==old&&!strcmp(old,"old"));assert(current->first_failure==&current->exhaustion&&current->exhaustion.old_pointer==old&&current->exhaustion.new_pointer);assert(sl_sepol_memory_finish(current)!=0);
 current=&owners[2];begin(current,2,64);record=0;const char original[]="u:r:t:s0";assert(sepol_context_from_string(NULL,original,&record)==0&&record);
 unsigned splits=0;for(size_t i=0;i<current->attempt_count;i++)if(rows[2][i].operation==SL_SEPOL_SCALAR_MUTATION){assert(rows[2][i].old_scalar==':'&&!rows[2][i].new_scalar&&rows[2][i].returned);splits++;}
 assert(splits==3&&!strcmp(original,"u:r:t:s0")&&!strcmp(sepol_context_get_mls(record),"s0"));assert(sl_sepol_memory_finish(current)==0);return 0;
}