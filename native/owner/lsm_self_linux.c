#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "lsm_self_linux.h"
#include <sys/syscall.h>
#include <limits.h>
#include <stdint.h>
_Static_assert(sizeof(sl_linux_lsm_header)==32,"selected lsm_ctx header ABI");
_Static_assert(_Alignof(sl_linux_lsm_header)==8,"selected lsm_ctx alignment ABI");
_Static_assert(offsetof(sl_linux_lsm_header,ctx_len)==24,"selected lsm_ctx extent ABI");
/* Linux native x86_64 syscall_64.tbl:459 lsm_get_self_attr. */
enum {SL_LSM_GET_SELF_ATTR=459,SL_LSM_SELINUX=101,SL_LSM_CURRENT=100,SL_LSM_EXEC=101,SL_LSM_FSCREATE=102,SL_LSM_SINGLE=1};
static int lsm_disjoint(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(!an||!bn||an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 0;
 return x+an<=y||y+bn<=x;
}
int sl_linux_prepare_selinux_self(sl_linux_lsm_self *o){
 /* Actual caller owns fresh zeroed root storage (arena's fresh anonymous
  * mapping, or explicitly initialized fixture), before any borrower exists.
  * No uninitialized atomic is read to try to prove that precondition. */
 if(!o||o->attempted||o->returned||o->actual_bytes)return -1;
 sl_linux_call *calls[3]={&o->before,&o->query,&o->after};
 for(unsigned i=0;i<3;i++)if(o->initialized_calls[i]||calls[i]->source||calls[i]->number)return -1;
 for(unsigned i=0;i<3;i++){
  atomic_init(&calls[i]->entered,0);atomic_init(&calls[i]->returned,0);o->initialized_calls[i]=calls[i];
 }
 return 0;
}
static long observe_selinux_attribute(sl_linux_lsm_self *o,unsigned attr){
 if(!o||o->attempted)return -1;
 /* Actual arena constructor must establish this before any first write. */
 if(!o->bytes||o->capacity<sizeof(sl_linux_lsm_header)+2||o->capacity>UINT32_MAX||
    (uintptr_t)o->bytes%_Alignof(sl_linux_lsm_header)||!lsm_disjoint(o,sizeof *o,o->bytes,o->capacity))return -1;
 o->attempted=1;o->result=-1;o->actual_bytes=o->bytes;o->actual_capacity=o->capacity;
 sl_linux_call *calls[3]={&o->before,&o->query,&o->after};
 for(unsigned i=0;i<3;i++)if(o->initialized_calls[i]!=calls[i])goto done;
 for(unsigned i=0;i<3;i++)if(calls[i]->source||calls[i]->number||atomic_load(&calls[i]->entered)||atomic_load(&calls[i]->returned))goto done;
 if(o->returned||o->context||o->extent||o->framing_complete||o->supplied_size||o->reported_size)goto done;
 /* This same buffer is used once. Its input selector remains separately
  * retained before the kernel overwrites its output header and context. */
 for(unsigned long i=0;i<o->actual_capacity;i++)if(o->actual_bytes[i])goto done;
 o->requested_id=SL_LSM_SELINUX;o->requested_flags=SL_LSM_SINGLE;o->requested_attr=attr;
 ((sl_linux_lsm_header*)o->actual_bytes)->id=o->requested_id;
 o->supplied_size=(uint32_t)o->actual_capacity;o->reported_size=o->supplied_size;
 o->raw_before=sl_linux_call6(&o->before,"selinux.self.gettid.before",SYS_gettid,0,0,0,0,0,0);
 if(o->raw_before<=0)goto done;
 o->raw_query=sl_linux_call6(&o->query,attr==SL_LSM_EXEC?"selinux.self.explicit.exec":attr==SL_LSM_FSCREATE?"selinux.self.explicit.fscreate":"selinux.self.explicit.current",SL_LSM_GET_SELF_ATTR,
  (long)o->requested_attr,(long)o->actual_bytes,(long)&o->reported_size,(long)o->requested_flags,0,0);
 /* Retain actual after observation even if query failed or copied partials.
  * No allocation/retry uses the kernel's potentially larger reported size. */
 o->raw_after=sl_linux_call6(&o->after,"selinux.self.gettid.after",SYS_gettid,0,0,0,0,0,0);
 o->observed=*(sl_linux_lsm_header*)o->actual_bytes;
 if(o->raw_query!=1||o->raw_after!=o->raw_before||o->reported_size>o->supplied_size||o->reported_size<sizeof o->observed)goto done;
 if(attr!=SL_LSM_CURRENT&&o->observed.id==SL_LSM_SELINUX&&!o->observed.flags&&o->observed.len==sizeof o->observed&&o->reported_size==sizeof o->observed&&!o->observed.ctx_len){o->framing_complete=1;o->result=0;goto done;}
 if(o->observed.id!=SL_LSM_SELINUX||o->observed.flags||o->observed.len!=o->reported_size||
    o->observed.len%8||o->observed.ctx_len<2||o->observed.ctx_len>o->observed.len-sizeof o->observed)goto done;
 unsigned long end=sizeof o->observed+(unsigned long)o->observed.ctx_len;
 if(o->observed.len-end>=8)goto done;
 const unsigned char *context=o->actual_bytes+sizeof o->observed;
 for(unsigned long i=0;i<(unsigned long)o->observed.ctx_len;i++){
  o->examined=i+1;
  if((i+1==o->observed.ctx_len)!=(context[i]==0))goto done;
 }
 for(unsigned long i=end;i<o->observed.len;i++){o->padding_examined++;if(o->actual_bytes[i])goto done;}
 o->context=context;o->extent=(unsigned long)o->observed.ctx_len-1;o->framing_complete=1;o->result=0;
 done:o->returned=1;return o->result;
}
long sl_linux_observe_selinux_self(sl_linux_lsm_self *o){return observe_selinux_attribute(o,SL_LSM_CURRENT);}
long sl_linux_observe_selinux_exec(sl_linux_lsm_self *o){return observe_selinux_attribute(o,SL_LSM_EXEC);}
long sl_linux_observe_selinux_fscreate(sl_linux_lsm_self *o){return observe_selinux_attribute(o,SL_LSM_FSCREATE);}
#endif