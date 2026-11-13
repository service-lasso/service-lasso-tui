//go:build windows && amd64

package windows

import "unsafe"

// Pointer-free per-occurrence native backing. The assembly caller supplies Fn,
// N and argument words from source-owned imports/operands, never bootstrap
// bytes. This type is not a Go callback, public API or runtime ready grant.
type serviceLassoNativeCallStorage struct {
 Info StdCallInfo
 Entered uint64
 Returned uint64
 Arguments [MaxArgs]uintptr
}

// The existing direct StdCall primitive consumes precisely its existing Info
// prefix. Every call occurrence needs distinct retained storage and source-
// derived roster/stack/TLS charge before its first native borrower.
var _ [48 - unsafe.Sizeof(StdCallInfo{})]byte
var _ [unsafe.Sizeof(StdCallInfo{}) - 48]byte
var _ [400 - unsafe.Sizeof(serviceLassoNativeCallStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoNativeCallStorage{}) - 400]byte
var _ [48 - unsafe.Offsetof(serviceLassoNativeCallStorage{}.Entered)]byte
var _ [unsafe.Offsetof(serviceLassoNativeCallStorage{}.Entered) - 48]byte
var _ [56 - unsafe.Offsetof(serviceLassoNativeCallStorage{}.Returned)]byte
var _ [unsafe.Offsetof(serviceLassoNativeCallStorage{}.Returned) - 56]byte
var _ [64 - unsafe.Offsetof(serviceLassoNativeCallStorage{}.Arguments)]byte
var _ [unsafe.Offsetof(serviceLassoNativeCallStorage{}.Arguments) - 64]byte
