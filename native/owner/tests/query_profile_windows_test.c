#ifdef _WIN32
#include "../query_profile_windows.h"
#include "../query_profile_pe_windows.h"
#include <assert.h>

/* Called only by a future admitted genuine-input harness. This file creates
 * no principal, held-file, source, nativecase, clock, arena or reservation.
 * All controls/copies/scratch are separately owned/excluded before invocation.
 * These vectors have NOT been compiled or executed by the author. */
static void clone_bytes(void *out,const void *in,size_t n){unsigned char *a=out;const unsigned char *b=in;for(size_t i=0;i<n;i++)a[i]=b[i];}
static int equal_bytes(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return 0;return 1;}
static void put16(unsigned char *p,uint16_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static uint32_t get32(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int separate(const void *a,size_t an,const void *b,size_t bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an&&bn&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static void source_span(struct slcli_product_initial_windows_source_span *s,const void *p,size_t n,const void *actor,const void *occurrence,size_t on){
 s->original=p;s->bytes=n;s->actor_original=actor;s->actor_bytes=1;s->occurrence_original=occurrence;s->occurrence_bytes=on;
}

/* Pure SDK/source-byte adversaries, NOT genuine source/native provenance.
 * No CLI principal/file/caller is fabricated and no native API is invoked. */
void sl_win_query_ace_geometry_vectors(void){
 _Alignas(8) unsigned char body[128]={0};unsigned char actor=1;
 struct slcli_product_initial_windows_source_span spans[3];sl_win_query_semantic_row row={0};
 sl_win_query_ace_source ace={0};sl_win_query_claim_value_source value={0};
 source_span(&spans[0],body,sizeof(body),&actor,&row,sizeof(row));
 source_span(&spans[1],&ace,sizeof(ace),&actor,&row,sizeof(row));
 source_span(&spans[2],&value,sizeof(value),&actor,&row,sizeof(row));
 row.inputs_original=spans;row.input_count=3;
 body[0]=ACCESS_ALLOWED_ACE_TYPE;put16(body+2,20);body[8]=SID_REVISION;body[9]=1;body[15]=1;
 ace.original=(const ACE_HEADER *)body;ace.bytes=20;ace.source_original=&spans[1];ace.mask_original=(const ACCESS_MASK *)(body+4);
 ace.sid.original=(const SID *)(body+8);ace.sid.bytes=12;ace.sid.source_original=&spans[0];
 assert(sl_win_query_ace_source_matches(&row,&ace));
 const BYTE basic_types[]={ACCESS_ALLOWED_ACE_TYPE,ACCESS_DENIED_ACE_TYPE,SYSTEM_AUDIT_ACE_TYPE,SYSTEM_ALARM_ACE_TYPE,
  SYSTEM_MANDATORY_LABEL_ACE_TYPE,SYSTEM_SCOPED_POLICY_ID_ACE_TYPE,SYSTEM_PROCESS_TRUST_LABEL_ACE_TYPE};
 for(size_t i=0;i<sizeof(basic_types);i++){body[0]=basic_types[i];assert(sl_win_query_ace_source_matches(&row,&ace));}
 body[0]=ACCESS_ALLOWED_ACE_TYPE;
 put16(body+2,4);ace.bytes=4;assert(!sl_win_query_ace_source_matches(&row,&ace)); /* Reviewed4B header-only counterexample. */
 put16(body+2,20);ace.bytes=20;
 ace.mask_original=(const ACCESS_MASK *)(body+8);assert(!sl_win_query_ace_source_matches(&row,&ace));ace.mask_original=(const ACCESS_MASK *)(body+4);
 body[9]=SID_MAX_SUB_AUTHORITIES+1;assert(!sl_win_query_ace_source_matches(&row,&ace));body[9]=1;
 /* Optional object GUID flags move the native SID locator; scalar equality
  * or an arbitrary aligned SID elsewhere in backing cannot replace it. */
 for(size_t i=12;i;i--)body[12+i-1]=body[8+i-1];
 put32(body+8,0);body[0]=ACCESS_ALLOWED_OBJECT_ACE_TYPE;put16(body+2,24);ace.bytes=24;
 ace.object_flags_original=(const DWORD *)(body+8);ace.sid.original=(const SID *)(body+12);
 assert(sl_win_query_ace_source_matches(&row,&ace));
 put32(body+8,ACE_OBJECT_TYPE_PRESENT);assert(!sl_win_query_ace_source_matches(&row,&ace));put32(body+8,0);
 ace.sid.original=(const SID *)(body+16);assert(!sl_win_query_ace_source_matches(&row,&ace));ace.sid.original=(const SID *)(body+12);
 /* Callback payload is retained exact source bytes, not evaluated access. */
 body[0]=ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE;put16(body+2,28);ace.bytes=28;
 ace.payload_original=body+24;ace.payload_extent=4;ace.payload_source=&spans[0];
 assert(sl_win_query_ace_source_matches(&row,&ace));
 ace.payload_original=body+20;assert(!sl_win_query_ace_source_matches(&row,&ace));ace.payload_original=body+24;
 /* Resource-attribute relative values require complete typed payload extents. */
 body[0]=SYSTEM_RESOURCE_ATTRIBUTE_ACE_TYPE;put16(body+2,52);ace.bytes=52;
 for(size_t i=0;i<12;i++)body[8+i]=body[12+i];
 ace.object_flags_original=NULL;ace.sid.original=(const SID *)(body+8);
 ace.payload_original=body+20;ace.payload_extent=32;put32(body+20,20);put16(body+24,CLAIM_SECURITY_ATTRIBUTE_TYPE_UINT64);
 put16(body+26,0);put32(body+28,0);put32(body+32,1);put32(body+36,24);put16(body+40,'x');put16(body+42,0);
 ace.resource_attribute.original=(const CLAIM_SECURITY_ATTRIBUTE_RELATIVE_V1 *)(body+20);
 ace.resource_attribute.name_original=(const WCHAR *)(body+40);ace.resource_attribute.name_units=2;
 ace.resource_attribute.values_original=&value;ace.resource_attribute.value_count=1;
 value.original=body+44;value.bytes=8;value.source_original=&spans[0];
 assert(sl_win_query_ace_source_matches(&row,&ace));
 put32(body+36,31);assert(!sl_win_query_ace_source_matches(&row,&ace));put32(body+36,24);
 put32(body+32,2);assert(!sl_win_query_ace_source_matches(&row,&ace));put32(body+32,1);
 put16(body+24,CLAIM_SECURITY_ATTRIBUTE_TYPE_BOOLEAN);put32(body+44,1);put32(body+48,0);
 assert(sl_win_query_ace_source_matches(&row,&ace));put32(body+44,2);assert(!sl_win_query_ace_source_matches(&row,&ace));
 put16(body+24,CLAIM_SECURITY_ATTRIBUTE_TYPE_STRING);value.bytes=4;put16(body+44,'v');put16(body+46,0);
 assert(sl_win_query_ace_source_matches(&row,&ace));put16(body+46,'z');assert(!sl_win_query_ace_source_matches(&row,&ace));
 put16(body+24,CLAIM_SECURITY_ATTRIBUTE_TYPE_SID);put16(body+2,60);ace.bytes=60;ace.payload_extent=40;value.bytes=16;
 put32(body+44,12);body[48]=SID_REVISION;body[49]=1;
 assert(sl_win_query_ace_source_matches(&row,&ace));body[49]=2;assert(!sl_win_query_ace_source_matches(&row,&ace));body[49]=1;
 put16(body+24,CLAIM_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING);put32(body+44,12);
 assert(sl_win_query_ace_source_matches(&row,&ace));put32(body+44,13);assert(!sl_win_query_ace_source_matches(&row,&ace));
 body[0]=0xff;assert(!sl_win_query_ace_source_matches(&row,&ace)); /* Explicit unknown source case, never header-only acceptance. */
}

void sl_win_query_pe_geometry_vectors(void){
 _Alignas(8) unsigned char file[0x2600]={0};unsigned char actor=1;
 sl_win_query_semantic_row row={0};sl_win_query_image_member member={0};sl_win_query_pe_closure closure={0};
 sl_win_query_pe_import imports[2]={0};sl_win_query_pe_delay delays[2]={0};sl_win_query_pe_tls tls_rows[2]={0};
 sl_win_query_pe_export functions[1]={0},names[1]={0};sl_win_query_pe_relocation relocations[2]={0};
 struct slcli_product_initial_windows_source_span spans[20];
 struct slcli_bq_resource_cost cost={0},failure={0},before;
 IMAGE_NT_HEADERS64 *h=(IMAGE_NT_HEADERS64 *)(file+0x80);IMAGE_SECTION_HEADER *section=(IMAGE_SECTION_HEADER *)(file+0x188);
 put16(file,IMAGE_DOS_SIGNATURE);put32(file+0x3c,0x80);
 h->Signature=IMAGE_NT_SIGNATURE;h->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;h->FileHeader.NumberOfSections=1;
 h->FileHeader.SizeOfOptionalHeader=(WORD)sizeof(h->OptionalHeader);h->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;
 h->OptionalHeader.ImageBase=UINT64_C(0x140000000);h->OptionalHeader.SectionAlignment=0x1000;h->OptionalHeader.FileAlignment=0x200;
 h->OptionalHeader.SizeOfImage=0x2000;h->OptionalHeader.SizeOfHeaders=0x200;h->OptionalHeader.NumberOfRvaAndSizes=16;
 section->VirtualAddress=0x1000;section->Misc.VirtualSize=0x1000;section->PointerToRawData=0x200;section->SizeOfRawData=0x200;
 member.file_bytes_original=file;member.file_extent=sizeof(file);member.headers_original=h;member.headers_offset=0x80;
 member.sections_original=section;member.section_count=1;member.sections_offset=0x188;member.closure_original=&closure;
 closure.member_original=&member;closure.source_original=&spans[2];
 source_span(&spans[0],file,sizeof(file),&actor,&row,sizeof(row));source_span(&spans[1],&member,sizeof(member),&actor,&row,sizeof(row));
 source_span(&spans[2],&closure,sizeof(closure),&actor,&row,sizeof(row));source_span(&spans[3],&imports[0],sizeof(imports[0]),&actor,&row,sizeof(row));
 source_span(&spans[4],&imports[1],sizeof(imports[1]),&actor,&row,sizeof(row));source_span(&spans[5],&actor,1,&actor,&row,sizeof(row));
 source_span(&spans[6],imports,sizeof(imports),&actor,&row,sizeof(row));row.inputs_original=spans;row.input_count=7;
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 h->OptionalHeader.NumberOfRvaAndSizes=17;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));h->OptionalHeader.NumberOfRvaAndSizes=16;
 h->OptionalHeader.ImageBase=UINT64_MAX;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));h->OptionalHeader.ImageBase=UINT64_C(0x140000000);
 h->OptionalHeader.AddressOfEntryPoint=0x2000;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));h->OptionalHeader.AddressOfEntryPoint=0;
 h->OptionalHeader.SectionAlignment=3;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));h->OptionalHeader.SectionAlignment=0x1000;
 section->PointerToRawData=0x2600;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));section->PointerToRawData=0x200;
 section->VirtualAddress=0;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));section->VirtualAddress=0x1000;
 /* Separate mapped-interval and raw-file overlap cases. The second row is
  * an actual SDK source view within the retained file header span. */
 IMAGE_SECTION_HEADER *second=section+1;clone_bytes(second,section,sizeof(*second));
 h->FileHeader.NumberOfSections=2;member.section_count=2;second->PointerToRawData=0x400;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 second->VirtualAddress=0x2000;second->PointerToRawData=0x200;h->OptionalHeader.SizeOfImage=0x3000;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 h->FileHeader.NumberOfSections=1;member.section_count=1;h->OptionalHeader.SizeOfImage=0x2000;
 for(size_t i=0;i<sizeof(*second);i++)((unsigned char *)second)[i]=0;
 h->OptionalHeader.SectionAlignment=0x200;section->VirtualAddress=0x200;
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 h->OptionalHeader.FileAlignment=0x400;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 h->OptionalHeader.FileAlignment=0x200;h->OptionalHeader.SectionAlignment=0x1000;section->VirtualAddress=0x1000;
 /* SECURITY is a file offset: a valid table beyond SizeOfImage is not an RVA. */
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]=(IMAGE_DATA_DIRECTORY){0x2400,8};
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]=(IMAGE_DATA_DIRECTORY){0x2400,20};before=cost;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));assert(equal_bytes(&cost,&before,sizeof(cost)));
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]=(IMAGE_DATA_DIRECTORY){0x1000,40};
 /* Nonempty table cannot omit its genuine source descriptor terminator. */
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 closure.imports_original=imports;closure.import_count=1;
 for(size_t i=0;i<2;i++){
  imports[i].source.source_original=&spans[3+i];imports[i].source.interpretation_original=&spans[5];
  imports[i].source.cost.user_ram_bytes=sizeof(imports[i]);imports[i].source.cost.user_virtual_bytes=sizeof(imports[i]);
  imports[i].source.cost.pure_attempts=1;imports[i].source.cost.cpu_nanoseconds=1;
 }
 imports[0].descriptor_original=(const IMAGE_IMPORT_DESCRIPTOR *)(file+0x200);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 /* A real source descriptor with a zero thunk still needs its terminal IAT
  * locator and subsequent descriptor terminal; a guessed row is insufficient. */
 put32(file+0x200,0x1040);put32(file+0x20c,0x1060);put32(file+0x210,0x1050);file[0x260]='x';
 imports[0].lookup_original=(const ULONGLONG *)(file+0x240);imports[0].module_original=file+0x260;imports[0].module_bytes=2;imports[0].iat_rva=0x1050;
 before=cost;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));assert(equal_bytes(&cost,&before,sizeof(cost)));
 closure.import_count=2;imports[1].descriptor_index=1;imports[1].descriptor_original=(const IMAGE_IMPORT_DESCRIPTOR *)(file+0x214);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put32(file+0x20c,0);imports[0].module_original=file;imports[0].module_bytes=3;before=cost;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));assert(equal_bytes(&cost,&before,sizeof(cost)));
 put32(file+0x20c,0x1060);imports[0].module_original=file+0x260;imports[0].module_bytes=2;
 imports[0].iat_rva=0x1058;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));imports[0].iat_rva=0x1050;
 imports[1].descriptor_index=2;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));imports[1].descriptor_index=1;
 /* Failure/cost outputs cannot alias retained input bytes; refusal writes none. */
 uint32_t original=get32(file);assert(!sl_win_query_pe_source_matches(&row,&member,(struct slcli_bq_resource_cost *)file,&failure));assert(get32(file)==original);
 /* Each additional typed projection has its own source row and explicit
  * array extent. These are pure SDK geometry vectors, not native originals. */
