#ifndef SERVICE_LASSO_QUERY_PROFILE_PE_WINDOWS_H
#define SERVICE_LASSO_QUERY_PROFILE_PE_WINDOWS_H
#ifdef _WIN32
#include "query_profile_windows.h"
/* Semantic source projections of the existing366 ImportAt/DelayImportAt/
 * TLSAt/Export{Function,Name}At/RelocationAt rules. These are NOT redeclarations
 * of private Go objects, a native C/Go ABI, or a source/native grant. Actual
 * constructor provenance and complete source/case designation stay external. */
typedef struct sl_win_query_pe_row_source {
 const struct slcli_product_initial_windows_source_span *source_original;
 const struct slcli_product_initial_windows_source_span *interpretation_original;
 struct slcli_bq_resource_cost cost,failed_overlap_cost;
} sl_win_query_pe_row_source;
typedef struct sl_win_query_pe_import {
 sl_win_query_pe_row_source source;
 uint64_t descriptor_index,thunk_index;
 const IMAGE_IMPORT_DESCRIPTOR *descriptor_original;
 const ULONGLONG *lookup_original;
 uint64_t iat_rva;
 const WORD *hint_original;
 const unsigned char *module_original,*name_original;
 size_t module_bytes,name_bytes;
} sl_win_query_pe_import;
typedef struct sl_win_query_pe_delay {
 sl_win_query_pe_row_source source;
 uint64_t descriptor_index,thunk_index;
 const IMAGE_DELAYLOAD_DESCRIPTOR *descriptor_original;
 const ULONGLONG *lookup_original;
 uint64_t module_rva,iat_rva,bound_rva,unload_rva;
 const WORD *hint_original;
 const unsigned char *module_original,*name_original;
 size_t module_bytes,name_bytes;
} sl_win_query_pe_delay;
typedef struct sl_win_query_pe_tls {
 sl_win_query_pe_row_source source;
 uint64_t callback_index;
 const IMAGE_TLS_DIRECTORY64 *directory_original;
 const unsigned char *template_original;size_t template_bytes;
 uint64_t index_rva;unsigned index_kind;
 const ULONGLONG *callback_original;
} sl_win_query_pe_tls;
typedef struct sl_win_query_pe_export {
 sl_win_query_pe_row_source source;
 uint64_t selected_index,function_index;unsigned named;
 const IMAGE_EXPORT_DIRECTORY *directory_original;
 const DWORD *function_original,*name_rva_original;
 const WORD *ordinal_original;
 const unsigned char *name_original,*forwarder_original;
 size_t name_bytes,forwarder_bytes;
} sl_win_query_pe_export;
typedef struct sl_win_query_pe_relocation {
 sl_win_query_pe_row_source source;
 uint64_t block_offset,entry_index;
 const IMAGE_BASE_RELOCATION *block_original;
 const WORD *entry_original;
 const unsigned char *target_original;unsigned target_kind;
} sl_win_query_pe_relocation;
typedef struct sl_win_query_pe_closure {
 const sl_win_query_image_member *member_original;
 const struct slcli_product_initial_windows_source_span *source_original;
 const sl_win_query_pe_import *imports_original;size_t import_count;
 const sl_win_query_pe_delay *delays_original;size_t delay_count;
 const sl_win_query_pe_tls *tls_original;size_t tls_count;
 const sl_win_query_pe_export *functions_original,*names_original;
 size_t function_count,name_count;
 const sl_win_query_pe_relocation *relocations_original;size_t relocation_count;
} sl_win_query_pe_closure;
/* Fixed readonly row/file correspondences and complete terminal traversal.
 * Does not construct source provenance, provider/loader/writer or finance. */
int sl_win_query_pe_source_matches(const sl_win_query_semantic_row *,const sl_win_query_image_member *,
 struct slcli_bq_resource_cost *,struct slcli_bq_resource_cost *);
#endif
#endif
