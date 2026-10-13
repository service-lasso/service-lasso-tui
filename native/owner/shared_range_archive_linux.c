#if defined(__linux__) && defined(__x86_64__)
#include "shared_range_archive_linux.h"
#include "final_capture_linux.h"
#include <string.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>
static uint64_t range_le64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
long sl_linux_interpret_shared_range_archive(sl_linux_shared_range_archive *o,
 const unsigned char *native,size_t native_extent,const unsigned char *range,size_t range_extent){
 if(!o||o->attempted||!native||!range||native_extent!=80||range_extent!=80)return -EINVAL;
 uintptr_t output=(uintptr_t)o,n=(uintptr_t)native,r=(uintptr_t)range;
 if(sizeof(*o)>UINTPTR_MAX-output||80>UINTPTR_MAX-n||80>UINTPTR_MAX-r||
    (output<n+80&&n<output+sizeof(*o))||(output<r+80&&r<output+sizeof(*o)))return -EINVAL;
 o->attempted=1;o->native_original=native;o->range_original=range;o->result=-EINVAL;
 /* Decode independently from the live range accessor. All scalars remain
  * observations from readonly bytes; never dereference an archived address. */
 if(range_le64(native)!=SYS_mmap||range_le64(native+8)||
    range_le64(native+24)!=(PROT_READ|PROT_WRITE)||
    range_le64(native+32)!=(MAP_SHARED|MAP_ANONYMOUS)||
    range_le64(native+40)!=UINT64_MAX||range_le64(native+48)||
    range_le64(native+64)!=1||range_le64(native+72)!=1)goto done;
 o->mapping_bytes=range_le64(native+16);o->base=range_le64(native+56);
 o->offset=range_le64(range);o->bytes=range_le64(range+8);o->address=range_le64(range+16);
 for(unsigned i=3;i<10;i++)if(range_le64(range+8*i))goto done;
 if(!o->base||o->base>LONG_MAX||!o->mapping_bytes||o->mapping_bytes>LONG_MAX||
    o->mapping_bytes>UINT64_MAX-o->base||!o->bytes||o->offset>o->mapping_bytes||
    o->bytes>o->mapping_bytes-o->offset||o->address!=o->base+o->offset)goto done;
 o->result=0;
 done:o->returned=1;return o->result;
}
long sl_linux_interpret_shared_range_records(sl_linux_shared_range_archive_roster *o,
 const unsigned char *bytes,size_t extent,const sl_linux_final_record *records,size_t count){
 if(!o||o->attempted||o->seen||!bytes||!records||!count||count>SIZE_MAX/sizeof(*records)||o->expected>511)return -EINVAL;
 uintptr_t at=(uintptr_t)o,b=(uintptr_t)bytes,r=(uintptr_t)records;size_t metadata=count*sizeof(*records);
 if(sizeof(*o)>UINTPTR_MAX-at||extent>UINTPTR_MAX-b||metadata>UINTPTR_MAX-r||
    (at<b+extent&&b<at+sizeof(*o))||(at<r+metadata&&r<at+sizeof(*o)))return -EINVAL;
 for(unsigned i=0;i<9;i++)if(o->roles[i].attempted||o->roles[i].returned)return -EINVAL;
 o->attempted=1;o->bytes_original=bytes;o->records_original=records;o->result=-EINVAL;
 size_t cursor=0;const unsigned char *first_native=0;
 for(size_t i=0;i<count;i++){
  const sl_linux_final_record *record=&records[i];
  if(record->offset!=cursor||cursor>extent||extent-cursor<32||record->length>extent-cursor-32)goto done;
  const unsigned char *header=bytes+cursor,*body=header+32;cursor+=32+record->length;
  if(range_le64(header)!=36)continue;
  if(record->length!=80||!i||records[i-1].length!=80||records[i-1].offset+112!=record->offset)goto done;
  const unsigned char *prior=bytes+records[i-1].offset;
  uint64_t stream=range_le64(header+8),frame=range_le64(header+16);unsigned role;
  if(stream==UINT64_MAX-9&&(frame==20||frame==21))role=(unsigned)(frame-20);
  else if(stream==UINT64_MAX-10&&frame>=40&&frame<=46)role=(unsigned)(frame-38);
  else goto done;
  if(range_le64(header+24)!=1||range_le64(prior)!=3||range_le64(prior+8)!=stream||
     range_le64(prior+16)!=frame||range_le64(prior+24)||!(o->expected&(1ULL<<role))||(o->seen&(1ULL<<role)))goto done;
  if(sl_linux_interpret_shared_range_archive(&o->roles[role],prior+32,80,body,80))goto done;
  if(first_native){if(memcmp(first_native,prior+32,80))goto done;}else first_native=prior+32;
  sl_linux_shared_range_archive *current=&o->roles[role];
  for(unsigned j=0;j<9;j++)if(o->seen&(1ULL<<j)){
   sl_linux_shared_range_archive *other=&o->roles[j];
   if(current->offset<other->offset+other->bytes&&other->offset<current->offset+current->bytes)goto done;
  }
  o->seen|=1ULL<<role;
 }
 if(cursor!=extent||o->seen!=o->expected)goto done;
 o->result=0;
 done:o->returned=1;return o->result;
}
#endif