#define PE_ROW(value,slot) do { \
 source_span(&spans[slot],&(value),sizeof(value),&actor,&row,sizeof(row)); \
 (value).source.source_original=&spans[slot];(value).source.interpretation_original=&spans[5]; \
 (value).source.cost.user_ram_bytes=sizeof(value);(value).source.cost.user_virtual_bytes=sizeof(value); \
 (value).source.cost.pure_attempts=1;(value).source.cost.cpu_nanoseconds=1; \
 } while(0)
 PE_ROW(delays[0],7);PE_ROW(delays[1],8);PE_ROW(tls_rows[0],9);PE_ROW(tls_rows[1],10);
 PE_ROW(functions[0],11);PE_ROW(names[0],12);PE_ROW(relocations[0],13);PE_ROW(relocations[1],14);
 source_span(&spans[15],delays,sizeof(delays),&actor,&row,sizeof(row));
 source_span(&spans[16],tls_rows,sizeof(tls_rows),&actor,&row,sizeof(row));
 source_span(&spans[17],relocations,sizeof(relocations),&actor,&row,sizeof(row));row.input_count=18;
 closure.imports_original=NULL;closure.import_count=0;
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]=(IMAGE_DATA_DIRECTORY){0,0};
 for(size_t i=0x200;i<0x400;i++)file[i]=0;
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]=(IMAGE_DATA_DIRECTORY){0x1000,64};
 closure.delays_original=delays;closure.delay_count=1;delays[0].descriptor_original=(const IMAGE_DELAYLOAD_DESCRIPTOR *)(file+0x200);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put32(file+0x200,1);put32(file+0x204,0x1080);put32(file+0x208,0x1090);put32(file+0x20c,0x10a0);put32(file+0x210,0x10b0);file[0x280]='d';
 delays[0].lookup_original=(const ULONGLONG *)(file+0x2b0);delays[0].module_rva=0x1090;delays[0].iat_rva=0x10a0;
 delays[0].module_original=file+0x280;delays[0].module_bytes=2;before=cost;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));assert(equal_bytes(&cost,&before,sizeof(cost)));
 closure.delay_count=2;delays[1].descriptor_index=1;delays[1].descriptor_original=(const IMAGE_DELAYLOAD_DESCRIPTOR *)(file+0x220);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 delays[0].module_rva++;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));delays[0].module_rva--;
 put32(file+0x200,2);assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));put32(file+0x200,1);
 closure.delays_original=NULL;closure.delay_count=0;h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]=(IMAGE_DATA_DIRECTORY){0,0};
 for(size_t i=0x200;i<0x400;i++)file[i]=0;
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS]=(IMAGE_DATA_DIRECTORY){0x1000,40};
 closure.tls_original=tls_rows;closure.tls_count=1;tls_rows[0].directory_original=(const IMAGE_TLS_DIRECTORY64 *)(file+0x200);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 IMAGE_TLS_DIRECTORY64 *td=(IMAGE_TLS_DIRECTORY64 *)(file+0x200);td->AddressOfCallBacks=h->OptionalHeader.ImageBase+0x1080;
 tls_rows[0].callback_original=(const ULONGLONG *)(file+0x280);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 *(ULONGLONG *)(file+0x280)=h->OptionalHeader.ImageBase+0x1100;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 closure.tls_count=2;tls_rows[1].directory_original=td;tls_rows[1].callback_index=1;tls_rows[1].callback_original=(const ULONGLONG *)(file+0x288);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 td->AddressOfIndex=h->OptionalHeader.ImageBase+0x1200;
 for(size_t i=0;i<2;i++){tls_rows[i].index_rva=0x1200;tls_rows[i].index_kind=2;}
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 td->AddressOfIndex=h->OptionalHeader.ImageBase+0x1ffe;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 td->AddressOfIndex=0;for(size_t i=0;i<2;i++){tls_rows[i].index_rva=0;tls_rows[i].index_kind=0;}
 td->StartAddressOfRawData=h->OptionalHeader.ImageBase+0x1100;td->EndAddressOfRawData=td->StartAddressOfRawData+4;
 for(size_t i=0;i<2;i++){tls_rows[i].template_original=file+0x300;tls_rows[i].template_bytes=4;}
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 tls_rows[1].callback_index=0;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));tls_rows[1].callback_index=1;
 td->EndAddressOfRawData=h->OptionalHeader.ImageBase;td->StartAddressOfRawData=td->EndAddressOfRawData+1;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 closure.tls_original=NULL;closure.tls_count=0;h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS]=(IMAGE_DATA_DIRECTORY){0,0};
 for(size_t i=0x200;i<0x400;i++)file[i]=0;
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]=(IMAGE_DATA_DIRECTORY){0x1000,128};
 IMAGE_EXPORT_DIRECTORY *ed=(IMAGE_EXPORT_DIRECTORY *)(file+0x200);ed->NumberOfFunctions=1;ed->NumberOfNames=1;
 ed->AddressOfFunctions=0x1080;ed->AddressOfNames=0x1084;ed->AddressOfNameOrdinals=0x1088;
 put32(file+0x280,0x1060);put32(file+0x284,0x1090);file[0x260]='f';file[0x290]='n';
 closure.functions_original=functions;closure.function_count=1;closure.names_original=names;closure.name_count=1;
 functions[0].directory_original=ed;functions[0].function_original=(const DWORD *)(file+0x280);
 functions[0].forwarder_original=file+0x260;functions[0].forwarder_bytes=2;
 names[0].directory_original=ed;names[0].function_original=functions[0].function_original;names[0].named=1;
 names[0].name_rva_original=(const DWORD *)(file+0x284);names[0].ordinal_original=(const WORD *)(file+0x288);
 names[0].name_original=file+0x290;names[0].name_bytes=2;names[0].forwarder_original=file+0x260;names[0].forwarder_bytes=2;
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put32(file+0x280,0);functions[0].forwarder_original=NULL;functions[0].forwarder_bytes=0;names[0].forwarder_original=NULL;names[0].forwarder_bytes=0;
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put32(file+0x280,0x1060);functions[0].forwarder_original=file+0x260;functions[0].forwarder_bytes=2;names[0].forwarder_original=file+0x260;names[0].forwarder_bytes=2;
 put16(file+0x288,1);assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));put16(file+0x288,0);
 put32(file+0x284,0);names[0].name_original=file;names[0].name_bytes=3;before=cost;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));assert(equal_bytes(&cost,&before,sizeof(cost)));
 put32(file+0x284,0x1090);names[0].name_original=file+0x290;names[0].name_bytes=2;
 functions[0].forwarder_original=file+0x261;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));functions[0].forwarder_original=file+0x260;
 ed->AddressOfFunctions=0x2000;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));ed->AddressOfFunctions=0x1080;
 closure.functions_original=NULL;closure.function_count=0;closure.names_original=NULL;closure.name_count=0;
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]=(IMAGE_DATA_DIRECTORY){0,0};
 for(size_t i=0x200;i<0x400;i++)file[i]=0;
 h->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]=(IMAGE_DATA_DIRECTORY){0x1000,12};
 put32(file+0x200,0x1000);put32(file+0x204,12);put16(file+0x208,(WORD)(IMAGE_REL_BASED_DIR64<<12|0x80));
 closure.relocations_original=relocations;closure.relocation_count=2;
 for(size_t i=0;i<2;i++){relocations[i].block_original=(const IMAGE_BASE_RELOCATION *)(file+0x200);relocations[i].entry_index=i;relocations[i].entry_original=(const WORD *)(file+0x208+i*2);}
 relocations[0].target_original=file+0x280;relocations[0].target_kind=1;
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put16(file+0x208,(WORD)(IMAGE_REL_BASED_DIR64<<12|0x200));relocations[0].target_original=NULL;relocations[0].target_kind=2;
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put16(file+0x208,(WORD)(IMAGE_REL_BASED_DIR64<<12|0x80));relocations[0].target_original=file+0x280;relocations[0].target_kind=1;
 closure.relocation_count=1;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));closure.relocation_count=2;
 relocations[0].target_original=file+0x288;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));relocations[0].target_original=file+0x280;
 put16(file+0x208,(WORD)(IMAGE_REL_BASED_HIGHLOW<<12|0x80));assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put16(file+0x208,(WORD)(IMAGE_REL_BASED_DIR64<<12|0x80));
 before=cost;struct slcli_bq_resource_cost failed_before=failure;
 const struct slcli_product_initial_windows_source_span *interpretation=relocations[0].source.interpretation_original;
 relocations[0].source.interpretation_original=&spans[18];
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 assert(equal_bytes(&cost,&before,sizeof(cost))&&equal_bytes(&failure,&failed_before,sizeof(failure)));
 relocations[0].source.interpretation_original=interpretation;
 uint64_t ram=relocations[0].source.cost.user_ram_bytes,virtual_bytes=relocations[0].source.cost.user_virtual_bytes;
 relocations[0].source.cost.user_ram_bytes=UINT64_MAX;relocations[0].source.cost.user_virtual_bytes=UINT64_MAX;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 assert(equal_bytes(&cost,&before,sizeof(cost))&&equal_bytes(&failure,&failed_before,sizeof(failure)));
 relocations[0].source.cost.user_ram_bytes=ram;relocations[0].source.cost.user_virtual_bytes=virtual_bytes;
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&cost));
 assert(equal_bytes(&cost,&before,sizeof(cost)));
 /* Aligned foreign SDK views are not accepted by matching their scalars. */
 member.headers_original=(const IMAGE_NT_HEADERS64 *)(file+0x88);assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));member.headers_original=h;
 member.sections_offset+=sizeof(IMAGE_SECTION_HEADER);assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 /* Independent lower-header counterexample: h8 and ImageBase0x800000000
  * overlap DOS e_lfanew with the high image-base DWORD, retaining value8.
  * All SDK views are aligned and contained; range denial is indispensable. */
 _Alignas(8) unsigned char early[0x400]={0};IMAGE_NT_HEADERS64 selected=*h;
 for(size_t i=0;i<IMAGE_NUMBEROF_DIRECTORY_ENTRIES;i++)selected.OptionalHeader.DataDirectory[i]=(IMAGE_DATA_DIRECTORY){0,0};
 selected.OptionalHeader.ImageBase=UINT64_C(0x800000000);
 clone_bytes(early+8,&selected,sizeof(selected));clone_bytes(early+8+sizeof(selected),section,sizeof(*section));put16(early,IMAGE_DOS_SIGNATURE);
 member.file_bytes_original=early;member.file_extent=sizeof(early);member.headers_original=(const IMAGE_NT_HEADERS64 *)(early+8);member.headers_offset=8;
 member.sections_original=(const IMAGE_SECTION_HEADER *)(early+8+sizeof(selected));member.sections_offset=8+sizeof(selected);
 source_span(&spans[0],early,sizeof(early),&actor,&row,sizeof(row));closure.relocations_original=NULL;closure.relocation_count=0;
 before=cost;failed_before=failure;assert(get32(early+0x3c)==8);
 assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 assert(equal_bytes(&cost,&before,sizeof(cost))&&equal_bytes(&failure,&failed_before,sizeof(failure)));
 /* Minimum nonoverlapping DOS boundary64 is valid, not a blanket denial
  * of low NT-header offsets. A high-bit signed offset is always invalid. */
 selected.OptionalHeader.ImageBase=UINT64_C(0x140000000);clone_bytes(early+64,&selected,sizeof(selected));
 clone_bytes(early+64+sizeof(selected),section,sizeof(*section));put32(early+0x3c,64);
 member.headers_original=(const IMAGE_NT_HEADERS64 *)(early+64);member.headers_offset=64;
 member.sections_original=(const IMAGE_SECTION_HEADER *)(early+64+sizeof(selected));member.sections_offset=64+sizeof(selected);
 assert(sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put32(early+0x3c,63);member.headers_offset=63;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));
 put32(early+0x3c,UINT32_C(0x80000000));member.headers_offset=UINT32_C(0x80000000);
 before=cost;assert(!sl_win_query_pe_source_matches(&row,&member,&cost,&failure));assert(equal_bytes(&cost,&before,sizeof(cost)));
