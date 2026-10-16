#ifdef _WIN32
#include "query_profile_windows.h"
#ifndef SERVICE_LASSO_PRODUCT_HELD_FILE_HEADER
#error exact source-qualified CLI held-file header required
#endif
#include SERVICE_LASSO_PRODUCT_HELD_FILE_HEADER
#include <limits.h>

typedef const struct slcli_product_initial_windows_source_span source_span;
static int range(const void *p,size_t n){return p&&n&&n<=UINTPTR_MAX-(uintptr_t)p;}
static int aligned(const void *p,size_t n){return p&&!((uintptr_t)p%n);}
static int away(const void *a,size_t an,const void *b,size_t bn){
 return range(a,an)&&range(b,bn)&&!((uintptr_t)a<(uintptr_t)b+bn&&(uintptr_t)b<(uintptr_t)a+an);
}
static int contains(const void *a,size_t an,const void *b,size_t bn){
 return range(a,an)&&range(b,bn)&&(uintptr_t)b>=(uintptr_t)a&&
 ((uintptr_t)b-(uintptr_t)a)<=an&&bn<=an-((uintptr_t)b-(uintptr_t)a);
}
static int array(size_t count,size_t unit,size_t *bytes){if(!count||count>SIZE_MAX/unit)return 0;*bytes=count*unit;return 1;}
static int span(source_span *s){return aligned(s,_Alignof(struct slcli_product_initial_windows_source_span))&&range(s,sizeof(*s))&&range(s->original,s->bytes)&&range(s->actor_original,s->actor_bytes)&&range(s->occurrence_original,s->occurrence_bytes);}
static int span_away(source_span *s,const void *out,size_t n){
 return span(s)&&away(out,n,s,sizeof(*s))&&away(out,n,s->original,s->bytes)&&
 away(out,n,s->actor_original,s->actor_bytes)&&away(out,n,s->occurrence_original,s->occurrence_bytes);
}
static int same(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return 0;return 1;}
static int zero(const void *a,size_t n){const unsigned char *x=a;for(size_t i=0;i<n;i++)if(x[i])return 0;return 1;}
static int add(uint64_t *a,uint64_t b){if(b>UINT64_MAX-*a)return 0;*a+=b;return 1;}
static int cost_add(struct slcli_bq_resource_cost *a,const struct slcli_bq_resource_cost *b){
#define COST_FIELD(f) if(!add(&a->f,b->f))return 0
 COST_FIELD(user_ram_bytes);COST_FIELD(user_virtual_bytes);COST_FIELD(kernel_ram_bytes);COST_FIELD(cache_ram_bytes);
 COST_FIELD(retained_storage_bytes);COST_FIELD(native_references);COST_FIELD(physical_extent_events);
 COST_FIELD(native_attempts);COST_FIELD(pure_attempts);COST_FIELD(cpu_nanoseconds);
#undef COST_FIELD
 return 1;
}
static int cost_covers(const struct slcli_bq_resource_cost *a,const struct slcli_bq_resource_cost *b){
#define COST_COVERS(f) if(a->f<b->f)return 0
 COST_COVERS(user_ram_bytes);COST_COVERS(user_virtual_bytes);COST_COVERS(kernel_ram_bytes);COST_COVERS(cache_ram_bytes);
 COST_COVERS(retained_storage_bytes);COST_COVERS(native_references);COST_COVERS(physical_extent_events);
 COST_COVERS(native_attempts);COST_COVERS(pure_attempts);COST_COVERS(cpu_nanoseconds);
#undef COST_COVERS
 return 1;
}
static unsigned arm(unsigned role){
 if(role<=SL_WIN_QUERY_TOKEN_ACCESS)return SL_WIN_QUERY_PRINCIPAL;
 if(role==SL_WIN_QUERY_WRITER_STACK||role==SL_WIN_QUERY_WRITER_TURNS)return SL_WIN_QUERY_WRITER;
 return SL_WIN_QUERY_IMAGE;
}
/* Every actual semantic pointer must be in a declared original input, in
 * addition to its own typed/extent checks. This is coverage, not authenticity. */
