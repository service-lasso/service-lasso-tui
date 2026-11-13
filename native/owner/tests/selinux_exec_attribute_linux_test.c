/* UNRUN exact read-only EXEC query framing, substituted raw syscall body. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../lsm_self_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
static unsigned mode,calls;
long sl_linux_call6(sl_linux_call *c,const char *source,long n,long a,long b,long d,long e,long f,long g){calls++;c->source=source;c->number=n;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;atomic_store(&c->entered,1);long result=29;
 if(n==459){assert(a==101&&e==1&&!f&&!g);sl_linux_lsm_header *h=(sl_linux_lsm_header *)b;assert(h->id==101);uint32_t *size=(uint32_t *)d;if(mode==2){*size=4097;result=-E2BIG;}else{*h=(sl_linux_lsm_header){101,0,mode==1?40:32,mode==1?2:0};*size=(uint32_t)h->len;if(mode==1){unsigned char *bytes=(unsigned char *)(h+1);bytes[0]='x';bytes[1]=0;}if(mode==3)h->flags=1;result=1;}}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../lsm_self_linux.c"
typedef struct{sl_linux_lsm_self query;_Alignas(8) unsigned char bytes[64];} fixture;
int main(void){fixture cases[4]={0};for(mode=0;mode<4;mode++){fixture *f=&cases[mode];f->query.bytes=f->bytes;f->query.capacity=sizeof f->bytes;assert(!sl_linux_prepare_selinux_self(&f->query));unsigned before=calls;long result=sl_linux_observe_selinux_exec(&f->query);assert(calls==before+3&&f->query.returned&&f->query.requested_attr==101&&f->query.actual_bytes==f->bytes&&f->query.raw_after==29);
 if(mode<2){assert(!result&&f->query.framing_complete);if(!mode)assert(!f->query.context&&!f->query.extent&&!f->query.observed.ctx_len&&f->query.reported_size==32);else assert(f->query.context==f->bytes+32&&f->query.extent==1&&f->query.observed.ctx_len==2);}
 else assert(result&&(!f->query.framing_complete));if(mode==2)assert(f->query.raw_query==-E2BIG&&f->query.reported_size==4097);
 before=calls;assert(sl_linux_observe_selinux_exec(&f->query)==-1&&calls==before);
 }return 0;}