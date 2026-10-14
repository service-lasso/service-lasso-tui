//go:build windows && amd64

package windows

import "unsafe"

// Actual loaded-provider/forwarder/IAT/source ownership remains a complete
// caller prerequisite. Native returned device spelling is a locator only.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoGetMappedFileName K32GetMappedFileNameW%4 "kernel32.dll"
var serviceLassoGetMappedFileName uintptr

type serviceLassoCreatorNameStorage struct {
 Call serviceLassoNativeCallStorage
 Process uint64
 Address uint64
 Buffer uint64
 Capacity uint32
 Count uint32
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorNameExtent=456
var _ [serviceLassoCreatorNameExtent-unsafe.Sizeof(serviceLassoCreatorNameStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorNameStorage{})-serviceLassoCreatorNameExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorNameStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorNameStorage{})-8]byte
var _ [424-unsafe.Offsetof(serviceLassoCreatorNameStorage{}.Capacity)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNameStorage{}.Capacity)-424]byte
var _ [428-unsafe.Offsetof(serviceLassoCreatorNameStorage{}.Count)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorNameStorage{}.Count)-428]byte
