/* Controlled raw syscall outputs only; UNRUN, not deployed-kernel proof. */
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "lsm_self_linux.h"
#include <sys/syscall.h>
static unsigned calls,mode;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 c->source=s;c->number=n;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;
 atomic_store(&c->entered,1);calls++;long result=701;
 if(n==459){
  assert(a==100&&e==1&&f==0&&g==0);
  sl_linux_lsm_header *h=(sl_linux_lsm_header*)b;uint32_t *size=(uint32_t*)d;
  assert(h->id==101&&h->flags==0&&h->len==0&&h->ctx_len==0&&*size==64);
  if(mode==1){*size=128;result=-E2BIG;}
  else if(mode==2)result=-ENOSYS;
  else{
   h->id=101;h->flags=0;h->len=48;h->ctx_len=9;memcpy((unsigned char*)h+32,"u:r:t:s0",9);*size=48;result=1;
   if(mode==3)result=2;
   if(mode==4)h->id=102;
   if(mode==5)h->flags=1;
   if(mode==6)h->ctx_len=17;
   if(mode==7)((unsigned char*)h)[35]=0;
   if(mode==8)((unsigned char*)h)[40]='x';
   if(mode==9)((unsigned char*)h)[47]=1;
   if(mode==10)*size=65;
   if(mode==11)h->len=40;
  }
 }
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "lsm_self_linux.c"
int main(void){
 sl_linux_lsm_self o;_Alignas(sl_linux_lsm_header) unsigned char bytes[64];
 for(mode=0;mode<=11;mode++){
  memset(&o,0,sizeof o);memset(bytes,0,sizeof bytes);calls=0;o.bytes=bytes;o.capacity=sizeof bytes;
  assert(sl_linux_prepare_selinux_self(&o)==0);
  assert(sl_linux_prepare_selinux_self(&o)==-1&&calls==0);
  long result=sl_linux_observe_selinux_self(&o);assert(o.attempted&&o.returned&&calls==3);
  assert(o.actual_bytes==bytes&&o.actual_capacity==64&&o.supplied_size==64&&o.raw_before==701&&o.raw_after==701);
  if(!mode){assert(result==0&&o.framing_complete&&o.context==bytes+32&&o.extent==8&&o.examined==9);}
  else{assert(result==-1&&!o.framing_complete&&!o.context);}
  if(mode==1)assert(o.raw_query==-E2BIG&&o.reported_size==128&&o.observed.id==101);
  if(mode==2)assert(o.raw_query==-ENOSYS&&o.reported_size==64);
  assert(sl_linux_observe_selinux_self(&o)==-1&&calls==3);
 }
 memset(&o,0,sizeof o);memset(bytes,0,sizeof bytes);calls=0;o.bytes=bytes;o.capacity=64;
 assert(sl_linux_observe_selinux_self(&o)==-1&&calls==0&&o.returned&&!o.query.source);
 memset(&o,0,sizeof o);memset(bytes,0,sizeof bytes);calls=0;o.bytes=bytes+1;o.capacity=63;
 assert(sl_linux_observe_selinux_self(&o)==-1&&calls==0);
 return 0;
}