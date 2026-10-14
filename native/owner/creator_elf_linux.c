#if defined(__linux__) && defined(__x86_64__)
#include "creator_elf_linux.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
static unsigned long elf_u16(const unsigned char *p){return p[0]|((unsigned long)p[1]<<8);}
static unsigned long elf_u32(const unsigned char *p){return elf_u16(p)|(elf_u16(p+2)<<16);}
static unsigned long elf_u64(const unsigned char *p){return elf_u32(p)|(elf_u32(p+4)<<32);}
static int elf_disjoint(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int elf_fresh(const void *p,unsigned long n){const unsigned char *b=p;for(unsigned long i=0;i<n;i++)if(b[i])return 0;return 1;}
long sl_linux_interpret_creator_elf(sl_linux_creator_elf *o){
 if(!o||!o->file_original||!o->programs||!o->capacity||o->capacity>ULONG_MAX/sizeof(*o->programs))return -EINVAL;
 const sl_linux_creator_file *f=o->file_original;unsigned long rows=o->capacity*sizeof(*o->programs);
 if(!sl_linux_creator_file_excludes_output(f,o,sizeof(*o))||!sl_linux_creator_file_excludes_output(f,o->programs,rows)||
    !elf_disjoint(o,sizeof(*o),o->programs,rows)||
    !elf_fresh(&o->actual_file,sizeof(*o)-offsetof(sl_linux_creator_elf,actual_file))||!elf_fresh(o->programs,rows))return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->actual_file=f;o->actual_programs=o->programs;o->actual_capacity=o->capacity;o->first_interpreter=ULONG_MAX;
 const unsigned char *b=f->actual_body;unsigned long extent=f->extent;
 if(extent<64||b[0]!=0x7f||b[1]!='E'||b[2]!='L'||b[3]!='F'||b[4]!=2||b[5]!=1||b[6]!=1||
    elf_u32(b+20)!=1||elf_u16(b+52)!=64||elf_u16(b+54)!=56)goto done;
 o->type=elf_u16(b+16);o->machine=elf_u16(b+18);o->entry=elf_u64(b+24);o->program_offset=elf_u64(b+32);o->program_count=elf_u16(b+56);
 if((o->type!=2&&o->type!=3)||o->machine!=62)goto done;
 if(o->program_count==0xffff){
  unsigned long section=elf_u64(b+40);
  if(!section||elf_u16(b+58)!=64||section>extent||64>extent-section)goto done;
  o->extended_count=1;o->extended_count_offset=section+44;o->program_count=elf_u32(b+section+44);
  if(elf_u32(b+section+4)!=0||o->program_count<0xffff)goto done;
 }
 if(!o->program_count||!o->program_offset||o->program_offset>extent||
    o->program_count>(extent-o->program_offset)/56)goto done;
 if(o->program_count>o->actual_capacity){o->result=-ENOSPC;goto done;}
 for(unsigned long i=0;i<o->program_count;i++){
  const unsigned char *p=b+o->program_offset+i*56;sl_linux_elf_program *r=&o->actual_programs[i];
  r->ordinal=i;r->type=elf_u32(p);r->flags=elf_u32(p+4);r->offset=elf_u64(p+8);r->address=elf_u64(p+16);
  r->physical=elf_u64(p+24);r->file_bytes=elf_u64(p+32);r->memory_bytes=elf_u64(p+40);r->alignment=elf_u64(p+48);
  /* PT_NULL's remaining words are undefined, and unknown types are raw
   * observations. Only actually consumed semantic spans are checked here. */
  if(r->type==1){
   o->load_count++;
   if(r->offset>extent||r->file_bytes>extent-r->offset||r->file_bytes>r->memory_bytes||r->memory_bytes>ULONG_MAX-r->address)goto done;
   if(r->alignment>1&&((r->alignment&(r->alignment-1))||r->address%r->alignment!=r->offset%r->alignment))goto done;
  }
  if(r->type==3){
   o->interpreter_count++;if(o->first_interpreter==ULONG_MAX)o->first_interpreter=i;
   if(o->load_count)o->interpreter_after_load=1;
   if(!r->file_bytes||r->offset>extent||r->file_bytes>extent-r->offset)goto done;
   while(r->interpreter_bytes<r->file_bytes&&b[r->offset+r->interpreter_bytes])r->interpreter_bytes++;
   if(r->interpreter_bytes==r->file_bytes)goto done;
  }
  r->interpreted=1;
 }
 if(!sl_linux_creator_file_matches_original(f)||o->file_original!=f||o->programs!=o->actual_programs||o->capacity!=o->actual_capacity)goto done;
 o->complete=1;o->result=0;
done:o->returned=1;return o->result;
 /* No source authentication, loaded-byte equality, interpreter lookup or
  * blanket mutable-section exemption. Actual caller owns all original views. */
}
#endif