#undef PE_ROW
}

void sl_win_query_profile_source_refusal_vectors(
 const sl_win_query_profile_source *genuine,
 sl_win_query_semantic_row copies[SL_WIN_QUERY_ROLE_COUNT],
 sl_win_query_derivation *fresh,unsigned char *before,size_t before_extent){
 sl_win_query_profile_source changed;
 assert(genuine&&genuine->row_count==SL_WIN_QUERY_ROLE_COUNT&&copies&&fresh&&before);
 assert(before_extent==sizeof(*fresh));
 assert(sl_win_query_source_excludes_output(genuine,copies,sizeof(*copies)*SL_WIN_QUERY_ROLE_COUNT));
 assert(sl_win_query_source_excludes_output(genuine,&changed,sizeof(changed)));
 assert(sl_win_query_source_excludes_output(genuine,before,before_extent));
 clone_bytes(&changed,genuine,sizeof(changed));
 clone_bytes(copies,genuine->rows_original,sizeof(*copies)*SL_WIN_QUERY_ROLE_COUNT);
 changed.rows_original=copies;
 const sl_win_query_profile_source *original_source=fresh->source_original;
 fresh->source_original=&changed;
 /* A wrong role/arm is refused before hash/output or any native occurrence. */
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++){
  unsigned old=copies[i].arm;copies[i].arm=3;
  clone_bytes(before,fresh,sizeof(*fresh));
  assert(!sl_win_derive_query_profile(fresh));assert(equal_bytes(before,fresh,sizeof(*fresh)));
  copies[i].arm=old;
 }
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++){
  unsigned old=copies[i].role;copies[i].role=(unsigned)((i+1)%SL_WIN_QUERY_ROLE_COUNT);
  clone_bytes(before,fresh,sizeof(*fresh));
  assert(!sl_win_derive_query_profile(fresh));assert(equal_bytes(before,fresh,sizeof(*fresh)));
  copies[i].role=old;
 }
 changed.row_count=SL_WIN_QUERY_ROLE_COUNT-1;
 clone_bytes(before,fresh,sizeof(*fresh));
 assert(!sl_win_derive_query_profile(fresh));assert(equal_bytes(before,fresh,sizeof(*fresh)));
 changed.row_count=SL_WIN_QUERY_ROLE_COUNT;
 /* Exact declared transitive source byte/actor/occurrence spans are all
  * forbidden outputs, even when they alias a larger source-selected range. */
 for(size_t i=0;i<genuine->row_count;i++)for(size_t j=0;j<genuine->rows_original[i].input_count;j++){
  const struct slcli_product_initial_windows_source_span *s=&genuine->rows_original[i].inputs_original[j];
  assert(!sl_win_query_source_excludes_output(genuine,s,sizeof(*s)));
  assert(!sl_win_query_source_excludes_output(genuine,s->original,s->bytes));
  assert(!sl_win_query_source_excludes_output(genuine,s->actor_original,s->actor_bytes));
  assert(!sl_win_query_source_excludes_output(genuine,s->occurrence_original,s->occurrence_bytes));
 }
 fresh->source_original=original_source;
}

