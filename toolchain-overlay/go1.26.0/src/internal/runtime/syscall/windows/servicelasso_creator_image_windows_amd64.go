//go:build windows && amd64

package windows

import "unsafe"

// Actual provider/forwarder/IAT/own-writer qualification is a separate caller
// obligation. A named/nonzero import cannot authenticate any loaded image.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoVirtualQueryEx VirtualQueryEx%4 "kernel32.dll"
var serviceLassoVirtualQueryEx uintptr

// Each generated before/after module row owns one distinct fixed original.
// Geometry is not a module roster or binding-wide R admission.
// Numeric addresses/handles are native views; no hidden Go reference headers.
type serviceLassoCreatorImageMappingStorage struct {
 Call serviceLassoNativeCallStorage
 Information serviceLassoMemoryBasicInformation
 Process uint64
 Address uint64
 RegionEnd uint64
 Failure uint64
 Attempted uint64
 Returned uint64
}
const serviceLassoCreatorImageMappingExtent=496
var _ [serviceLassoCreatorImageMappingExtent-unsafe.Sizeof(serviceLassoCreatorImageMappingStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorImageMappingStorage{})-serviceLassoCreatorImageMappingExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorImageMappingStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorImageMappingStorage{})-8]byte
var _ [400-unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.Information)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.Information)-400]byte
var _ [448-unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.Process)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.Process)-448]byte
var _ [456-unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.Address)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.Address)-456]byte
var _ [464-unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.RegionEnd)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorImageMappingStorage{}.RegionEnd)-464]byte
