/* Controlled library-call seam only; UNRUN, no parser/kernel/context authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include <assert.h>
#include <string.h>
#include "selinux_context_linux.h"
#include "../../third_party/libsepol/src/context.h"
static int calls,raw_result,status_change;static unsigned long begins;
static const unsigned char *expected;static size_t expected_extent;
static context_struct_t actual_context;
int sl_sepol_memory_begin(sl_sepol_memory_owner *m){begins++;m->active=1;return 0;}
int sl_sepol_memory_finish(sl_sepol_memory_owner *m){m->active=0;m->finished=1;return 0;}
int sl_sepol_diagnostics_begin(sl_sepol_diagnostic_owner *d){(void)d;return 0;}
int sl_sepol_diagnostics_finish(sl_sepol_diagnostic_owner *d){(void)d;return 0;}
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){
 s->attempted=s->returned=s->fields_read=1;s->actual_status=s->status_original;
 s->sequence_before=s->sequence_after=status_change?4:2;s->version=1;s->enforcing=1;s->policyload=0;s->deny_unknown=1;s->result=0;return 0;
}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){
 return a->returned&&b->returned&&!a->result&&!b->result&&a->actual_status==b->actual_status&&a->sequence_after==b->sequence_after;
}
int context_from_string(sepol_handle_t *h,const policydb_t *p,context_struct_t **out,const char *body,size_t n){
 (void)h;(void)p;calls++;assert((const unsigned char*)body==expected&&n==expected_extent);
 *out=&actual_context;return raw_result;
}
static unsigned native_calls;static long native_result=1;
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 c->source=s;c->number=n;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;atomic_store(&c->entered,1);native_calls++;
 long result=701;
 if(n==459){assert(a==100&&e==1);sl_linux_lsm_header *h=(sl_linux_lsm_header*)b;uint32_t *size=(uint32_t*)d;assert(h->id==101&&*size==64);h->id=101;h->flags=0;h->len=48;h->ctx_len=9;memcpy((unsigned char*)h+32,"u:r:t:s0",9);*size=48;result=native_result;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "lsm_self_linux.c"
#include "selinux_context_linux.c"
static void ready(sl_linux_selinux_context *o,sl_linux_policy_parse *p,sl_linux_selinux_snapshot *s,const unsigned char *body,size_t n){
 memset(o,0,sizeof *o);memset(p,0,sizeof *p);memset(s,0,sizeof *s);memset(&actual_context,0,sizeof actual_context);
 calls=raw_result=status_change=0;begins=0;expected=body;expected_extent=n;
 actual_context.user=1;actual_context.role=2;actual_context.type=3;
 p->attempted=p->returned=p->parser.returned=p->parser.init_returned=p->parser.read_returned=1;p->actual_snapshot=s;
 s->after_policy.status_original=&s->status;p->after_parse.status_original=&s->status;
 sl_linux_sample_selinux_status(&s->after_policy);sl_linux_sample_selinux_status(&p->after_parse);
 o->parse_original=p;o->body=body;o->extent=n;
}
int main(void){
 sl_linux_selinux_context o;sl_linux_policy_parse p;sl_linux_selinux_snapshot s;
 const unsigned char body[]="user:role:type:s0";
 ready(&o,&p,&s,body,sizeof body-1);assert(sl_linux_interpret_selinux_context(&o)==0&&calls==1&&begins==1);
 assert(o.actual_body==body&&o.actual_extent==sizeof body-1&&o.context_original==&actual_context&&o.user==1&&o.role==2&&o.type==3);
 assert(sl_linux_interpret_selinux_context(&o)==-1&&calls==1&&begins==1);
 ready(&o,&p,&s,body,sizeof body-1);raw_result=-47;status_change=1;
 assert(sl_linux_interpret_selinux_context(&o)==-1&&o.call_result==-47&&o.status_result==-1&&o.context_original==&actual_context&&o.call_returned);
 assert(o.memory.finished&&o.after_context.returned);
 ready(&o,&p,&s,body,sizeof body);assert(sl_linux_interpret_selinux_context(&o)==-1&&calls==0&&begins==0&&o.examined==sizeof body);
 ready(&o,&p,&s,(const unsigned char*)"<<none>>",8);assert(sl_linux_interpret_selinux_context(&o)==-1&&calls==0&&begins==0);
 ready(&o,&p,&s,body,sizeof body-1);p.parser.read_remaining=1;assert(sl_linux_interpret_selinux_context(&o)==-1&&calls==0&&begins==0);
 ready(&o,&p,&s,body,sizeof body-1);status_change=1;assert(sl_linux_interpret_selinux_context(&o)==-1&&o.call_result==0&&o.status_result==-1&&o.context_original==&actual_context);
 sl_linux_lsm_self self;_Alignas(sl_linux_lsm_header) unsigned char raw[64];
 ready(&o,&p,&s,body,sizeof body-1);o.body=NULL;o.extent=0;memset(&self,0,sizeof self);memset(raw,0,sizeof raw);self.bytes=raw;self.capacity=64;native_calls=0;
 assert(sl_linux_prepare_selinux_self(&self)==0);assert(sl_linux_observe_selinux_self(&self)==0&&native_calls==3&&self.observed.ctx_len==9);
 expected=raw+32;expected_extent=8;assert(sl_linux_interpret_selinux_self(&o,&self)==0&&calls==1);
 assert(o.self_original==&self&&o.actual_body==self.context&&o.actual_extent==8&&self.context[8]==0&&self.observed.ctx_len==9);
 assert(sl_linux_interpret_selinux_self(&o,&self)==-1&&calls==1&&native_calls==3);
 ready(&o,&p,&s,body,sizeof body-1);o.body=NULL;o.extent=0;memset(&self,0,sizeof self);memset(raw,0,sizeof raw);self.bytes=raw;self.capacity=64;native_result=-7;
 assert(sl_linux_prepare_selinux_self(&self)==0);assert(sl_linux_observe_selinux_self(&self)==-1);assert(sl_linux_interpret_selinux_self(&o,&self)==-1&&calls==0&&o.self_returned&&o.actual_self==&self);
 sl_linux_cgroup_control_label control;memset(&control,0,sizeof control);
 ready(&o,&p,&s,body,sizeof body-1);o.body=NULL;o.extent=0;o.control_original=&control;
 control.attempted=control.returned=control.framing_complete=1;control.actual_bytes=(unsigned char*)body;control.actual_capacity=sizeof body;control.raw_extent=sizeof body;control.extent=sizeof body-1;control.raw_label_count=sizeof body;
 assert(sl_linux_interpret_control_context(&o)==0&&calls==1&&o.actual_control==&control&&o.actual_body==body&&o.actual_extent==sizeof body-1);
 assert(sl_linux_interpret_control_context(&o)==-1&&calls==1);
 ready(&o,&p,&s,body,sizeof body-1);o.body=NULL;o.extent=0;o.control_original=&control;control.raw_label_count=-34;
 assert(sl_linux_interpret_control_context(&o)==-1&&!calls&&o.control_returned&&o.actual_control==&control);
 return 0;
}