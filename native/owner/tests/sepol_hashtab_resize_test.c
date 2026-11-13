/* UNRUN actual pinned owned hashtab linked source; no parser/fit authority. */
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "../sepol_source_owner.h"
#include <sepol/policydb/hashtab.h>
#include <sepol/policydb/policydb.h>
static union {max_align_t alignment;unsigned char bytes[8192];} backing[4];
static sl_sepol_memory_attempt rows[4][64];static unsigned hashes,callbacks;
static unsigned hash(hashtab_t h,const_hashtab_key_t key){(void)h;(void)key;hashes++;return 0;}
static int compare(hashtab_t h,const_hashtab_key_t a,const_hashtab_key_t b){(void)h;return strcmp(a,b);}
static int visit(hashtab_key_t key,hashtab_datum_t datum,void *arg){(void)key;(void)datum;(void)arg;callbacks++;return 0;}
static void destroy(hashtab_key_t key,hashtab_datum_t datum,void *arg){(void)key;(void)datum;(void)arg;callbacks++;}
static void begin(sl_sepol_memory_owner *o,unsigned slot,size_t cap){
 memset(o,0,sizeof *o);o->bytes=backing[slot].bytes;o->capacity=sizeof backing[slot].bytes;o->attempts=rows[slot];o->attempt_capacity=cap;atomic_init(&o->claimed,0);assert(sl_sepol_memory_begin(o)==0);hashes=0;
}
int main(void){
 policydb_t policies[3]={{0}};sl_sepol_memory_owner owners[4];hashtab_t tables[3];hashtab_ptr_t *old_tables[3];hashtab_ptr_t first_nodes[3];char a[]="a",b[]="b";
 for(unsigned slot=0;slot<3;slot++){
  sl_sepol_memory_owner *o=&owners[slot];begin(o,slot,slot==1?8:slot==2?10:64);tables[slot]=hashtab_create(hash,compare,1);assert(tables[slot]&&tables[slot]->sl_source_owner==o);assert(hashtab_insert(tables[slot],a,a)==0);old_tables[slot]=tables[slot]->htable;first_nodes[slot]=old_tables[slot][0];
  int result=hashtab_insert(tables[slot],b,b);
  if(!slot){assert(result==0&&tables[slot]->size==2&&tables[slot]->nel==2&&tables[slot]->htable!=old_tables[slot]&&old_tables[slot][0]==first_nodes[slot]);unsigned old_next=0,old_table=0;for(size_t i=0;i<o->attempt_count;i++){sl_sepol_memory_attempt *r=&rows[slot][i];if(r->field_original==&first_nodes[slot]->next&&r->operation==SL_SEPOL_POINTER_MUTATION){assert(r->returned);old_next++;}if(r->field_original==&tables[slot]->htable&&r->old_pointer==old_tables[slot]){assert(r->returned);old_table++;}}assert(old_next==3&&old_table==1&&sl_sepol_memory_finish(o)==0);}
  else{assert(result<0&&hashes==2&&tables[slot]->size==2&&tables[slot]->nel==1&&tables[slot]->htable==old_tables[slot]&&old_tables[slot][0]==first_nodes[slot]&&o->first_failure==&o->exhaustion);if(slot==1)assert(o->exhaustion.operation==SL_SEPOL_POINTER_MUTATION&&o->exhaustion.field_original==&first_nodes[slot]->next);else assert(o->exhaustion.operation==SL_SEPOL_LOGICAL_FREE&&o->exhaustion.input==old_tables[slot]);size_t prior=o->attempt_count;sl_sepol_memory_attempt *first=o->first_failure;callbacks=0;
 assert(hashtab_search(tables[slot],b)==NULL&&hashtab_remove(tables[slot],b,destroy,NULL)<0&&hashtab_map(tables[slot],visit,NULL)<0&&hashtab_insert(tables[slot],b,b)<0);
 policies[slot].sl_source_owner=o;policies[slot].p_classes.table=tables[slot];hashtab_hash_eval(tables[slot],"failed");hashtab_destroy(tables[slot]);policydb_destroy(&policies[slot]);assert(policies[slot].p_classes.table==tables[slot]);
 assert(hashes==2&&!callbacks&&o->attempt_count==prior&&o->first_failure==first&&tables[slot]->htable==old_tables[slot]&&tables[slot]->size==2&&tables[slot]->nel==1);
 assert(sl_sepol_memory_finish(o)!=0);}
 }
 begin(&owners[3],3,64);callbacks=0;assert(sl_sepol_source_current_owner()==&owners[3]);
 for(unsigned slot=1;slot<3;slot++){
  assert(hashtab_search(tables[slot],b)==NULL&&hashtab_map(tables[slot],visit,NULL)<0&&hashtab_remove(tables[slot],b,destroy,NULL)<0&&hashtab_insert(tables[slot],b,b)<0);
  assert(policydb_read(&policies[slot],NULL,0)!=0);
  hashtab_destroy(tables[slot]);hashtab_hash_eval(tables[slot],"old-failed");policydb_destroy(&policies[slot]);
  assert(tables[slot]->sl_source_owner==&owners[slot]&&policies[slot].sl_source_owner==&owners[slot]&&tables[slot]->size==2&&tables[slot]->htable==old_tables[slot]);
 }
 assert(!hashes&&!callbacks&&!owners[3].attempt_count&&!owners[3].failed&&!owners[3].first_failure);
 assert(hashtab_search(tables[0],a)==a&&hashes==1);assert(hashtab_insert(tables[0],b,b)<0&&hashes==1);
 assert(sl_sepol_memory_finish(&owners[3])==0);
 assert(tables[1]->htable==old_tables[1]&&tables[2]->htable==old_tables[2]&&owners[1].failed&&owners[2].failed);return 0;
}