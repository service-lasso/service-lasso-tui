#if defined(__linux__) && defined(__x86_64__)
#include "failed_archive_linux.h"
#include <errno.h>
static uint64_t failed_u64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static uint32_t failed_u32(const unsigned char *p){uint32_t n=0;for(unsigned i=0;i<4;i++)n|=(uint32_t)p[i]<<(8*i);return n;}
long sl_linux_interpret_failed_archive_header(sl_linux_failed_archive_header *o,
 const unsigned char *bytes,size_t extent){
 if(!o||!bytes||extent<SL_FAILED_ARCHIVE_HEADER_BYTES)return -EINVAL;
 uintptr_t out=(uintptr_t)o,in=(uintptr_t)bytes;
 if(sizeof(*o)>UINTPTR_MAX-out||extent>UINTPTR_MAX-in||
    (out<in+extent&&in<out+sizeof(*o)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;
 for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 o->attempted=1;o->bytes_original=bytes;o->extent_original=extent;o->result=-EINVAL;
 static const unsigned char magic[8]={'S','L','F','T','A','R','1',0};
 for(unsigned i=0;i<8;i++)if(bytes[i]!=magic[i])goto done;
 if(failed_u32(bytes+8)!=1||failed_u32(bytes+12)||failed_u64(bytes+48)||failed_u64(bytes+56))goto done;
 o->total_extent=failed_u64(bytes+16);o->record_count=failed_u64(bytes+24);
 o->records_extent=failed_u64(bytes+32);o->creator_birth_index=failed_u64(bytes+40);
 if(o->records_extent>UINT64_MAX-SL_FAILED_ARCHIVE_HEADER_BYTES||
    o->total_extent!=SL_FAILED_ARCHIVE_HEADER_BYTES+o->records_extent||
    o->total_extent!=extent||!o->record_count||
    o->record_count>o->records_extent/32||o->creator_birth_index>=o->record_count)goto done;
 /* Full closed record/type/native correspondence is a distinct consumer.
  * A valid header cannot establish terminal, stable bytes, identity or seal. */
 o->result=0;
 done:o->returned=1;return o->result;
}
#endif
