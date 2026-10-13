/* UNRUN actual caller body with synthetic fork returns, not clone/native evidence. */
#include "../outer_birth_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
static long returned_raw;static unsigned calls,mismatch;
long sl_linux_enter_outer_birth(sl_linux_outer_birth *b){
 calls++;b->attempted=1;atomic_store(&b->entered,1);
 if(returned_raw==0){b->raw_child=mismatch?7:0;atomic_store(&b->child_captured,1);}
 else{b->raw_parent=mismatch?returned_raw+1:returned_raw;atomic_store(&b->parent_captured,1);}
 return returned_raw;
}
#include "../outer_continuation_linux.c"
int main(void){
 static struct {sl_linux_outer_birth birth;sl_linux_outer_continuation owner;} cases[6];
 long raw[6]={100,0,-ENOMEM,100,0,-EPERM};
 for(unsigned i=0;i<6;i++){
  sl_linux_outer_birth *b=&cases[i].birth;sl_linux_outer_continuation *o=&cases[i].owner;
  b->prepared=1;o->birth_original=b;returned_raw=raw[i];mismatch=i>=3;
  assert(sl_linux_enter_outer_continuation(o)==returned_raw&&o->attempted&&o->actual_birth==b);
  sl_linux_outer_return *r=returned_raw==0?&o->child:&o->parent,*other=returned_raw==0?&o->parent:&o->child;
  assert(atomic_load_explicit(&r->published,memory_order_acquire)&&r->captured&&r->raw==returned_raw&&r->matched==!mismatch);
  assert(r->result==(mismatch?-EINVAL:returned_raw<0?returned_raw:0));
  assert(!atomic_load_explicit(&other->published,memory_order_acquire)&&!other->captured&&!other->matched&&!other->raw&&!other->result);
  unsigned before=calls;sl_linux_outer_continuation snapshot=*o;
  assert(sl_linux_enter_outer_continuation(o)==-EINVAL&&calls==before&&!memcmp(o,&snapshot,sizeof(snapshot)));
 }
 return 0;
}