/* Genuine held malformed SDK files must be supplied by the future admitted
 * acquisition harness. This constructs no held-file/principal/CLI source grant.
 * Each malformed case reaches BOTH the independent geometry helper and the
 * full source derivation with its actual retained source/output allocations. */
enum {SL_TEST_PE_IMPORT_ZERO_NAME,SL_TEST_PE_EXPORT_ZERO_NAME,SL_TEST_PE_HEADER8,
 SL_TEST_PE_HEADER_HIGH,SL_TEST_PE_RAW_OVERLAP,SL_TEST_PE_MAPPED_OVERLAP,SL_TEST_PE_CASES};
void sl_win_query_profile_genuine_pe_refusals(
 sl_win_query_derivation *const cases[SL_TEST_PE_CASES],const size_t members[SL_TEST_PE_CASES],
 unsigned char *const before[SL_TEST_PE_CASES],const size_t capacities[SL_TEST_PE_CASES]){
 for(size_t i=0;i<SL_TEST_PE_CASES;i++){
  sl_win_query_derivation *d=cases[i];assert(d&&d->source_original&&before[i]);
  const sl_win_query_semantic_row *r=&d->source_original->rows_original[SL_WIN_QUERY_IMAGE_MODULES];
  assert(members[i]<r->source.image.member_count);
  const sl_win_query_image_member *m=&r->source.image.members_original[members[i]];
  const sl_win_query_pe_closure *c=m->closure_original;assert(m->file_original&&c);
  assert(m->file_extent>=sizeof(IMAGE_DOS_HEADER)&&get32(m->file_bytes_original+offsetof(IMAGE_DOS_HEADER,e_lfanew))==m->headers_offset);
  switch(i){
  case SL_TEST_PE_IMPORT_ZERO_NAME:assert(c->import_count&&c->imports_original[0].descriptor_original->Name==0&&c->imports_original[0].descriptor_original->FirstThunk);break;
  case SL_TEST_PE_EXPORT_ZERO_NAME:assert(c->name_count&&get32((const unsigned char *)c->names_original[0].name_rva_original)==0);break;
  case SL_TEST_PE_HEADER8:assert(m->headers_offset==8&&m->headers_original->OptionalHeader.ImageBase==UINT64_C(0x800000000));break;
  case SL_TEST_PE_HEADER_HIGH:assert(m->headers_offset>=UINT32_C(0x80000000));break;
  case SL_TEST_PE_RAW_OVERLAP:
   assert(m->section_count>=2&&m->sections_original[0].SizeOfRawData&&m->sections_original[1].SizeOfRawData&&
    m->sections_original[0].PointerToRawData==m->sections_original[1].PointerToRawData);break;
  case SL_TEST_PE_MAPPED_OVERLAP:
   assert(m->section_count>=2&&m->sections_original[0].Misc.VirtualSize&&m->sections_original[1].Misc.VirtualSize&&
    m->sections_original[0].VirtualAddress==m->sections_original[1].VirtualAddress);break;
  }
  struct slcli_bq_resource_cost cost={0},failure={0},unchanged={0};
  assert(!sl_win_query_pe_source_matches(r,m,&cost,&failure));
  assert(equal_bytes(&cost,&unchanged,sizeof(cost))&&equal_bytes(&failure,&unchanged,sizeof(failure)));
  assert(d->row_capacity<=SIZE_MAX/sizeof(*d->row_copies_original)&&d->snapshot_capacity<=SIZE_MAX/sizeof(*d->snapshots_original));
  size_t row_bytes=d->row_capacity*sizeof(*d->row_copies_original),snapshot_bytes=d->snapshot_capacity*sizeof(*d->snapshots_original);
  assert(row_bytes<=SIZE_MAX-sizeof(*d)&&snapshot_bytes<=SIZE_MAX-sizeof(*d)-row_bytes);
  size_t total=sizeof(*d)+row_bytes+snapshot_bytes;
  for(size_t j=0;j<d->snapshot_capacity;j++){assert(d->snapshots_original[j].capacity<=SIZE_MAX-total);total+=d->snapshots_original[j].capacity;}
  assert(capacities[i]>=total&&sl_win_query_source_excludes_output(d->source_original,before[i],total));
  assert(separate(before[i],total,d,sizeof(*d))&&separate(before[i],total,d->row_copies_original,row_bytes)&&
   separate(before[i],total,d->snapshots_original,snapshot_bytes));
  for(size_t j=0;j<d->snapshot_capacity;j++)assert(separate(before[i],total,d->snapshots_original[j].bytes_original,d->snapshots_original[j].capacity));
  size_t at=0;clone_bytes(before[i]+at,d,sizeof(*d));at+=sizeof(*d);
  clone_bytes(before[i]+at,d->row_copies_original,row_bytes);at+=row_bytes;
  clone_bytes(before[i]+at,d->snapshots_original,snapshot_bytes);at+=snapshot_bytes;
  for(size_t j=0;j<d->snapshot_capacity;j++){clone_bytes(before[i]+at,d->snapshots_original[j].bytes_original,d->snapshots_original[j].capacity);at+=d->snapshots_original[j].capacity;}
  assert(!sl_win_derive_query_profile(d));at=0;
  assert(equal_bytes(before[i]+at,d,sizeof(*d)));at+=sizeof(*d);
  assert(equal_bytes(before[i]+at,d->row_copies_original,row_bytes));at+=row_bytes;
  assert(equal_bytes(before[i]+at,d->snapshots_original,snapshot_bytes));at+=snapshot_bytes;
  for(size_t j=0;j<d->snapshot_capacity;j++){assert(equal_bytes(before[i]+at,d->snapshots_original[j].bytes_original,d->snapshots_original[j].capacity));at+=d->snapshots_original[j].capacity;}
  /* Actual input custody and all malformed original bytes remain with the
   * genuine acquisition caller; a refusal here retires no borrower. */
 }
}

