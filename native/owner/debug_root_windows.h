#ifndef SERVICE_LASSO_DEBUG_ROOT_WINDOWS_H
#define SERVICE_LASSO_DEBUG_ROOT_WINDOWS_H
#ifdef _WIN32
#include "owner_windows.h"
#include "windows_root_layout.h"

/* One source-owned native attempt. Failed/unknown/foreign events stay original;
 * no event handle is closed or reused, and no exception is auto-continued. */
typedef struct sl_win_debug_event_observation {
 sl_win_birth *birth_original,*actual_birth;
 sl_win_call thread_query,held_thread_query,held_thread_wait,wait;
 DWORD actual_thread_id,held_thread_id,timeout_ms,actual_timeout_ms;
 DEBUG_EVENT event;
 int result;
 unsigned attempted,returned,create_event_matches_birth;
} sl_win_debug_event_observation;

/* Acquisition only. A matching creation event does not authenticate mapped
 * bytes, root writer provenance, entry readiness, resume or disposition. */
int sl_win_observe_debug_event(sl_win_debug_event_observation *original);

typedef struct sl_win_mapped_root_observation {
 sl_win_debug_event_observation *event_original,*actual_event;
 sl_win_call region_query,read;
 ULONG_PTR mapped_base,root_address;
 MEMORY_BASIC_INFORMATION region;
 SIZE_T region_returned,bytes_read;
 unsigned char bytes[SL_WINDOWS_NATIVE_ROOT_SOURCE_EXTENT];
 int result;
 unsigned attempted,returned;
} sl_win_mapped_root_observation;
/* Read-only actual mapped root acquisition. Before-write/readback occurrences
 * must be separately preowned; no successful read grants write/authentication. */
int sl_win_observe_mapped_root(sl_win_mapped_root_observation *original);

typedef struct sl_win_root_write {
 sl_win_mapped_root_observation *before_original,*actual_before;
 const sl_windows_native_root_storage *source_original,*actual_source;
 sl_win_call write;
 SIZE_T bytes_written;
 sl_win_mapped_root_observation readback;
 int result;
 unsigned attempted,returned;
} sl_win_root_write;
/* Exact bounded effect primitive. The connected caller must first qualify the
 * complete source/provider/image/own-writer conjunction; this helper is not an
 * issuer. Partial write/readback stays retained and does not permit Continue. */
int sl_win_write_exact_root(sl_win_root_write *original);

typedef struct sl_win_debug_release {
 sl_win_root_write *write_original,*actual_write;
 sl_win_call current_thread_query,held_thread_query,held_thread_wait;
 sl_win_call continue_event,stop_debug,resume_primary;
 DWORD current_thread_id,held_thread_id,previous_suspend_count;
 HANDLE independent_handles[3],event_handles[3];
 unsigned attempted,returned,event_handles_unknown;
 int result;
} sl_win_debug_release;
/* Ordered native effects only after the complete connected caller's genuine
 * entry/router/provider/source predicates. Event handles remain UNKNOWN after
 * Stop; independent PI handles are the only Resume subjects. No handle close,
 * forced termination, failure retry, custody retirement or FINAL is issued. */
int sl_win_release_debug_root(sl_win_debug_release *original);
#endif
#endif
