#ifndef SERVICE_LASSO_SHARED_RANGE_LINUX_H
#define SERVICE_LASSO_SHARED_RANGE_LINUX_H
#include "owner_linux.h"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <limits.h>
/* A containing acquisition and a checked original subrange. No synthetic
 * nested syscall result, ownership transfer, finance or admission is implied. */
static inline void *sl_linux_shared_range_address(const sl_linux_shared_range *r,
 unsigned long needed,unsigned long alignment){
 if(!r||!r->mapping_original||!needed||!alignment||needed>r->bytes)return 0;
 const sl_linux_call *m=r->mapping_original;
 if(!atomic_load_explicit(&m->returned,memory_order_acquire)||
    !atomic_load_explicit(&m->entered,memory_order_acquire)||!m->source||
    m->number!=SYS_mmap||m->result<=0||m->arguments[0]||m->arguments[1]<=0||
    m->arguments[2]!=(PROT_READ|PROT_WRITE)||
    m->arguments[3]!=(MAP_SHARED|MAP_ANONYMOUS)||m->arguments[4]!=-1||m->arguments[5])return 0;
 unsigned long base=(unsigned long)m->result,extent=(unsigned long)m->arguments[1];
 if(extent>ULONG_MAX-base||r->offset>extent||r->bytes>extent-r->offset||
    (base+r->offset)%alignment)return 0;
 return (void *)(base+r->offset);
}
#endif
