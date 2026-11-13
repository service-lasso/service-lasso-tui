//go:build windows && amd64

package windows

import "unsafe"

// Caller-owned distinct native row. Pointer-free numeric native views only;
// complete traversal/containing reservation and all original lifetimes remain
// the owning factory/caller's duties before any invocation.
type serviceLassoCreatorNativeRelocation struct {
 PE uint64
 LoadedBase uint64
 DirectoryRVA uint64
 DirectoryBytes uint64
 BlockOffset uint64
 BlockBytes uint64
 NextBlock uint64
 EntryIndex uint64
 EntryCount uint64
 PageRVA uint64
 RawEntry uint64
 Type uint64
 TargetRVA uint64
 Original uint64
 Expected uint64
 OriginalKind uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorNativeRelocationExtent=152
var _ [serviceLassoCreatorNativeRelocationExtent-unsafe.Sizeof(serviceLassoCreatorNativeRelocation{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorNativeRelocation{})-serviceLassoCreatorNativeRelocationExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorNativeRelocation{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorNativeRelocation{})-8]byte
var _ [128-unsafe.Offsetof(serviceLassoCreatorNativeRelocation{}.Attempted)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeRelocation{}.Attempted)-128]byte
var _ [144-unsafe.Offsetof(serviceLassoCreatorNativeRelocation{}.Failure)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNativeRelocation{}.Failure)-144]byte
