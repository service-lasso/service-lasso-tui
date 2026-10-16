#ifndef SERVICE_LASSO_QUERY_PROFILE_WINDOWS_H
#define SERVICE_LASSO_QUERY_PROFILE_WINDOWS_H
#ifdef _WIN32
/* The selected build supplies the actual held CLI source header/closure.
 * These are real borrowed types, never duplicated authority declarations. */
#ifndef SERVICE_LASSO_PRODUCT_SOURCE_HEADER
#error exact source-qualified CLI product source header required
#endif
#include SERVICE_LASSO_PRODUCT_SOURCE_HEADER
#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include "owned_sha256.h"
#include "owner_windows.h"
struct slcli_build_principal_windows;
struct slcli_build_held_file_windows;
struct sl_win_query_pe_closure;

enum sl_win_query_role {
 SL_WIN_QUERY_ENDPOINT_SECURITY,SL_WIN_QUERY_PROCESS_SECURITY,
 SL_WIN_QUERY_TOKEN_ACCESS,SL_WIN_QUERY_ARGUMENT_MODULE,
 SL_WIN_QUERY_WRITER_STACK,SL_WIN_QUERY_WRITER_TURNS,
 SL_WIN_QUERY_IMAGE_MODULES,SL_WIN_QUERY_IMAGE_NAME,SL_WIN_QUERY_IMAGE_FILE,
 SL_WIN_QUERY_IMAGE_MEMORY,SL_WIN_QUERY_IMAGE_REGIONS,SL_WIN_QUERY_ROLE_COUNT
};
enum sl_win_query_arm {SL_WIN_QUERY_PRINCIPAL,SL_WIN_QUERY_IMAGE,SL_WIN_QUERY_WRITER};
enum sl_win_query_mapping_kind {SL_WIN_QUERY_MAPPING_IMAGE,SL_WIN_QUERY_MAPPING_DATA_FILE};
enum sl_win_query_writer_step {SL_WIN_QUERY_SEND,SL_WIN_QUERY_OBSERVE,SL_WIN_QUERY_CANCEL,SL_WIN_QUERY_SETTLE};

/* Complete finite source-owned inputs. The real enclosing CLI constructor
 * supplies designation, source/native membership, finance and call control.
 * A pointer/span/count or successful pure checker supplies none of those. */
typedef struct sl_win_query_sid_source {
 const SID *original;size_t bytes;
 const struct slcli_product_initial_windows_source_span *source_original;
} sl_win_query_sid_source;
typedef struct sl_win_query_hash_source {
 const SID_AND_ATTRIBUTES_HASH *original;
 const SID_AND_ATTRIBUTES *attributes_original;size_t count;
 const sl_win_query_sid_source *sids_original;
} sl_win_query_hash_source;
typedef struct sl_win_query_claim_value_source {
 const unsigned char *original;size_t bytes;
 const struct slcli_product_initial_windows_source_span *source_original;
} sl_win_query_claim_value_source;
typedef struct sl_win_query_claim_source {
 const CLAIM_SECURITY_ATTRIBUTE_RELATIVE_V1 *original;
 const WCHAR *name_original;size_t name_units;
 const sl_win_query_claim_value_source *values_original;size_t value_count;
} sl_win_query_claim_source;
typedef struct sl_win_query_ace_source {
 const ACE_HEADER *original;size_t bytes;
 const struct slcli_product_initial_windows_source_span *source_original;
 const ACCESS_MASK *mask_original;
 const DWORD *object_flags_original;
 const GUID *object_type_original,*inherited_object_type_original;
 sl_win_query_sid_source sid;
 const unsigned char *payload_original;size_t payload_extent;
 const struct slcli_product_initial_windows_source_span *payload_source;
 sl_win_query_claim_source resource_attribute;
} sl_win_query_ace_source;
typedef struct sl_win_query_principal_case {
 const struct slcli_product_initial_windows_source_span *source_original;
 uint32_t scenario,slot; /* Exact closed birth.security[6]/tokens[2]/endpoint occurrence. */
 const struct slcli_build_principal_windows *effective_original,*primary_original;
 const SECURITY_DESCRIPTOR_RELATIVE *descriptor_original;
 const unsigned char *descriptor_bytes_original;size_t descriptor_extent;
 sl_win_query_sid_source owner_sid,group_sid;
 const ACL *dacl_original;
 const sl_win_query_ace_source *aces_original;size_t ace_count;
 const TOKEN_ACCESS_INFORMATION *access_original;
 const unsigned char *access_bytes_original;size_t access_extent;
 sl_win_query_hash_source sid_hash,restricted_hash,capabilities_hash;
 const TOKEN_PRIVILEGES *privileges_original;size_t privilege_count;
 sl_win_query_sid_source package_sid,trust_sid;
 const struct slcli_product_initial_windows_source_span *native_layout_source;
 struct slcli_bq_resource_cost cost,failed_overlap_cost;
} sl_win_query_principal_case;
typedef struct sl_win_query_principal_geometry {
 const sl_win_query_principal_case *cases_original;size_t case_count;
} sl_win_query_principal_geometry;

