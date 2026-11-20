//go:build linux && cgo

package coreentry

/*
#cgo CFLAGS: -I${SRCDIR}/../../native/entry
#include <stdlib.h>
#include "sl_core_initial_entry.h"
#include "sl_core_initial_entry.c"
*/
import "C"

import "unsafe"

// AcquireNative passes the sealed views to the private Core entry contract.
// The contract still returns RETAINED_UNAVAILABLE until the Core launch record
// supplies authentic originals; this bridge never invents them.
func (e *Entry) AcquireNative() NativeResult {
	if e == nil || e.Image == nil || len(e.Image.Views) == 0 || len(e.Image.Views) > 11 || len(e.Image.Base) == 0 {
		return NativeRefused
	}
	count := len(e.Image.Views)
	size := C.size_t(count) * C.size_t(unsafe.Sizeof(C.struct_sl_core_initial_entry_view{}))
	views := (*C.struct_sl_core_initial_entry_view)(C.malloc(size))
	if views == nil {
		return NativeRefused
	}
	defer C.free(unsafe.Pointer(views))
	viewSlice := unsafe.Slice(views, count)
	for index, view := range e.Image.Views {
		viewSlice[index].kind = C.uint32_t(view.Kind)
		viewSlice[index].flags = C.uint32_t(view.Flags)
		viewSlice[index].offset = C.uint64_t(view.Offset)
		viewSlice[index].length = C.uint64_t(view.Length)
		viewSlice[index].alignment = C.uint64_t(view.Alignment)
		viewSlice[index].item_count = C.uint64_t(view.ItemCount)
	}
	source := C.struct_sl_core_initial_entry_source{
		image_base:  (*C.uint8_t)(unsafe.Pointer(&e.Image.Base[0])),
		image_bytes: C.uint64_t(len(e.Image.Base)),
		views:       views,
		view_count:  C.uint32_t(count),
		binder:      nil,
		originals:   nil,
	}
	var entry C.struct_sl_core_initial_entry
	return NativeResult(C.sl_core_initial_entry_acquire(&entry, &source))
}