static int covered(const sl_win_query_semantic_row *r,const void *p,size_t n){
 for(size_t i=0;i<r->input_count;i++)if(contains(r->inputs_original[i].original,r->inputs_original[i].bytes,p,n))return 1;
 return 0;
}
static int declared(const sl_win_query_semantic_row *r,source_span *s){
 if(!span(s))return 0;
 for(size_t i=0;i<r->input_count;i++)if(s==&r->inputs_original[i])return 1;
 return 0;
}
static int sid(const sl_win_query_semantic_row *r,const sl_win_query_sid_source *s){
 size_t n;
 if(!s->original)return !s->bytes&&!s->source_original;
 if(!declared(r,s->source_original)||!aligned(s->original,_Alignof(SID))||s->bytes<offsetof(SID,SubAuthority)||!covered(r,s->original,s->bytes))return 0;
 if(s->original->Revision!=SID_REVISION||s->original->SubAuthorityCount>SID_MAX_SUB_AUTHORITIES)return 0;
 n=offsetof(SID,SubAuthority)+(size_t)s->original->SubAuthorityCount*sizeof(DWORD);
 return n==s->bytes;
}
static int hash(const sl_win_query_semantic_row *r,const sl_win_query_hash_source *h){
 size_t bytes;
 if(!h->original)return !h->attributes_original&&!h->count&&!h->sids_original;
 if(!aligned(h->original,_Alignof(SID_AND_ATTRIBUTES_HASH))||!covered(r,h->original,sizeof(*h->original))||h->original->SidCount!=h->count||
    h->original->SidAttr!=h->attributes_original)return 0;
 if(!h->count)return !h->sids_original; /* Retain the raw zero-count SidAttr; do not dereference it. */
 if(!aligned(h->attributes_original,_Alignof(SID_AND_ATTRIBUTES))||!array(h->count,sizeof(*h->attributes_original),&bytes)||!covered(r,h->attributes_original,bytes)||
    !aligned(h->sids_original,_Alignof(sl_win_query_sid_source))||!array(h->count,sizeof(*h->sids_original),&bytes)||!covered(r,h->sids_original,bytes))return 0;
 for(size_t i=0;i<h->count;i++)if(!sid(r,&h->sids_original[i])||h->attributes_original[i].Sid!=h->sids_original[i].original)return 0;
 return 1;
}
static int hash_inside(const void *body,size_t n,const sl_win_query_hash_source *h){
 if(!h->original)return 1;
 if(!contains(body,n,h->original,sizeof(*h->original)))return 0;
 if(h->count&&!contains(body,n,h->attributes_original,h->count*sizeof(*h->attributes_original)))return 0;
 for(size_t i=0;i<h->count;i++)if(!contains(body,n,h->sids_original[i].original,h->sids_original[i].bytes))return 0;
 return 1;
}
static int principal_case(const sl_win_query_semantic_row *r,const sl_win_query_principal_case *p,uint64_t *dimension){
 size_t bytes;
 if(!declared(r,p->source_original)||!p->effective_original||!p->primary_original||
    !covered(r,p->effective_original,sizeof(*p->effective_original))||!covered(r,p->primary_original,sizeof(*p->primary_original))||
    !declared(r,p->native_layout_source))return 0;
 if(r->role==SL_WIN_QUERY_TOKEN_ACCESS){
  const TOKEN_ACCESS_INFORMATION *a=p->access_original;
  if(!aligned(a,_Alignof(TOKEN_ACCESS_INFORMATION))||!contains(p->access_bytes_original,p->access_extent,a,sizeof(*a))||
     (const void *)a!=(const void *)p->access_bytes_original||!covered(r,p->access_bytes_original,p->access_extent)||
     !hash(r,&p->sid_hash)||!hash(r,&p->restricted_hash)||!hash(r,&p->capabilities_hash)||
     !hash_inside(p->access_bytes_original,p->access_extent,&p->sid_hash)||
     !hash_inside(p->access_bytes_original,p->access_extent,&p->restricted_hash)||
     !hash_inside(p->access_bytes_original,p->access_extent,&p->capabilities_hash)||
     !sid(r,&p->package_sid)||!sid(r,&p->trust_sid)||a->SidHash!=p->sid_hash.original||
     a->RestrictedSidHash!=p->restricted_hash.original||a->CapabilitiesHash!=p->capabilities_hash.original||
     a->PackageSid!=p->package_sid.original||a->TrustLevelSid!=p->trust_sid.original||
     a->Privileges!=p->privileges_original||a->Flags||a->SecurityAttributes)return 0;
  if(p->privileges_original){
   if(!aligned(p->privileges_original,_Alignof(TOKEN_PRIVILEGES))||!covered(r,p->privileges_original,offsetof(TOKEN_PRIVILEGES,Privileges))||
      p->privileges_original->PrivilegeCount!=p->privilege_count||
      p->privilege_count>(SIZE_MAX - offsetof(TOKEN_PRIVILEGES,Privileges))/sizeof(LUID_AND_ATTRIBUTES))return 0;
   bytes=offsetof(TOKEN_PRIVILEGES,Privileges)+p->privilege_count*sizeof(LUID_AND_ATTRIBUTES);
   if(!covered(r,p->privileges_original,bytes)||!contains(p->access_bytes_original,p->access_extent,p->privileges_original,bytes))return 0;
  }else if(p->privilege_count)return 0;
  if((p->package_sid.original&&!contains(p->access_bytes_original,p->access_extent,p->package_sid.original,p->package_sid.bytes))||
     (p->trust_sid.original&&!contains(p->access_bytes_original,p->access_extent,p->trust_sid.original,p->trust_sid.bytes)))return 0;
  /* This is a source-selected complete layout envelope, not the returned size
   * of the query it will fund. Constructor/nativecase relation stays external. */
  *dimension=p->access_extent;
 }else{
  const SECURITY_DESCRIPTOR_RELATIVE *d=p->descriptor_original;
  if(!aligned(d,_Alignof(SECURITY_DESCRIPTOR_RELATIVE))||(const void *)d!=(const void *)p->descriptor_bytes_original||
     !contains(p->descriptor_bytes_original,p->descriptor_extent,d,sizeof(*d))||
     !covered(r,p->descriptor_bytes_original,p->descriptor_extent)||d->Revision!=SECURITY_DESCRIPTOR_REVISION||
     !(d->Control&SE_SELF_RELATIVE)||!sid(r,&p->owner_sid)||!sid(r,&p->group_sid))return 0;
  if((d->Owner&&(!contains(p->descriptor_bytes_original,p->descriptor_extent,p->owner_sid.original,p->owner_sid.bytes)||
      d->Owner!=(uintptr_t)p->owner_sid.original-(uintptr_t)d))||(!d->Owner&&p->owner_sid.original)||
     (d->Group&&(!contains(p->descriptor_bytes_original,p->descriptor_extent,p->group_sid.original,p->group_sid.bytes)||
      d->Group!=(uintptr_t)p->group_sid.original-(uintptr_t)d))||(!d->Group&&p->group_sid.original))return 0;
  if(d->Dacl){
   const ACL *acl=p->dacl_original;size_t cursor=sizeof(*acl);
   if(!aligned(acl,_Alignof(ACL))||!contains(p->descriptor_bytes_original,p->descriptor_extent,acl,sizeof(*acl))||
      d->Dacl!=(uintptr_t)acl-(uintptr_t)d||acl->AceCount!=p->ace_count||
      acl->AclSize<sizeof(*acl)||!contains(p->descriptor_bytes_original,p->descriptor_extent,acl,acl->AclSize))return 0;
   if(p->ace_count&&(!aligned(p->aces_original,_Alignof(sl_win_query_ace_source))||!array(p->ace_count,sizeof(*p->aces_original),&bytes)||!covered(r,p->aces_original,bytes)))return 0;
   for(size_t i=0;i<p->ace_count;i++){
    const sl_win_query_ace_source *v=&p->aces_original[i];
    if(!declared(r,v->source_original)||!contains(acl,acl->AclSize,v->original,v->bytes)||
       !aligned(v->original,_Alignof(ACE_HEADER))||v->bytes<sizeof(ACE_HEADER)||v->original->AceSize!=v->bytes||
       (const unsigned char *)v->original!=(const unsigned char *)acl+cursor||v->bytes>acl->AclSize-cursor)return 0;
    cursor+=v->bytes;
   }
   if(cursor>acl->AclSize)return 0;
  }else if(p->dacl_original||p->ace_count||p->aces_original)return 0;
  *dimension=p->descriptor_extent;
 }
 return *dimension&&*dimension<=MAXDWORD;
}
static int principal(const sl_win_query_semantic_row *r,uint64_t *dimension){
 const sl_win_query_principal_geometry *p=&r->source.principal;size_t bytes;uint64_t value;
 struct slcli_bq_resource_cost cost={0},failure={0};
 size_t slots=r->role==SL_WIN_QUERY_PROCESS_SECURITY?6:r->role==SL_WIN_QUERY_TOKEN_ACCESS?2:1;
 if(!aligned(p->cases_original,_Alignof(sl_win_query_principal_case))||!array(p->case_count,sizeof(*p->cases_original),&bytes)||!covered(r,p->cases_original,bytes))return 0;
 if(p->case_count%slots||(p->case_count-1)/slots>UINT32_MAX)return 0;
 *dimension=0;
 for(size_t i=0;i<p->case_count;i++){
  if(p->cases_original[i].slot!=i%slots||p->cases_original[i].scenario!=i/slots||
     !principal_case(r,&p->cases_original[i],&value))return 0;
  if(!p->cases_original[i].cost.native_attempts||!cost_add(&cost,&p->cases_original[i].cost)||
     !cost_add(&failure,&p->cases_original[i].failed_overlap_cost))return 0;
  if(value>*dimension)*dimension=value;
 }
 return *dimension!=0&&cost_covers(&r->cost,&cost)&&cost_covers(&r->failed_overlap_cost,&failure);
}
static int name(const sl_win_query_semantic_row *r,const WCHAR *p,size_t units){
 size_t bytes;if(!aligned(p,_Alignof(WCHAR))||units<2||!array(units,sizeof(*p),&bytes)||!covered(r,p,bytes)||p[units-1])return 0;
 for(size_t i=0;i+1<units;i++)if(!p[i])return 0;
 return 1;
}
static int image(const sl_win_query_semantic_row *r,uint64_t *dimension){
 const sl_win_query_image_geometry *g=&r->source.image;size_t bytes;
 uint64_t module_units=0,mapped_units=0,file_bytes=0,memory_bytes=0,modules=0;
 if(!declared(r,g->native_layout_source)||!g->mapping_count||g->mapping_count-1>UINT32_MAX||!aligned(g->members_original,_Alignof(sl_win_query_image_member))||!array(g->member_count,sizeof(*g->members_original),&bytes)||
    !covered(r,g->members_original,bytes)||!aligned(g->mappings_original,_Alignof(sl_win_query_mapping_source))||!array(g->mapping_count,sizeof(*g->mappings_original),&bytes)||
    !covered(r,g->mappings_original,bytes)||!array(g->loader_writer_count,sizeof(*g->loader_writer_original),&bytes)||
    !covered(r,g->loader_writer_original,bytes))return 0;
 for(size_t i=0;i<g->loader_writer_count;i++)if(!declared(r,&g->loader_writer_original[i]))return 0;
 for(size_t i=0;i<g->member_count;i++){
  const sl_win_query_image_member *m=&g->members_original[i];const IMAGE_NT_HEADERS64 *h=m->headers_original;
  const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER *)m->file_bytes_original;
  if(!declared(r,m->source_original)||!aligned(m->file_original,_Alignof(struct slcli_build_held_file_windows))||!covered(r,m->file_original,sizeof(*m->file_original))||
     !slcli_build_held_file_windows_matches_original(m->file_original)||
     m->file_original->body_original!=m->file_bytes_original||m->file_original->used_bytes!=m->file_extent||
     !covered(r,m->file_bytes_original,m->file_extent)||!aligned(dos,_Alignof(IMAGE_DOS_HEADER))||!contains(m->file_bytes_original,m->file_extent,dos,sizeof(*dos))||
     dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||(size_t)dos->e_lfanew!=m->headers_offset||
     !aligned(h,_Alignof(IMAGE_NT_HEADERS64))||!contains(m->file_bytes_original,m->file_extent,h,sizeof(*h))||
     (uintptr_t)h-(uintptr_t)m->file_bytes_original!=m->headers_offset||h->Signature!=IMAGE_NT_SIGNATURE||
     h->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||h->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||
     h->FileHeader.SizeOfOptionalHeader!=sizeof(h->OptionalHeader)||h->FileHeader.NumberOfSections!=m->section_count||
     !h->OptionalHeader.SizeOfImage||!aligned(m->sections_original,_Alignof(IMAGE_SECTION_HEADER))||!array(m->section_count,sizeof(*m->sections_original),&bytes)||
     !contains(m->file_bytes_original,m->file_extent,m->sections_original,bytes)||
     m->headers_offset>SIZE_MAX-sizeof(*h)||m->sections_offset!=m->headers_offset+sizeof(*h)||
     (uintptr_t)m->sections_original-(uintptr_t)m->file_bytes_original!=m->sections_offset||
     !name(r,m->module_name_original,m->module_name_units)||!name(r,m->mapped_name_original,m->mapped_name_units))return 0;
  for(size_t j=0;j<m->section_count;j++){
   const IMAGE_SECTION_HEADER *s=&m->sections_original[j];
   if(s->PointerToRawData>m->file_extent||s->SizeOfRawData>m->file_extent-s->PointerToRawData||
      s->VirtualAddress>h->OptionalHeader.SizeOfImage||s->Misc.VirtualSize>h->OptionalHeader.SizeOfImage-s->VirtualAddress)return 0;
  }
  if(m->module_name_units>module_units)module_units=m->module_name_units;
  if(m->mapped_name_units>mapped_units)mapped_units=m->mapped_name_units;
  if(!add(&file_bytes,m->file_extent))return 0;
 }
 for(size_t i=0;i<g->mapping_count;i++){
  const sl_win_query_mapping_source *m=&g->mappings_original[i];size_t index;
  if(!declared(r,m->source_original)||m->kind>SL_WIN_QUERY_MAPPING_DATA_FILE||m->occurrence!=i||
     !aligned(m->geometry_original,_Alignof(MEMORY_BASIC_INFORMATION))||!covered(r,m->geometry_original,sizeof(*m->geometry_original))||
     !range(m->geometry_original->BaseAddress,m->geometry_original->RegionSize))return 0;
  for(index=0;index<g->member_count;index++)if(m->member_original==&g->members_original[index])break;
  if(index==g->member_count||!add(&memory_bytes,m->geometry_original->RegionSize))return 0;
  if(m->kind==SL_WIN_QUERY_MAPPING_IMAGE){
   int first=1;
   if(!contains(m->geometry_original->AllocationBase,m->member_original->headers_original->OptionalHeader.SizeOfImage,
      m->geometry_original->BaseAddress,m->geometry_original->RegionSize))return 0;
   for(size_t j=0;j<i;j++)if(g->mappings_original[j].kind==SL_WIN_QUERY_MAPPING_IMAGE&&
      g->mappings_original[j].geometry_original->AllocationBase==m->geometry_original->AllocationBase){
    if(g->mappings_original[j].member_original!=m->member_original)return 0;
    first=0;
   }
   if(first&&!add(&modules,1))return 0;
  }
 }
 switch(r->role){
 case SL_WIN_QUERY_ARGUMENT_MODULE:*dimension=module_units;break;
 case SL_WIN_QUERY_IMAGE_MODULES:*dimension=modules;break;
 case SL_WIN_QUERY_IMAGE_NAME:*dimension=mapped_units;break;
 case SL_WIN_QUERY_IMAGE_FILE:*dimension=file_bytes;break;
 case SL_WIN_QUERY_IMAGE_MEMORY:*dimension=memory_bytes;break;
 case SL_WIN_QUERY_IMAGE_REGIONS:*dimension=g->mapping_count;break;
 default:return 0;
 }
 if(!*dimension)return 0;
 if((r->role==SL_WIN_QUERY_ARGUMENT_MODULE||r->role==SL_WIN_QUERY_IMAGE_NAME)&&*dimension>MAXDWORD)return 0;
 if(r->role==SL_WIN_QUERY_IMAGE_MODULES&&*dimension>MAXDWORD/sizeof(HMODULE))return 0;
 return 1;
}
static int writer(const sl_win_query_semantic_row *r,uint64_t *dimension){
 const sl_win_query_writer_schedule *w=&r->source.writer;size_t bytes;uint64_t stack=0,pad,turns=0;unsigned seen=0;
 if(!w->step_count||w->step_count-1>UINT32_MAX||!declared(r,w->worker_entry_source)||!declared(r,w->helper_closure_source)||!declared(r,w->tls_source)||
    !declared(r,w->native_layout_source)||!aligned(w->steps_original,_Alignof(sl_win_query_writer_occurrence))||!array(w->step_count,sizeof(*w->steps_original),&bytes)||
    !covered(r,w->steps_original,bytes)||!w->alignment_bytes||(w->alignment_bytes&(w->alignment_bytes-1))||
    !w->native_reservation_granule||(w->native_reservation_granule&(w->native_reservation_granule-1))||
    w->alignment_bytes>w->native_reservation_granule||!w->fixed_frame_bytes||!w->maximum_nested_helper_bytes)return 0;
 for(size_t i=0;i<w->step_count;i++){
  const sl_win_query_writer_occurrence *s=&w->steps_original[i];
  if(s->ordinal!=i||s->kind>SL_WIN_QUERY_SETTLE||!declared(r,s->source_original)||
     (!i&&s->turn)||(i&&s->turn!=w->steps_original[i-1].turn&&s->turn!=(uint64_t)w->steps_original[i-1].turn+1))return 0;
  seen|=1u<<s->kind;
  turns=(uint64_t)s->turn+1;
 }
 if(seen!=15u||!add(&stack,w->abi_shadow_bytes)||!add(&stack,w->fixed_frame_bytes)||!add(&stack,w->maximum_nested_helper_bytes))return 0;
 pad=(0-stack)&(w->alignment_bytes-1);if(!add(&stack,pad))return 0;
 pad=(0-stack)&(w->native_reservation_granule-1);if(!add(&stack,pad))return 0;
 *dimension=r->role==SL_WIN_QUERY_WRITER_STACK?stack:turns;
 return *dimension&&*dimension<=SIZE_MAX;
}
static int source_shape(const sl_win_query_profile_source *s){
 size_t bytes;
 if(!aligned(s,_Alignof(sl_win_query_profile_source))||!s->initial_source_original||s->row_count!=SL_WIN_QUERY_ROLE_COUNT||
    !array(s->row_count,sizeof(*s->rows_original),&bytes)||!range(s->rows_original,bytes)||
    (uintptr_t)s->rows_original%_Alignof(sl_win_query_semantic_row))return 0;
 for(size_t i=0;i<s->row_count;i++){
  const sl_win_query_semantic_row *r=&s->rows_original[i];
  if(r->role!=i||r->arm!=arm(r->role)||!array(r->input_count,sizeof(*r->inputs_original),&bytes)||
     !range(r->inputs_original,bytes)||(uintptr_t)r->inputs_original%_Alignof(struct slcli_product_initial_windows_source_span))return 0;
  for(size_t j=0;j<r->input_count;j++)if(!span(&r->inputs_original[j]))return 0;
  if(!declared(r,r->source_original))return 0;
 }
 return 1;
}
static int any_covered(const sl_win_query_profile_source *s,const void *p,size_t n){
 for(size_t i=0;i<s->row_count;i++)if(covered(&s->rows_original[i],p,n))return 1;
 return 0;
}
static int association(const sl_win_query_profile_source *s,const sl_win_query_association_bytes *a){
 if(!span(a->source_original)||!any_covered(s,a->original,a->extent)||
    !contains(a->source_original->original,a->source_original->bytes,a->original,a->extent))return 0;
 for(size_t i=0;i<s->row_count;i++)if(declared(&s->rows_original[i],a->source_original))return 1;
 return 0;
}
int sl_win_query_source_excludes_output(const sl_win_query_profile_source *s,const void *out,size_t n){
 if(!source_shape(s)||!range(out,n)||!away(out,n,s,sizeof(*s))||
    !away(out,n,s->rows_original,s->row_count*sizeof(*s->rows_original))||
    !slcli_product_windows_initial_source_excludes_output(s->initial_source_original,out,n))return 0;
 for(size_t i=0;i<s->row_count;i++){
  const sl_win_query_semantic_row *r=&s->rows_original[i];
  if(!away(out,n,r->inputs_original,r->input_count*sizeof(*r->inputs_original)))return 0;
  for(size_t j=0;j<r->input_count;j++)if(!span_away(&r->inputs_original[j],out,n))return 0;
  if(r->arm==SL_WIN_QUERY_PRINCIPAL){
   const sl_win_query_principal_geometry *g=&r->source.principal;size_t bytes;
   if(!aligned(g->cases_original,_Alignof(sl_win_query_principal_case))||
      !array(g->case_count,sizeof(*g->cases_original),&bytes)||!covered(r,g->cases_original,bytes))return 0;
   for(size_t j=0;j<g->case_count;j++){
    const sl_win_query_principal_case *v=&g->cases_original[j];
    if(!aligned(v->effective_original,_Alignof(struct slcli_build_principal_windows))||
       !aligned(v->primary_original,_Alignof(struct slcli_build_principal_windows))||
       !covered(r,v->effective_original,sizeof(*v->effective_original))||!covered(r,v->primary_original,sizeof(*v->primary_original))||
       !slcli_build_principal_windows_output_away_original(v->effective_original,out,n)||
       !slcli_build_principal_windows_output_away_original(v->primary_original,out,n))return 0;
   }
  }else if(r->arm==SL_WIN_QUERY_IMAGE){
   const sl_win_query_image_geometry *g=&r->source.image;size_t bytes;
   if(!aligned(g->members_original,_Alignof(sl_win_query_image_member))||
      !array(g->member_count,sizeof(*g->members_original),&bytes)||!covered(r,g->members_original,bytes))return 0;
   for(size_t j=0;j<g->member_count;j++){
    const struct slcli_build_held_file_windows *f=g->members_original[j].file_original;
    if(!aligned(f,_Alignof(struct slcli_build_held_file_windows))||!covered(r,f,sizeof(*f))||
       !slcli_build_held_file_windows_output_away_original(f,out,n))return 0;
   }
  }
 }
 if(!association(s,&s->provider_original)||!away(out,n,s->provider_original.original,s->provider_original.extent)||
    s->source_commit_extent!=40||!any_covered(s,s->source_commit_original,40)||!away(out,n,s->source_commit_original,40))return 0;
 for(size_t i=0;i<SL_WIN_QUERY_ASSOCIATION_COUNT;i++)if(!association(s,&s->associations[i])||
    !away(out,n,s->associations[i].original,s->associations[i].extent))return 0;
 return 1;
}
static void copy(void *out,const void *in,size_t n){unsigned char *a=out;const unsigned char *b=in;for(size_t i=0;i<n;i++)a[i]=b[i];}
static void part(source_span *s,unsigned index,const void **p,size_t *n){
 if(!index){*p=s->original;*n=s->bytes;}
 else if(index==1){*p=s->actor_original;*n=s->actor_bytes;}
 else{*p=s->occurrence_original;*n=s->occurrence_bytes;}
}
static int snapshot_geometry(const sl_win_query_derivation *p,size_t *count,size_t *bytes){
 const sl_win_query_profile_source *s=p->source_original;size_t n=0,all=0,expected=0;
 for(size_t i=0;i<s->row_count;i++){
  if(s->rows_original[i].input_count>(SIZE_MAX-expected)/3)return 0;
  expected+=3*s->rows_original[i].input_count;
 }
 if(expected!=p->snapshot_capacity||expected>SIZE_MAX/sizeof(*p->snapshots_original)||
    !aligned(p->snapshots_original,_Alignof(sl_win_query_input_snapshot))||
    !range(p->snapshots_original,expected*sizeof(*p->snapshots_original)))return 0;
 for(size_t i=0;i<s->row_count;i++){
  const sl_win_query_semantic_row *r=&s->rows_original[i];
  for(size_t j=0;j<r->input_count;j++)for(unsigned k=0;k<3;k++){
   const void *body;size_t extent;const sl_win_query_input_snapshot *v;
   if(n>=p->snapshot_capacity)return 0;v=&p->snapshots_original[n];part(&r->inputs_original[j],k,&body,&extent);
   if(v->source_original!=&r->inputs_original[j]||v->part!=k||v->capacity!=extent||
      !range(v->bytes_original,extent)||!sl_win_query_source_excludes_output(s,v->bytes_original,extent)||
      !away(v->bytes_original,extent,p,sizeof(*p))||
      !away(v->bytes_original,extent,p->row_copies_original,p->row_capacity*sizeof(*p->row_copies_original))||
      !away(v->bytes_original,extent,p->snapshots_original,p->snapshot_capacity*sizeof(*p->snapshots_original))||
      extent>SIZE_MAX-all)return 0;
   for(size_t prior=0;prior<n;prior++)if(!away(v->bytes_original,extent,p->snapshots_original[prior].bytes_original,p->snapshots_original[prior].capacity))return 0;
   all+=extent;n++;
  }
 }
 *count=n;*bytes=all;return n==p->snapshot_capacity;
}
int sl_win_derive_query_profile(sl_win_query_derivation *p){
 const sl_win_query_profile_source *s;size_t row_bytes,snapshot_bytes,count,retained;
 if(!aligned(p,_Alignof(sl_win_query_derivation))||p->attempted||p->returned||!(s=p->source_original)||!source_shape(s)||
    p->row_capacity!=s->row_count||!array(p->row_capacity,sizeof(*p->row_copies_original),&row_bytes)||
    !array(p->snapshot_capacity,sizeof(*p->snapshots_original),&snapshot_bytes)||
    !aligned(p->row_copies_original,_Alignof(sl_win_query_semantic_row))||!aligned(p->snapshots_original,_Alignof(sl_win_query_input_snapshot))||
    !sl_win_query_source_excludes_output(s,p,sizeof(*p))||
    !sl_win_query_source_excludes_output(s,p->row_copies_original,row_bytes)||
    !sl_win_query_source_excludes_output(s,p->snapshots_original,snapshot_bytes)||
    !away(p,sizeof(*p),p->row_copies_original,row_bytes)||!away(p,sizeof(*p),p->snapshots_original,snapshot_bytes)||
    !away(p->row_copies_original,row_bytes,p->snapshots_original,snapshot_bytes)||
    !snapshot_geometry(p,&count,&retained))return 0;
 /* Semantic traversal happens before ANY output write. All typed pointees are
  * required to be covered by the declared original input spans above. */
 uint64_t dimensions[SL_WIN_QUERY_ROLE_COUNT]={0};
 struct slcli_bq_resource_cost cost={0},failure={0};
 for(size_t i=0;i<s->row_count;i++){
  const sl_win_query_semantic_row *r=&s->rows_original[i];
  int ok=r->arm==SL_WIN_QUERY_PRINCIPAL?principal(r,&dimensions[i]):
         r->arm==SL_WIN_QUERY_IMAGE?image(r,&dimensions[i]):writer(r,&dimensions[i]);
  if(!ok||!r->cost.user_ram_bytes||!r->cost.user_virtual_bytes||!r->cost.pure_attempts||!r->cost.cpu_nanoseconds||
     !cost_add(&cost,&r->cost)||!cost_add(&failure,&r->failed_overlap_cost))return 0;
 }
 if(retained>UINT64_MAX-row_bytes||retained+row_bytes>UINT64_MAX-snapshot_bytes||
    cost.user_ram_bytes<retained+row_bytes+snapshot_bytes||cost.user_virtual_bytes<cost.user_ram_bytes)return 0;
 if(p->actual_row_copies||p->actual_snapshots||!zero(&p->source_snapshot,sizeof(p->source_snapshot))||!zero(p->row_copies_original,row_bytes)||
    !zero(p->dimensions,sizeof(p->dimensions))||!zero(&p->simultaneous,sizeof(p->simultaneous))||
    !zero(&p->failed_overlap,sizeof(p->failed_overlap))||p->snapshot_count||p->failed_row||p->result)return 0;
 for(size_t i=0;i<count;i++)if(!zero(&p->snapshots_original[i].source_snapshot,sizeof(p->snapshots_original[i].source_snapshot))||
    !zero(p->snapshots_original[i].bytes_original,p->snapshots_original[i].capacity))return 0;
 p->attempted=1;
 p->actual_row_copies=p->row_copies_original;p->actual_snapshots=p->snapshots_original;
 copy(&p->source_snapshot,s,sizeof(*s));copy(p->row_copies_original,s->rows_original,row_bytes);
 copy(p->dimensions,dimensions,sizeof(dimensions));copy(&p->simultaneous,&cost,sizeof(cost));copy(&p->failed_overlap,&failure,sizeof(failure));
 for(size_t i=0;i<count;i++){
  sl_win_query_input_snapshot *v=&p->snapshots_original[i];const void *body;size_t extent;
  part(v->source_original,v->part,&body,&extent);copy(&v->source_snapshot,v->source_original,sizeof(v->source_snapshot));copy(v->bytes_original,body,extent);
 }
 p->snapshot_count=count;p->result=1;p->returned=1;return 1;
}
int sl_win_query_derivation_matches_original(const sl_win_query_derivation *p){
 size_t count,retained;
 if(!aligned(p,_Alignof(sl_win_query_derivation))||!p->attempted||!p->returned||!p->result||!source_shape(p->source_original)||
    p->row_copies_original!=p->actual_row_copies||p->snapshots_original!=p->actual_snapshots||
    !same(&p->source_snapshot,p->source_original,sizeof(p->source_snapshot))||
    p->row_capacity!=p->source_original->row_count||
    !same(p->row_copies_original,p->source_original->rows_original,p->row_capacity*sizeof(*p->row_copies_original))||
    p->snapshot_count!=p->snapshot_capacity||!snapshot_geometry(p,&count,&retained)||count!=p->snapshot_count)return 0;
 for(size_t i=0;i<p->snapshot_count;i++){
  const sl_win_query_input_snapshot *v=&p->snapshots_original[i];const void *body;size_t n;
  if(!same(&v->source_snapshot,v->source_original,sizeof(v->source_snapshot))||v->part>2)return 0;
  part(v->source_original,v->part,&body,&n);if(n!=v->capacity||!same(v->bytes_original,body,n))return 0;
 }
 uint64_t dimensions[SL_WIN_QUERY_ROLE_COUNT]={0};struct slcli_bq_resource_cost cost={0},failure={0};
 for(size_t i=0;i<p->source_original->row_count;i++){
  const sl_win_query_semantic_row *r=&p->source_original->rows_original[i];
  int ok=r->arm==SL_WIN_QUERY_PRINCIPAL?principal(r,&dimensions[i]):
         r->arm==SL_WIN_QUERY_IMAGE?image(r,&dimensions[i]):writer(r,&dimensions[i]);
  if(!ok||!cost_add(&cost,&r->cost)||!cost_add(&failure,&r->failed_overlap_cost))return 0;
 }
 return same(dimensions,p->dimensions,sizeof(dimensions))&&same(&cost,&p->simultaneous,sizeof(cost))&&same(&failure,&p->failed_overlap,sizeof(failure));
}
static int derivation_away(const sl_win_query_derivation *d,const void *out,size_t n){
 if(!sl_win_query_source_excludes_output(d->source_original,out,n)||!away(out,n,d,sizeof(*d))||
    !away(out,n,d->row_copies_original,d->row_capacity*sizeof(*d->row_copies_original))||
    !away(out,n,d->snapshots_original,d->snapshot_capacity*sizeof(*d->snapshots_original)))return 0;
 for(size_t i=0;i<d->snapshot_count;i++)if(!away(out,n,d->snapshots_original[i].bytes_original,d->snapshots_original[i].capacity))return 0;
 return 1;
}
typedef struct json_writer {unsigned char *output;size_t capacity,used;unsigned write;} json_writer;
static int character(json_writer *w,unsigned char c){
 if(w->used==SIZE_MAX||w->used>=w->capacity)return 0;
 if(w->write)w->output[w->used]=c;w->used++;return 1;
}
static int literal(json_writer *w,const char *s){for(size_t i=0;s[i];i++)if(!character(w,(unsigned char)s[i]))return 0;return 1;}
static int decimal(json_writer *w,uint64_t value,int quoted){
 unsigned char digits[20];size_t n=0;if(!value)return 0;
 do{digits[n++]=(unsigned char)('0'+value%10);value/=10;}while(value);
 if(quoted&&!character(w,'"'))return 0;
 while(n)if(!character(w,digits[--n]))return 0;
 return !quoted||character(w,'"');
}
static int digest(json_writer *w,const unsigned char *bytes){
 static const unsigned char hex[]="0123456789abcdef";
 for(size_t i=0;i<32;i++)if(!character(w,hex[bytes[i]>>4])||!character(w,hex[bytes[i]&15]))return 0;
 return 1;
}
static int encode(json_writer *w,const sl_win_query_profile_encoding *e){
 const sl_win_query_derivation *d=e->derivation_original;const uint64_t *v=d->dimensions;
 if(!literal(w,"{\"schemaVersion\":3,\"sourceCommit\":\""))return 0;
 for(size_t i=0;i<40;i++)if(!character(w,d->source_original->source_commit_original[i]))return 0;
 if(!literal(w,"\",\"providerProfileSHA256\":\"")||!digest(w,e->hashes_original[0].digest)||!literal(w,"\",\"regions\":{"))return 0;
 static const char *const names[]={"endpointSecurity","processSecurity","tokenAccess"};
 for(size_t i=0;i<3;i++){
  if((i&&!character(w,','))||!character(w,'"')||!literal(w,names[i])||!literal(w,"\":{\"capacity\":")||
     !decimal(w,v[i],0)||!literal(w,",\"originalSHA256\":\"")||!digest(w,e->hashes_original[i+1].digest)||!literal(w,"\"}"))return 0;
 }
 if(!literal(w,"},\"arguments\":{\"moduleUnitsIncludingNUL\":")||!decimal(w,v[SL_WIN_QUERY_ARGUMENT_MODULE],0)||
    !literal(w,",\"originalSHA256\":\"")||!digest(w,e->hashes_original[4].digest)||
    !literal(w,"\"},\"argumentWriter\":{\"stackReservationBytes\":")||!decimal(w,v[SL_WIN_QUERY_WRITER_STACK],1)||
    !literal(w,",\"turnCapacity\":")||!decimal(w,v[SL_WIN_QUERY_WRITER_TURNS],1)||
    !literal(w,",\"originalSHA256\":\"")||!digest(w,e->hashes_original[5].digest)||
    !literal(w,"\"},\"creatorImages\":{\"moduleCapacity\":")||!decimal(w,v[SL_WIN_QUERY_IMAGE_MODULES],1)||
    !literal(w,",\"mappedNameUnitsIncludingNUL\":")||!decimal(w,v[SL_WIN_QUERY_IMAGE_NAME],0)||
    !literal(w,",\"fileBytes\":")||!decimal(w,v[SL_WIN_QUERY_IMAGE_FILE],1)||
    !literal(w,",\"memoryBytes\":")||!decimal(w,v[SL_WIN_QUERY_IMAGE_MEMORY],1)||
    !literal(w,",\"memoryRegionCapacity\":")||!decimal(w,v[SL_WIN_QUERY_IMAGE_REGIONS],1)||
    !literal(w,",\"originalSHA256\":\"")||!digest(w,e->hashes_original[6].digest)||!literal(w,"\"}}\n"))return 0;
 return 1;
}
int sl_win_encode_query_profile(sl_win_query_profile_encoding *e){
 const sl_win_query_derivation *d;size_t hn,wn;json_writer measure;
 if(!aligned(e,_Alignof(sl_win_query_profile_encoding))||e->attempted||e->returned||e->result||e->output_used||
    !(d=e->derivation_original)||!sl_win_query_derivation_matches_original(d)||e->hash_capacity!=SL_WIN_QUERY_ASSOCIATION_COUNT+1||
    !array(e->hash_capacity,sizeof(*e->hashes_original),&hn)||!array(e->hash_capacity,sizeof(*e->workspaces_original),&wn)||
    !aligned(e->hashes_original,_Alignof(sl_owned_sha256))||!aligned(e->workspaces_original,_Alignof(sl_owned_sha256_workspace))||
    !derivation_away(d,e,sizeof(*e))||!derivation_away(d,e->output_original,e->output_capacity)||
    !derivation_away(d,e->hashes_original,hn)||!derivation_away(d,e->workspaces_original,wn)||
    !away(e,sizeof(*e),e->output_original,e->output_capacity)||!away(e,sizeof(*e),e->hashes_original,hn)||
    !away(e,sizeof(*e),e->workspaces_original,wn)||!away(e->output_original,e->output_capacity,e->hashes_original,hn)||
    !away(e->output_original,e->output_capacity,e->workspaces_original,wn)||!away(e->hashes_original,hn,e->workspaces_original,wn)||
    !zero(e->hashes_original,hn)||!zero(e->workspaces_original,wn)||!zero(e->output_original,e->output_capacity))return 0;
 for(size_t i=0;i<40;i++){unsigned char c=d->source_original->source_commit_original[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return 0;}
 measure=(json_writer){NULL,e->output_capacity,0,0};if(!encode(&measure,e))return 0;
 e->attempted=1;
 for(size_t i=0;i<e->hash_capacity;i++){
  const sl_win_query_association_bytes *a=i?&d->source_original->associations[i-1]:&d->source_original->provider_original;
  sl_owned_sha256 *h=&e->hashes_original[i];h->body=a->original;h->extent=a->extent;h->workspace=&e->workspaces_original[i];
  if(sl_owned_sha256_compute(h))goto done;
 }
 json_writer actual={e->output_original,e->output_capacity,0,1};
 if(!encode(&actual,e)||actual.used!=measure.used)goto done;
 e->output_used=actual.used;
 if(!sl_win_query_derivation_matches_original(d))goto done;
 e->result=1;
done:e->returned=1;return e->result;
}
#endif