typedef struct sl_win_query_image_member {
 const struct slcli_build_held_file_windows *file_original;
 const unsigned char *file_bytes_original;size_t file_extent;
 const IMAGE_NT_HEADERS64 *headers_original;size_t headers_offset;
 const IMAGE_SECTION_HEADER *sections_original;size_t section_count,sections_offset;
 const WCHAR *module_name_original,*mapped_name_original;
 size_t module_name_units,mapped_name_units;
 const struct slcli_product_initial_windows_source_span *source_original;
 const struct sl_win_query_pe_closure *closure_original;
} sl_win_query_image_member;
typedef struct sl_win_query_mapping_source {
 const sl_win_query_image_member *member_original;
 const MEMORY_BASIC_INFORMATION *geometry_original;
 uint32_t kind,occurrence;
 const struct slcli_product_initial_windows_source_span *source_original;
} sl_win_query_mapping_source;
typedef struct sl_win_query_image_geometry {
 const sl_win_query_image_member *members_original;size_t member_count;
 const sl_win_query_mapping_source *mappings_original;size_t mapping_count;
 const struct slcli_product_initial_windows_source_span *loader_writer_original;
 size_t loader_writer_count;
 const struct slcli_product_initial_windows_source_span *native_layout_source;
} sl_win_query_image_geometry;

typedef struct sl_win_query_writer_occurrence {
 uint32_t kind,ordinal,turn;
 const struct slcli_product_initial_windows_source_span *source_original;
} sl_win_query_writer_occurrence;
typedef struct sl_win_query_writer_schedule {
 const struct slcli_product_initial_windows_source_span *worker_entry_source;
 const struct slcli_product_initial_windows_source_span *helper_closure_source,*tls_source;
 const sl_win_query_writer_occurrence *steps_original;size_t step_count;
 uint64_t abi_shadow_bytes,alignment_bytes,fixed_frame_bytes;
 uint64_t maximum_nested_helper_bytes,native_reservation_granule;
 const struct slcli_product_initial_windows_source_span *native_layout_source;
} sl_win_query_writer_schedule;

typedef struct sl_win_query_semantic_row {
 uint32_t role,arm,actor_index,phase,command,occurrence;
 const struct slcli_product_initial_windows_source_span *source_original;
 const struct slcli_product_initial_windows_source_span *inputs_original;
 size_t input_count;
 union {
  sl_win_query_principal_geometry principal;
  sl_win_query_image_geometry image;
  sl_win_query_writer_schedule writer;
 } source;
 struct slcli_bq_resource_cost cost,failed_overlap_cost;
} sl_win_query_semantic_row;

enum {SL_WIN_QUERY_ASSOCIATION_COUNT=6};
typedef struct sl_win_query_association_bytes {
 const unsigned char *original;size_t extent;
 const struct slcli_product_initial_windows_source_span *source_original;
} sl_win_query_association_bytes;
typedef struct sl_win_query_profile_source {
 const struct slcli_product_storage_windows_source *initial_source_original;
 const sl_win_query_semantic_row *rows_original;size_t row_count;
 /* Complete constructor-owned canonical source byte encodings, not raw C
  * structs/addresses or supplied digest authority. They stay private. */
 sl_win_query_association_bytes provider_original;
 sl_win_query_association_bytes associations[SL_WIN_QUERY_ASSOCIATION_COUNT];
 const unsigned char *source_commit_original;size_t source_commit_extent;
} sl_win_query_profile_source;

typedef struct sl_win_query_input_snapshot {
 const struct slcli_product_initial_windows_source_span *source_original;
 struct slcli_product_initial_windows_source_span source_snapshot;
 unsigned char *bytes_original;size_t capacity;
 unsigned part; /* body0, actor1, occurrence2; all three retained independently. */
} sl_win_query_input_snapshot;
typedef struct sl_win_query_derivation {
 const sl_win_query_profile_source *source_original;
 sl_win_query_profile_source source_snapshot;
 sl_win_query_semantic_row *row_copies_original;size_t row_capacity;
 sl_win_query_input_snapshot *snapshots_original;size_t snapshot_capacity;
 sl_win_query_semantic_row *actual_row_copies;
 sl_win_query_input_snapshot *actual_snapshots;
 uint64_t dimensions[SL_WIN_QUERY_ROLE_COUNT];
 struct slcli_bq_resource_cost simultaneous,failed_overlap;
 size_t snapshot_count,failed_row;
 unsigned attempted,returned;int result;
} sl_win_query_derivation;

/* Pure representation only. No native query, allocation, clock, entry, family
 * settlement, source authentication or reservation is performed/issued. */
int sl_win_derive_query_profile(sl_win_query_derivation *);
int sl_win_query_derivation_matches_original(const sl_win_query_derivation *);
int sl_win_query_source_excludes_output(const sl_win_query_profile_source *,const void *,size_t);
/* Structural ACE source geometry only, never native policy/access admission. */
int sl_win_query_ace_source_matches(const sl_win_query_semantic_row *,const sl_win_query_ace_source *);

typedef struct sl_win_query_profile_encoding {
 const sl_win_query_derivation *derivation_original;
 unsigned char *output_original;size_t output_capacity,output_used;
 sl_owned_sha256 *hashes_original;sl_owned_sha256_workspace *workspaces_original;
 size_t hash_capacity;
 unsigned attempted,returned;int result;
} sl_win_query_profile_encoding;
/* Writes only existing closed schema3 association bytes into preowned backing.
 * Caller must revalidate/retain the actual nativecase and exclusive file through
 * every borrower. Encoding success grants no nativecase/provider authority. */
int sl_win_encode_query_profile(sl_win_query_profile_encoding *);
#endif
#endif
