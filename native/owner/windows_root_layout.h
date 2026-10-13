#ifndef SERVICE_LASSO_WINDOWS_ROOT_LAYOUT_H
#define SERVICE_LASSO_WINDOWS_ROOT_LAYOUT_H
#include <stdint.h>
#include <stddef.h>

/* Pointer-free source layout only. Zero storage grants no native authority. */
typedef struct sl_windows_native_root_storage {
    uint8_t layout_tag[8];
    uint64_t layout_version, source_extent;
    uint64_t endpoint, public_count, security_label_bytes;
    uint64_t terminal_calls, representation_bytes, entry_gate;
    uint8_t argument_header[48];
    uint64_t phase;
} sl_windows_native_root_storage;

#define SL_WINDOWS_NATIVE_ROOT_SOURCE_EXTENT (8u + 8u * 8u + 48u + 8u)
_Static_assert(sizeof(sl_windows_native_root_storage) == SL_WINDOWS_NATIVE_ROOT_SOURCE_EXTENT, "root source extent");
_Static_assert(_Alignof(sl_windows_native_root_storage) == 8, "root source alignment");
_Static_assert(offsetof(sl_windows_native_root_storage, endpoint) == 24, "root endpoint offset");
_Static_assert(offsetof(sl_windows_native_root_storage, entry_gate) == 64, "root gate offset");
_Static_assert(offsetof(sl_windows_native_root_storage, argument_header) == 72, "root original header offset");
_Static_assert(offsetof(sl_windows_native_root_storage, phase) == 120, "root phase offset");
#endif
