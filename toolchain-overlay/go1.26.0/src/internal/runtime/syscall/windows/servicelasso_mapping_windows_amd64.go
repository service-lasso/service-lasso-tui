//go:build windows && amd64

package windows

import "unsafe"

//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoVirtualQuery VirtualQuery%3 "kernel32.dll"
var serviceLassoVirtualQuery uintptr

// Native numeric views only, never hidden Go pointers. SDK26100 amd64
// MEMORY_BASIC_INFORMATION layout; retain native padding as original bytes.
type serviceLassoMemoryBasicInformation struct {
 BaseAddress uint64
 AllocationBase uint64
 AllocationProtect uint32
 PartitionID uint16
 Padding0 [2]byte
 RegionSize uint64
 State uint32
 Protect uint32
 Type uint32
 Padding1 [4]byte
}
var _ [48-unsafe.Sizeof(serviceLassoMemoryBasicInformation{})]byte
var _ [unsafe.Sizeof(serviceLassoMemoryBasicInformation{})-48]byte
var _ [8-unsafe.Alignof(serviceLassoMemoryBasicInformation{})]byte
var _ [unsafe.Alignof(serviceLassoMemoryBasicInformation{})-8]byte
var _ [24-unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.RegionSize)]byte
var _ [unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.RegionSize)-24]byte
var _ [32-unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.State)]byte
var _ [unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.State)-32]byte
var _ [36-unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.Protect)]byte
var _ [unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.Protect)-36]byte
var _ [40-unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.Type)]byte
var _ [unsafe.Offsetof(serviceLassoMemoryBasicInformation{}.Type)-40]byte

type serviceLassoRootMappingStorage struct {
 Call serviceLassoNativeCallStorage
 Information serviceLassoMemoryBasicInformation
 Address uint64
 RegionEnd uint64
 Failure uint64
 Attempted uint64
 Returned uint64
}
var serviceLassoRootMappingOriginal serviceLassoRootMappingStorage
const serviceLassoRootMappingExtent=488
var _ [serviceLassoRootMappingExtent-unsafe.Sizeof(serviceLassoRootMappingOriginal)]byte
var _ [unsafe.Sizeof(serviceLassoRootMappingOriginal)-serviceLassoRootMappingExtent]byte
