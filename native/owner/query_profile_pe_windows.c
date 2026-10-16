#ifdef _WIN32
#include "query_profile_pe_windows.h"
#include <limits.h>
typedef const struct slcli_product_initial_windows_source_span span_type;
static int inside(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an&&bn&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&y>=x&&y-x<=an&&bn<=an-(y-x);
}
static int within(uint64_t start,uint64_t bytes,uint64_t limit){return start<=limit&&bytes<=limit-start;}
static uint16_t u16(const void *p){const unsigned char *b=p;return (uint16_t)(b[0]|(uint16_t)b[1]<<8);}
static uint32_t u32(const void *p){const unsigned char *b=p;return b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static uint64_t u64(const void *p){const unsigned char *b=p;return u32(b)|(uint64_t)u32(b+4)<<32;}
static int covered(const sl_win_query_semantic_row *r,const void *p,size_t n){
 for(size_t i=0;i<r->input_count;i++)if(inside(r->inputs_original[i].original,r->inputs_original[i].bytes,p,n))return 1;
 return 0;
}
static int declared(const sl_win_query_semantic_row *r,span_type *s){
 for(size_t i=0;i<r->input_count;i++)if(s==&r->inputs_original[i])return 1;return 0;
}
static int disjoint(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an&&bn&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int output_away(const sl_win_query_semantic_row *r,const void *out,size_t n){
 if(!out||!n||n>UINTPTR_MAX-(uintptr_t)out||!disjoint(out,n,r,sizeof(*r))||
    r->input_count>SIZE_MAX/sizeof(*r->inputs_original)||!disjoint(out,n,r->inputs_original,r->input_count*sizeof(*r->inputs_original)))return 0;
 for(size_t i=0;i<r->input_count;i++){
  const struct slcli_product_initial_windows_source_span *s=&r->inputs_original[i];
  if(!disjoint(out,n,s->original,s->bytes)||!disjoint(out,n,s->actor_original,s->actor_bytes)||
     !disjoint(out,n,s->occurrence_original,s->occurrence_bytes))return 0;
 }return 1;
}
static int add(uint64_t *a,uint64_t b){if(b>UINT64_MAX-*a)return 0;*a+=b;return 1;}
static int cost(struct slcli_bq_resource_cost *a,const struct slcli_bq_resource_cost *b){
#define F(f) if(!add(&a->f,b->f))return 0
 F(user_ram_bytes);F(user_virtual_bytes);F(kernel_ram_bytes);F(cache_ram_bytes);F(retained_storage_bytes);
 F(native_references);F(physical_extent_events);F(native_attempts);F(pure_attempts);F(cpu_nanoseconds);
#undef F
 return 1;
}
static int row(const sl_win_query_semantic_row *r,const void *original,size_t bytes,
 const sl_win_query_pe_row_source *s,struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 /* Same typed source row and retained interpretation provenance; the genuine
  * constructor separately owns source designation, never inferred from this. */
 return covered(r,original,bytes)&&declared(r,s->source_original)&&declared(r,s->interpretation_original)&&
 s->source_original->original==original&&s->source_original->bytes==bytes&&
 s->cost.user_ram_bytes>=bytes&&s->cost.user_virtual_bytes>=s->cost.user_ram_bytes&&s->cost.pure_attempts&&s->cost.cpu_nanoseconds&&
 cost(a,&s->cost)&&cost(f,&s->failed_overlap_cost);
}
static int array(const sl_win_query_semantic_row *r,const void *p,size_t count,size_t size,size_t alignment){
 if(!count)return p==NULL;
 return count<=SIZE_MAX/size&&p&&!((uintptr_t)p%alignment)&&covered(r,p,count*size);
}
static int power(uint64_t n){return n&&!(n&(n-1));}
static int round_up(uint64_t n,uint64_t alignment,uint64_t *out){
 if(!power(alignment)||n>UINT64_MAX-(alignment-1))return 0;*out=(n+alignment-1)&~(alignment-1);return 1;
}
static int directories(const sl_win_query_image_member *m){
 const IMAGE_NT_HEADERS64 *h=m->headers_original;uint64_t image=h->OptionalHeader.SizeOfImage;
 uint64_t header=h->OptionalHeader.SizeOfHeaders,sa=h->OptionalHeader.SectionAlignment,fa=h->OptionalHeader.FileAlignment,header_end;
 size_t count=h->OptionalHeader.NumberOfRvaAndSizes;
 if(count>IMAGE_NUMBEROF_DIRECTORY_ENTRIES||!power(sa)||!power(fa)||sa<fa||!header||header>m->file_extent||header>image||
    h->OptionalHeader.ImageBase>UINT64_MAX-image||image%sa||header%fa||
    (h->OptionalHeader.AddressOfEntryPoint&&h->OptionalHeader.AddressOfEntryPoint>=image)||
    (sa<4096?fa!=sa:(fa<512||fa>65536))||!round_up(header,sa,&header_end)||header_end>image||
    !within(m->sections_offset,m->section_count*sizeof(IMAGE_SECTION_HEADER),header))return 0;
 for(size_t i=0;i<m->section_count;i++){
  const IMAGE_SECTION_HEADER *s=&m->sections_original[i];uint64_t raw=s->SizeOfRawData,virt=s->Misc.VirtualSize,extent=raw>virt?raw:virt,mapped;
  if(!round_up(extent,sa,&mapped)||s->VirtualAddress%sa||!within(s->VirtualAddress,mapped,image)||(mapped&&s->VirtualAddress<header_end)||
     (raw&&(s->PointerToRawData<header||s->PointerToRawData%fa||raw%fa||!within(s->PointerToRawData,raw,m->file_extent))))return 0;
  for(size_t j=0;j<i;j++){
   const IMAGE_SECTION_HEADER *t=&m->sections_original[j];uint64_t traw=t->SizeOfRawData,tvirt=t->Misc.VirtualSize,tm;
   if(!round_up(traw>tvirt?traw:tvirt,sa,&tm)||
      (mapped&&tm&&s->VirtualAddress<(uint64_t)t->VirtualAddress+tm&&t->VirtualAddress<(uint64_t)s->VirtualAddress+mapped)||
      (raw&&traw&&s->PointerToRawData<(uint64_t)t->PointerToRawData+traw&&t->PointerToRawData<(uint64_t)s->PointerToRawData+raw))return 0;
  }
 }
 for(size_t i=0;i<count;i++){
  const IMAGE_DATA_DIRECTORY *d=&h->OptionalHeader.DataDirectory[i];
  if(d->Size&&(!d->VirtualAddress||!within(d->VirtualAddress,d->Size,i==IMAGE_DIRECTORY_ENTRY_SECURITY?m->file_extent:image)))return 0;
 }
 return 1;
}
/* Same366 RVA kinds:1 actual readonly held file,2 explicit virtual zero,
 *0 gap/crossing/refusal. A zero kind never fabricates a file pointer. */
static unsigned view(const sl_win_query_image_member *m,uint64_t rva,uint64_t bytes,const unsigned char **out,uint64_t *available){
 const IMAGE_NT_HEADERS64 *h=m->headers_original;*out=NULL;*available=0;
 if(!bytes||!within(rva,bytes,h->OptionalHeader.SizeOfImage))return 0;
 if(rva<h->OptionalHeader.SizeOfHeaders){
  if(!within(rva,bytes,h->OptionalHeader.SizeOfHeaders)||!within(rva,bytes,m->file_extent))return 0;
  *out=m->file_bytes_original+(size_t)rva;*available=h->OptionalHeader.SizeOfHeaders-rva;return 1;
 }
 for(size_t i=0;i<m->section_count;i++){
  const IMAGE_SECTION_HEADER *s=&m->sections_original[i];if(rva<s->VirtualAddress)continue;
  uint64_t relative=rva-s->VirtualAddress;
  if(relative<s->SizeOfRawData){
   if(!within(relative,bytes,s->SizeOfRawData)||!within((uint64_t)s->PointerToRawData+relative,bytes,m->file_extent))return 0;
   *out=m->file_bytes_original+s->PointerToRawData+(size_t)relative;*available=s->SizeOfRawData-relative;return 1;
  }
  if(relative>=s->SizeOfRawData&&within(relative,bytes,s->Misc.VirtualSize)){*available=s->Misc.VirtualSize-relative;return 2;}
 }
 return 0;
}
static int source_va(const sl_win_query_image_member *m,uint64_t va,uint64_t bytes,uint64_t *rva){
 uint64_t base=m->headers_original->OptionalHeader.ImageBase;if(va<base)return 0;*rva=va-base;
 return within(*rva,bytes,m->headers_original->OptionalHeader.SizeOfImage);
}
static int original(const sl_win_query_image_member *m,uint64_t rva,uint64_t bytes,const void *p){
 const unsigned char *out;uint64_t available;return view(m,rva,bytes,&out,&available)==1&&out==p;
}
static int string(const sl_win_query_image_member *m,uint64_t rva,const unsigned char *p,size_t n){
 const unsigned char *out;uint64_t available;
 if(!rva||n<2||view(m,rva,1,&out,&available)!=1||out!=p||n>available||p[n-1])return 0;
 for(size_t i=0;i+1<n;i++)if(!p[i])return 0;return 1;
}
static int directory(const sl_win_query_image_member *m,unsigned index,uint64_t *start,uint64_t *bytes){
 *start=*bytes=0;if(index>=m->headers_original->OptionalHeader.NumberOfRvaAndSizes)return 1;
 *start=m->headers_original->OptionalHeader.DataDirectory[index].VirtualAddress;*bytes=m->headers_original->OptionalHeader.DataDirectory[index].Size;return 1;
}
static int imports(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,const sl_win_query_pe_closure *c,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 uint64_t start,size,di=0,ti=0;directory(m,IMAGE_DIRECTORY_ENTRY_IMPORT,&start,&size);
 if(!size)return !c->import_count&&!c->imports_original;
 if(!array(r,c->imports_original,c->import_count,sizeof(*c->imports_original),_Alignof(sl_win_query_pe_import)))return 0;
 for(size_t i=0;i<c->import_count;i++){
  const sl_win_query_pe_import *v=&c->imports_original[i];const unsigned char *d=(const unsigned char *)v->descriptor_original;
  if(!row(r,v,sizeof(*v),&v->source,a,f)||v->descriptor_index!=di||v->thunk_index!=ti||di>size/20||
     !within(di*20,20,size)||!original(m,start+di*20,20,d))return 0;
  uint64_t first=u32(d),stamp=u32(d+4),chain=u32(d+8),name=u32(d+12),iat=u32(d+16);
  if(!(first|stamp|chain|name|iat))return !ti&&i+1==c->import_count&&!v->lookup_original&&!v->iat_rva&&!v->hint_original&&!v->module_original&&!v->module_bytes&&!v->name_original&&!v->name_bytes;
  uint64_t lookup=first?first:iat;
  if(!iat||!string(m,name,v->module_original,v->module_bytes)||ti>UINT64_MAX/8||
     lookup>UINT64_MAX-ti*8||iat>UINT64_MAX-ti*8||!original(m,lookup+ti*8,8,v->lookup_original)||
     !within(iat+ti*8,8,m->headers_original->OptionalHeader.SizeOfImage)||v->iat_rva!=iat+ti*8)return 0;
  uint64_t raw=u64(v->lookup_original);
  if(!raw){if(v->hint_original||v->name_original||v->name_bytes)return 0;di++;ti=0;continue;}
  if(raw>>63){if((raw&UINT64_C(0x7fffffffffff0000))||v->hint_original||v->name_original||v->name_bytes)return 0;}
  else if(raw>MAXDWORD||!original(m,raw,2,v->hint_original)||!string(m,raw+2,v->name_original,v->name_bytes))return 0;
  ti++;
 }
 return 0; /* Missing descriptor/thunk terminal is never implicit. */
}
static int delay_address(const sl_win_query_image_member *m,uint64_t raw,uint64_t bytes,uint32_t attributes,uint64_t *rva){
 if(!raw){*rva=0;return 1;}if(attributes==1){*rva=raw;return within(raw,bytes,m->headers_original->OptionalHeader.SizeOfImage);}
 return source_va(m,raw,bytes,rva);
}
static int delays(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,const sl_win_query_pe_closure *c,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 uint64_t start,size,di=0,ti=0;directory(m,IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,&start,&size);
 if(!size)return !c->delay_count&&!c->delays_original;
 if(!array(r,c->delays_original,c->delay_count,sizeof(*c->delays_original),_Alignof(sl_win_query_pe_delay)))return 0;
 for(size_t i=0;i<c->delay_count;i++){
  const sl_win_query_pe_delay *v=&c->delays_original[i];const unsigned char *d=(const unsigned char *)v->descriptor_original;
  if(!row(r,v,sizeof(*v),&v->source,a,f)||v->descriptor_index!=di||v->thunk_index!=ti||di>size/32||
     !within(di*32,32,size)||!original(m,start+di*32,32,d))return 0;
  uint32_t attributes=u32(d),raw_name=u32(d+4),raw_module=u32(d+8),raw_iat=u32(d+12),raw_int=u32(d+16),raw_bound=u32(d+20),raw_unload=u32(d+24),stamp=u32(d+28);
  if(!(attributes|raw_name|raw_module|raw_iat|raw_int|raw_bound|raw_unload|stamp))return !ti&&i+1==c->delay_count&&
   !v->lookup_original&&!v->module_rva&&!v->iat_rva&&!v->bound_rva&&!v->unload_rva&&!v->hint_original&&!v->module_original&&!v->module_bytes&&!v->name_original&&!v->name_bytes;
  uint64_t name_rva,module_rva,iat,int_rva,bound,unload;
  if(attributes>1||!raw_name||!raw_module||!raw_iat||!raw_int||
     !delay_address(m,raw_name,1,attributes,&name_rva)||!delay_address(m,raw_module,8,attributes,&module_rva)||
     !delay_address(m,raw_iat,8,attributes,&iat)||!delay_address(m,raw_int,8,attributes,&int_rva)||
     !delay_address(m,raw_bound,8,attributes,&bound)||!delay_address(m,raw_unload,8,attributes,&unload)||
     !string(m,name_rva,v->module_original,v->module_bytes)||ti>UINT64_MAX/8||
     iat>UINT64_MAX-ti*8||int_rva>UINT64_MAX-ti*8||!original(m,int_rva+ti*8,8,v->lookup_original)||
     v->module_rva!=module_rva||v->iat_rva!=iat+ti*8||!within(v->iat_rva,8,m->headers_original->OptionalHeader.SizeOfImage))return 0;
  const unsigned char *out;uint64_t available;if(!view(m,module_rva,8,&out,&available))return 0;
  if(raw_bound){if(bound>UINT64_MAX-ti*8||v->bound_rva!=bound+ti*8||!within(v->bound_rva,8,m->headers_original->OptionalHeader.SizeOfImage))return 0;}else if(v->bound_rva)return 0;
  if(raw_unload){if(unload>UINT64_MAX-ti*8||v->unload_rva!=unload+ti*8||!within(v->unload_rva,8,m->headers_original->OptionalHeader.SizeOfImage))return 0;}else if(v->unload_rva)return 0;
  uint64_t raw=u64(v->lookup_original);
  if(!raw){if(v->hint_original||v->name_original||v->name_bytes)return 0;di++;ti=0;continue;}
  if(raw>>63){if((raw&UINT64_C(0x7fffffffffff0000))||v->hint_original||v->name_original||v->name_bytes)return 0;}
  else if(attributes!=1||raw>MAXDWORD||!original(m,raw,2,v->hint_original)||!string(m,raw+2,v->name_original,v->name_bytes))return 0;
  ti++;
 }
 return 0;
}
static int tls(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,const sl_win_query_pe_closure *c,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 uint64_t start,size;directory(m,IMAGE_DIRECTORY_ENTRY_TLS,&start,&size);
 if(!size)return !c->tls_count&&!c->tls_original;
 if(size<40||!array(r,c->tls_original,c->tls_count,sizeof(*c->tls_original),_Alignof(sl_win_query_pe_tls)))return 0;
 for(size_t i=0;i<c->tls_count;i++){
  const sl_win_query_pe_tls *v=&c->tls_original[i];const unsigned char *d=(const unsigned char *)v->directory_original;
  if(!row(r,v,sizeof(*v),&v->source,a,f)||v->callback_index!=i||!original(m,start,40,d))return 0;
  uint64_t first=u64(d),last=u64(d+8),index=u64(d+16),callbacks=u64(d+24),template_rva=0,template_bytes=0,index_rva=0,callback_rva;
  const unsigned char *out;uint64_t available;
  if(first||last){
   if(last<first)return 0;template_bytes=last-first;
   if(!source_va(m,first,template_bytes,&template_rva))return 0;
   if(template_bytes&&!original(m,template_rva,template_bytes,v->template_original))return 0;
  }
  if(v->template_bytes!=template_bytes||(!template_bytes&&v->template_original))return 0;
  unsigned kind=0;
  if(index){if(!source_va(m,index,4,&index_rva)||!(kind=view(m,index_rva,4,&out,&available)))return 0;}
  if(v->index_rva!=index_rva||v->index_kind!=kind)return 0;
  if(!callbacks)return !i&&c->tls_count==1&&!v->callback_original;
  if(!source_va(m,callbacks,8,&callback_rva)||i>UINT64_MAX/8||callback_rva>UINT64_MAX-i*8||
     !original(m,callback_rva+i*8,8,v->callback_original))return 0;
  if(!u64(v->callback_original))return i+1==c->tls_count;
  /* Outside-image callback targets remain raw source observations. The owning
   * qualified loader/provider graph, not this geometry check, interprets them. */
 }
 return 0;
}
static int export_row(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,
 const sl_win_query_pe_export *v,uint64_t start,uint64_t size,size_t index,int named,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 const unsigned char *d=(const unsigned char *)v->directory_original;
 if(!row(r,v,sizeof(*v),&v->source,a,f)||v->selected_index!=index||v->named!=(unsigned)named||!original(m,start,40,d))return 0;
 uint64_t functions=u32(d+20),names=u32(d+24),functions_rva=u32(d+28),names_rva=u32(d+32),ordinals=u32(d+36),fi=index;
 if(!within(functions_rva,functions*4,m->headers_original->OptionalHeader.SizeOfImage)||
    !within(names_rva,names*4,m->headers_original->OptionalHeader.SizeOfImage)||
    !within(ordinals,names*2,m->headers_original->OptionalHeader.SizeOfImage)||
    (functions&&!functions_rva)||(names&&(!names_rva||!ordinals)))return 0;
 if(named){
  if(index>=names||!original(m,names_rva+index*4,4,v->name_rva_original)||
     !original(m,ordinals+index*2,2,v->ordinal_original)||!string(m,u32(v->name_rva_original),v->name_original,v->name_bytes))return 0;
  fi=u16(v->ordinal_original);
 }else if(v->name_rva_original||v->ordinal_original||v->name_original||v->name_bytes)return 0;
 if(fi>=functions||v->function_index!=fi||!original(m,functions_rva+fi*4,4,v->function_original))return 0;
 uint64_t target=u32(v->function_original);
 if(!target)return !v->forwarder_original&&!v->forwarder_bytes;
 if(target>=start&&target-start<size)return string(m,target,v->forwarder_original,v->forwarder_bytes)&&v->forwarder_bytes<=size-(target-start);
 return within(target,1,m->headers_original->OptionalHeader.SizeOfImage)&&!v->forwarder_original&&!v->forwarder_bytes;
}
static int exports(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,const sl_win_query_pe_closure *c,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 uint64_t start,size;directory(m,IMAGE_DIRECTORY_ENTRY_EXPORT,&start,&size);
 if(!size)return !c->function_count&&!c->name_count&&!c->functions_original&&!c->names_original;
 const unsigned char *d;uint64_t available;
 if(size<40||view(m,start,40,&d,&available)!=1||c->function_count!=u32(d+20)||c->name_count!=u32(d+24)||
    !array(r,c->functions_original,c->function_count,sizeof(*c->functions_original),_Alignof(sl_win_query_pe_export))||
    !array(r,c->names_original,c->name_count,sizeof(*c->names_original),_Alignof(sl_win_query_pe_export)))return 0;
 uint64_t functions=u32(d+20),names=u32(d+24),functions_rva=u32(d+28),names_rva=u32(d+32),ordinals=u32(d+36);
 if((functions&&!functions_rva)||(names&&(!names_rva||!ordinals))||
    !within(functions_rva,functions*4,m->headers_original->OptionalHeader.SizeOfImage)||
    !within(names_rva,names*4,m->headers_original->OptionalHeader.SizeOfImage)||
    !within(ordinals,names*2,m->headers_original->OptionalHeader.SizeOfImage))return 0;
 for(size_t i=0;i<c->function_count;i++)if(!export_row(r,m,&c->functions_original[i],start,size,i,0,a,f))return 0;
 for(size_t i=0;i<c->name_count;i++)if(!export_row(r,m,&c->names_original[i],start,size,i,1,a,f))return 0;
 return 1;
}
static int relocations(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,const sl_win_query_pe_closure *c,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 uint64_t start,size,block=0,entry=0;directory(m,IMAGE_DIRECTORY_ENTRY_BASERELOC,&start,&size);
 if(!size)return !c->relocation_count&&!c->relocations_original;
 if(!array(r,c->relocations_original,c->relocation_count,sizeof(*c->relocations_original),_Alignof(sl_win_query_pe_relocation)))return 0;
 for(size_t i=0;i<c->relocation_count;i++){
  const sl_win_query_pe_relocation *v=&c->relocations_original[i];const unsigned char *d=(const unsigned char *)v->block_original;
  if(!row(r,v,sizeof(*v),&v->source,a,f)||v->block_offset!=block||v->entry_index!=entry||!within(block,8,size)||!original(m,start+block,8,d))return 0;
  uint64_t page=u32(d),bytes=u32(d+4);if(page%4096||bytes<8||bytes%2||!within(block,bytes,size))return 0;
  uint64_t count=(bytes-8)/2;
  if(!count){if(entry||v->entry_original||v->target_original||v->target_kind)return 0;block+=bytes;}
  else{
   if(entry>=count||!original(m,start+block+8+entry*2,2,v->entry_original))return 0;
   uint64_t raw=u16(v->entry_original),type=raw>>12,target=page+(raw&4095);
   if(type==IMAGE_REL_BASED_ABSOLUTE){if(v->target_original||v->target_kind)return 0;}
   else{
    const unsigned char *out;uint64_t available;unsigned kind=view(m,target,8,&out,&available);
    if(type!=IMAGE_REL_BASED_DIR64||!kind||v->target_kind!=kind||v->target_original!=out)return 0;
   }
   entry++;if(entry==count){block+=bytes;entry=0;}
  }
  if(block==size)return i+1==c->relocation_count;
 }
 return 0;
}
int sl_win_query_pe_source_matches(const sl_win_query_semantic_row *r,const sl_win_query_image_member *m,
 struct slcli_bq_resource_cost *a,struct slcli_bq_resource_cost *f){
 if(!r||!m||!a||!f||!r->inputs_original||!r->input_count||r->input_count>SIZE_MAX/sizeof(*r->inputs_original)||
    (uintptr_t)m%_Alignof(sl_win_query_image_member)||(uintptr_t)a%_Alignof(struct slcli_bq_resource_cost)||(uintptr_t)f%_Alignof(struct slcli_bq_resource_cost)||
    !covered(r,m,sizeof(*m))||!output_away(r,a,sizeof(*a))||!output_away(r,f,sizeof(*f))||!disjoint(a,sizeof(*a),f,sizeof(*f)))return 0;
 /* Validate the SDK views before any typed dereference, including calls from
  * the independent source-geometry vectors rather than the outer checker. */
 if(!m->file_bytes_original||!m->file_extent||!covered(r,m->file_bytes_original,m->file_extent)||
    m->file_extent<sizeof(IMAGE_DOS_HEADER)||u16(m->file_bytes_original)!=IMAGE_DOS_SIGNATURE||
    m->headers_offset<sizeof(IMAGE_DOS_HEADER)||m->headers_offset>INT32_MAX||
    u32(m->file_bytes_original+offsetof(IMAGE_DOS_HEADER,e_lfanew))!=m->headers_offset||
    !m->headers_original||(uintptr_t)m->headers_original%_Alignof(IMAGE_NT_HEADERS64)||
    !within(m->headers_offset,sizeof(IMAGE_NT_HEADERS64),m->file_extent)||
    m->headers_original!=(const IMAGE_NT_HEADERS64 *)(m->file_bytes_original+m->headers_offset)||
    !m->sections_original||(uintptr_t)m->sections_original%_Alignof(IMAGE_SECTION_HEADER)||
    !m->section_count||m->section_count>SIZE_MAX/sizeof(IMAGE_SECTION_HEADER)||
    !within(m->sections_offset,m->section_count*sizeof(IMAGE_SECTION_HEADER),m->file_extent)||
    m->sections_original!=(const IMAGE_SECTION_HEADER *)(m->file_bytes_original+m->sections_offset))return 0;
 const IMAGE_NT_HEADERS64 *h=m->headers_original;
 if(h->Signature!=IMAGE_NT_SIGNATURE||h->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
    h->FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64)||
    h->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||h->FileHeader.NumberOfSections!=m->section_count||
    m->headers_offset>SIZE_MAX-sizeof(IMAGE_NT_HEADERS64)||m->sections_offset!=m->headers_offset+sizeof(IMAGE_NT_HEADERS64))return 0;
 const sl_win_query_pe_closure *c=m->closure_original;
 if(!c||(uintptr_t)c%_Alignof(sl_win_query_pe_closure)||!covered(r,c,sizeof(*c))||c->member_original!=m||
    !declared(r,c->source_original)||c->source_original->original!=c||c->source_original->bytes!=sizeof(*c)||
    !directories(m))return 0;
 struct slcli_bq_resource_cost accumulated=*a,failed=*f;
 if(!imports(r,m,c,&accumulated,&failed)||!delays(r,m,c,&accumulated,&failed)||!tls(r,m,c,&accumulated,&failed)||
    !exports(r,m,c,&accumulated,&failed)||!relocations(r,m,c,&accumulated,&failed))return 0;
 *a=accumulated;*f=failed;return 1;
}
#endif