void sl_win_query_profile_genuine_source_vectors(
 sl_win_query_derivation *genuine_fresh,sl_win_query_profile_encoding *encoding,
 const uint64_t expected_dimensions[SL_WIN_QUERY_ROLE_COUNT],
 const unsigned char *expected_profile,size_t expected_profile_extent){
 assert(genuine_fresh&&encoding&&expected_dimensions&&expected_profile&&expected_profile_extent);
 assert(sl_win_derive_query_profile(genuine_fresh));
 assert(sl_win_query_derivation_matches_original(genuine_fresh));
 assert(genuine_fresh->dimensions[SL_WIN_QUERY_ENDPOINT_SECURITY]);
 assert(genuine_fresh->dimensions[SL_WIN_QUERY_PROCESS_SECURITY]);
 assert(genuine_fresh->dimensions[SL_WIN_QUERY_TOKEN_ACCESS]);
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++)assert(genuine_fresh->dimensions[i]==expected_dimensions[i]);
 /* A mutable derived quantity or a changed retained source-copy byte cannot
  * stand in for the original semantic/source relation. Only private outputs
  * are changed; the real native/source originals are untouched. */
 for(size_t i=0;i<SL_WIN_QUERY_ROLE_COUNT;i++){
  uint64_t old=genuine_fresh->dimensions[i];genuine_fresh->dimensions[i]=old==UINT64_MAX?old-1:old+1;
  assert(!sl_win_query_derivation_matches_original(genuine_fresh));
  genuine_fresh->dimensions[i]=old;assert(sl_win_query_derivation_matches_original(genuine_fresh));
 }
 for(size_t i=0;i<genuine_fresh->snapshot_count;i++){
  sl_win_query_input_snapshot *v=&genuine_fresh->snapshots_original[i];unsigned char old=v->bytes_original[0];
  v->bytes_original[0]^=1;assert(!sl_win_query_derivation_matches_original(genuine_fresh));
  v->bytes_original[0]=old;assert(sl_win_query_derivation_matches_original(genuine_fresh));
 }
 /* MAX across every principal case, never MAX costs or a single example. */
 for(size_t i=0;i<=SL_WIN_QUERY_TOKEN_ACCESS;i++){
  const sl_win_query_principal_geometry *g=&genuine_fresh->source_original->rows_original[i].source.principal;
  uint64_t maximum=0;
  for(size_t j=0;j<g->case_count;j++){
   uint64_t n=i==SL_WIN_QUERY_TOKEN_ACCESS?g->cases_original[j].access_extent:g->cases_original[j].descriptor_extent;
   if(n>maximum)maximum=n;
  }
  assert(maximum==genuine_fresh->dimensions[i]);
 }
 assert(encoding->derivation_original==genuine_fresh);
 assert(sl_win_encode_query_profile(encoding));
 /* Complete externally retained expected source vector, independent of this
  * encoder; no implementation-generated oracle or synthetic native issuer. */
 assert(encoding->output_used==expected_profile_extent);
 assert(equal_bytes(encoding->output_original,expected_profile,expected_profile_extent));
 assert(encoding->output_used&&encoding->output_original[encoding->output_used-1]=='\n');
 assert(sl_win_query_derivation_matches_original(genuine_fresh));
 for(size_t i=0;i<encoding->hash_capacity;i++){
  assert(encoding->hashes_original[i].attempted&&encoding->hashes_original[i].returned);
  assert(!encoding->hashes_original[i].result);
 }
 /* Caller retains all real inputs and output-file custody after these pure
  * vectors; a return here settles no native borrower or family. */
}
#endif
