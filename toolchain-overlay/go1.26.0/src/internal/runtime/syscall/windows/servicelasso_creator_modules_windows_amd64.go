//go:build windows && amd64

package windows

import "unsafe"

// Snapshot acquisition only. Official API may return incorrect information
// during loader mutation and excludes LOAD_LIBRARY_AS_DATAFILE mappings.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoEnumProcessModulesEx K32EnumProcessModulesEx%5 "kernel32.dll"
var serviceLassoEnumProcessModulesEx uintptr

type serviceLassoCreatorModuleStorage struct {
 Call serviceLassoNativeCallStorage
 Process uint64
 Modules uint64
 Required uint32
 Padding [4]byte
 Count uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
var serviceLassoCreatorModulesBeforeOriginal serviceLassoCreatorModuleStorage
var serviceLassoCreatorModulesAfterOriginal serviceLassoCreatorModuleStorage
const serviceLassoCreatorModuleExtent=456
var _ [serviceLassoCreatorModuleExtent-unsafe.Sizeof(serviceLassoCreatorModuleStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorModuleStorage{})-serviceLassoCreatorModuleExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorModuleStorage{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorModuleStorage{})-8]byte
var _ [400-unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Process)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Process)-400]byte
var _ [408-unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Modules)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Modules)-408]byte
var _ [416-unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Required)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Required)-416]byte
var _ [424-unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Count)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorModuleStorage{}.Count)-424]byte
