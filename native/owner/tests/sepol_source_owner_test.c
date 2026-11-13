/* UNRUN actual source allocator cases; no upstream library/policy/native grant. */
#include "../sepol_source_owner.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "../sepol_source_owner.c"
typedef union backing {max_align_t alignment;unsigned char bytes[256];} backing;
static backing all_storage[16],all_replacement[16];
static sl_sepol_memory_attempt all_attempts[16][16];static sl_sepol_memory_owner all_owners[16];
static unsigned next_slot,current_slot;
#define storage all_storage[current_slot]
#define replacement all_replacement[current_slot]
#define attempts all_attempts[current_slot]
#define owner all_owners[current_slot]
static void fresh(void){
 assert(next_slot<16);current_slot=next_slot++;
 owner=(sl_sepol_memory_owner){.bytes=storage.bytes,.capacity=sizeof storage.bytes,.attempts=attempts,.attempt_capacity=16};atomic_init(&owner.claimed,0);
}
static void clear_pointer(void **field){SL_SEPOL_CLEANUP_POINTER(*field,NULL);}
static void clear_scalar(unsigned *field){SL_SEPOL_CLEANUP_SCALAR(*field,0);}
int main(void){
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 unsigned char *original=sl_sepol_source_malloc(4,"original",10);assert(original&&original==storage.bytes);
 memcpy(original,"abcd",4);owner.bytes=replacement.bytes;owner.capacity=1;owner.attempt_capacity=1;
 unsigned char *grown=sl_sepol_source_realloc(original,8,"grown",11);
 assert(grown&&grown!=original&&!memcmp(grown,"abcd",4)&&!memcmp(original,"abcd",4));
 assert(attempts[1].previous_allocation==&attempts[0]&&attempts[0].logically_released&&attempts[1].returned);
 assert(owner.actual_bytes==storage.bytes&&owner.actual_attempt_capacity==16);
 sl_sepol_source_free(grown,"free",12);assert(attempts[2].input==grown&&attempts[2].previous_allocation==&attempts[1]&&attempts[1].logically_released);
 assert(!memcmp(original,"abcd",4)&&!memcmp(grown,"abcd",4));
 assert(sl_sepol_memory_finish(&owner)==0&&owner.finished&&!owner.active);
 assert(sl_sepol_memory_finish(&owner)<0&&sl_sepol_memory_begin(&owner)<0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 void *zero=sl_sepol_source_calloc(3,4,"zero",20);assert(zero);for(unsigned i=0;i<12;i++)assert(!((unsigned char*)zero)[i]);
 sl_sepol_source_free(NULL,"null",21);assert(attempts[1].returned&&!attempts[1].input&&!attempts[1].error);
 assert(sl_sepol_memory_finish(&owner)==0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 original=sl_sepol_source_malloc(4,"original",30);memcpy(original,"keep",4);
 assert(!sl_sepol_source_realloc(original,SIZE_MAX,"too-large",31)&&attempts[1].error==ENOMEM);
 assert(!attempts[0].logically_released&&!memcmp(original,"keep",4)&&owner.first_failure==&attempts[1]);
 sl_sepol_source_free(original,"cleanup",32);assert(attempts[2].returned&&attempts[0].logically_released&&!memcmp(original,"keep",4));
 assert(sl_sepol_memory_finish(&owner)<0); /* even a handled allocation failure remains a whole-profile denial */
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 assert(!sl_sepol_source_calloc(SIZE_MAX,2,"overflow",40)&&attempts[0].count==SIZE_MAX&&attempts[0].size==2&&attempts[0].error==ENOMEM&&!owner.used);
 assert(sl_sepol_memory_finish(&owner)<0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 original=sl_sepol_source_malloc(4,"original",50);sl_sepol_source_free(original,"free",51);sl_sepol_source_free(original,"double",52);
 assert(attempts[2].input==original&&attempts[2].error==EINVAL&&owner.failed&&owner.first_failure==&attempts[2]);
 assert(sl_sepol_memory_finish(&owner)<0);
 fresh();owner.attempt_capacity=1;assert(sl_sepol_memory_begin(&owner)==0);
 original=sl_sepol_source_malloc(4,"original",60);sl_sepol_source_free(original,"capacity",61);
 assert(owner.failed&&owner.first_failure==&owner.exhaustion&&owner.exhaustion.input==original&&owner.exhaustion.error==ENOMEM);
 assert(!attempts[0].logically_released&&owner.attempt_count==1&&owner.exhaustion.returned);
 assert(sl_sepol_memory_finish(&owner)<0);
 fresh();attempts[0].source="retained";assert(sl_sepol_memory_begin(&owner)<0&&attempts[0].source&&!owner.prepared&&owner.begin_entered&&owner.begin_returned&&owner.begin_result<0);
 assert(sl_sepol_memory_begin(&owner)<0&&owner.begin_returned&&attempts[0].source);
 fresh();owner.attempts=(sl_sepol_memory_attempt*)storage.bytes;assert(sl_sepol_memory_begin(&owner)<0&&!owner.prepared);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);unsigned char foreign=7;
 sl_sepol_source_free(&foreign,"foreign",70);assert(foreign==7&&attempts[0].input==&foreign&&attempts[0].error==EINVAL);
 assert(sl_sepol_memory_finish(&owner)<0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 const char *text="alpha";char *copy=sl_sepol_source_strdup(text,"duplicate",80);
 assert(copy&&!strcmp(copy,text)&&attempts[0].input==text&&attempts[0].extent==6&&attempts[0].operation==SL_SEPOL_DUPLICATE);
 char *prefix=sl_sepol_source_strndup(text,2,"prefix",81);assert(prefix&&!strcmp(prefix,"al")&&attempts[1].count==2&&attempts[1].input==text);
 char *empty=sl_sepol_source_strndup(text,0,"empty",82);assert(empty&&empty[0]==0&&attempts[2].extent==1);
 assert(sl_sepol_memory_finish(&owner)==0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 original=sl_sepol_source_malloc(4,"original",90);memcpy(original,"keep",4);
 assert(!sl_sepol_source_reallocarray(original,SIZE_MAX,2,"array-overflow",91)&&attempts[1].count==SIZE_MAX&&attempts[1].size==2&&attempts[1].input==original);
 assert(attempts[1].error==ENOMEM&&!attempts[0].logically_released&&!memcmp(original,"keep",4));
 assert(sl_sepol_memory_finish(&owner)<0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);char unterminated[256];memset(unterminated,'x',sizeof unterminated);
 assert(!sl_sepol_source_strdup(unterminated,"finite-string",100)&&attempts[0].input==unterminated&&attempts[0].error==ENOMEM&&!owner.used);
 assert(sl_sepol_memory_finish(&owner)<0);
 fresh();assert(sl_sepol_memory_begin(&owner)==0);
 void *target=sl_sepol_source_malloc(4,"edge-target",90);void *edge=target;unsigned scalar=64;
 clear_pointer(&edge);clear_scalar(&scalar);
 assert(!edge&&!scalar&&attempts[1].operation==SL_SEPOL_POINTER_MUTATION&&attempts[1].field_original==&edge);
 assert(attempts[1].old_pointer==target&&!attempts[1].new_pointer&&attempts[1].returned);
 assert(attempts[2].old_scalar==64&&!attempts[2].new_scalar&&attempts[2].field_original==&scalar&&attempts[2].returned);
 assert(!attempts[0].logically_released&&sl_sepol_memory_finish(&owner)==0);
 fresh();owner.attempt_capacity=1;assert(sl_sepol_memory_begin(&owner)==0);
 target=sl_sepol_source_malloc(4,"edge-target",91);edge=target;clear_pointer(&edge);
 assert(edge==target&&owner.exhaustion.field_original==&edge&&owner.exhaustion.old_pointer==target&&owner.exhaustion.error==ENOMEM);
 assert(sl_sepol_memory_finish(&owner)==-1);
 return 0;
}