//go:build windows && amd64

package windows

import "unsafe"

// Source-qualified native import only, not provenance or access authority.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoReadProcessMemory ReadProcessMemory%5 "kernel32.dll"
var serviceLassoReadProcessMemory uintptr

type serviceLassoCreatorRegionQuery struct {
 Call serviceLassoNativeCallStorage
 Information serviceLassoMemoryBasicInformation
 Process uint64
 Address uint64
 RegionEnd uint64
 ModuleBase uint64
 ImageEnd uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorRegionQueryExtent=512
var _ [serviceLassoCreatorRegionQueryExtent-unsafe.Sizeof(serviceLassoCreatorRegionQuery{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorRegionQuery{})-serviceLassoCreatorRegionQueryExtent]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorRegionQuery{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorRegionQuery{})-8]byte
var _ [400-unsafe.Offsetof(serviceLassoCreatorRegionQuery{}.Information)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorRegionQuery{}.Information)-400]byte

type serviceLassoCreatorMemoryCall struct {
 Info StdCallInfo
 Entered uint64
 Returned uint64
 Arguments [5]uintptr
 Count uint64
 Offset uint64
 Requested uint64
}
const serviceLassoCreatorMemoryCallExtent=128
var _ [serviceLassoCreatorMemoryCallExtent-unsafe.Sizeof(serviceLassoCreatorMemoryCall{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorMemoryCall{})-serviceLassoCreatorMemoryCallExtent]byte
var _ [48-unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Entered)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Entered)-48]byte
var _ [56-unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Returned)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Returned)-56]byte
var _ [64-unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Arguments)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Arguments)-64]byte
var _ [104-unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Count)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Count)-104]byte
var _ [112-unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Offset)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Offset)-112]byte
var _ [120-unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Requested)]byte
var _ [unsafe.Offsetof(serviceLassoCreatorMemoryCall{}.Requested)-120]byte
var _ [8-unsafe.Alignof(serviceLassoCreatorMemoryCall{})]byte
var _ [unsafe.Alignof(serviceLassoCreatorMemoryCall{})-8]byte

type serviceLassoCreatorRegionStorage struct {
 QueryIndex uint64
 ReadIndex uint64
 Process uint64
 Address uint64
 Extent uint64
 ArenaOffset uint64
 ModuleBase uint64
 ImageEnd uint64
 Disposition uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
type serviceLassoCreatorWalkStorage struct {
 ExtentOwner uint64
 Phase uint64
 ModuleIndex uint64
 Process uint64
 Base uint64
 ImageEnd uint64
 AddressCursor uint64
 FirstRegion uint64
 Used uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
const serviceLassoCreatorRegionExtent=96
const serviceLassoCreatorWalkExtent=96
var _ [serviceLassoCreatorRegionExtent-unsafe.Sizeof(serviceLassoCreatorRegionStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorRegionStorage{})-serviceLassoCreatorRegionExtent]byte
var _ [serviceLassoCreatorWalkExtent-unsafe.Sizeof(serviceLassoCreatorWalkStorage{})]byte
var _ [unsafe.Sizeof(serviceLassoCreatorWalkStorage{})-serviceLassoCreatorWalkExtent]byte
// Source-owned monotone cursors select only preowned static occurrences. They
// never finance storage or establish loader/own-writer/currentness authority.
var serviceLassoCreatorRegionUsed uint64
var serviceLassoCreatorMemoryArenaUsed uint64
