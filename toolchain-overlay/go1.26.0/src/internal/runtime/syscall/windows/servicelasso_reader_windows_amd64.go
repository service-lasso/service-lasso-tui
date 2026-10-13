//go:build windows && amd64

package windows

import "unsafe"

//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoReadFile ReadFile%5 "kernel32.dll"
var serviceLassoReadFile uintptr

// Pointer-free original operation storage. Native numeric argument addresses
// are never Go heap references; the target is the separately typed static
// header original. Actual entry/loaded-provider/finance/D dominate invocation.
type serviceLassoHeaderReadCall struct {
 Call serviceLassoNativeCallStorage
 Count uint32
 Padding [4]byte
 Offset uint64
 Requested uint64
}
type serviceLassoHeaderReadStorage struct {
 Calls [48]serviceLassoHeaderReadCall
 Endpoint uint64
 Completed uint64
 Used uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
var serviceLassoHeaderReadOriginal serviceLassoHeaderReadStorage
const serviceLassoHeaderReadCallExtent=424
const serviceLassoHeaderReadExtent=48*serviceLassoHeaderReadCallExtent+48
var _ [serviceLassoHeaderReadCallExtent-unsafe.Sizeof(serviceLassoHeaderReadCall{})]byte
var _ [unsafe.Sizeof(serviceLassoHeaderReadCall{})-serviceLassoHeaderReadCallExtent]byte
var _ [serviceLassoHeaderReadExtent-unsafe.Sizeof(serviceLassoHeaderReadOriginal)]byte
var _ [unsafe.Sizeof(serviceLassoHeaderReadOriginal)-serviceLassoHeaderReadExtent]byte
var _ [400-unsafe.Offsetof(serviceLassoHeaderReadCall{}.Count)]byte
var _ [unsafe.Offsetof(serviceLassoHeaderReadCall{}.Count)-400]byte
var _ [408-unsafe.Offsetof(serviceLassoHeaderReadCall{}.Offset)]byte
var _ [unsafe.Offsetof(serviceLassoHeaderReadCall{}.Offset)-408]byte
var _ [416-unsafe.Offsetof(serviceLassoHeaderReadCall{}.Requested)]byte
var _ [unsafe.Offsetof(serviceLassoHeaderReadCall{}.Requested)-416]byte
var _ [8-unsafe.Alignof(serviceLassoHeaderReadOriginal)]byte
var _ [unsafe.Alignof(serviceLassoHeaderReadOriginal)-8]byte

// Representation operands only; neither returned bytes nor matching source
// header authenticates a creator, endpoint, native provider or containing fit.
type serviceLassoHeaderInterpretStorage struct {
 RawUnits uint64
 ModuleUnits uint64
 ExpectedCount uint64
 BodyExtent uint64
 Flags uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
var serviceLassoHeaderInterpretOriginal serviceLassoHeaderInterpretStorage
const serviceLassoHeaderInterpretExtent=64
var _ [serviceLassoHeaderInterpretExtent-unsafe.Sizeof(serviceLassoHeaderInterpretOriginal)]byte
var _ [unsafe.Sizeof(serviceLassoHeaderInterpretOriginal)-serviceLassoHeaderInterpretExtent]byte

// Dedicated compact ReadFile record, not serviceLassoNativeCallStorage. The
// dedicated native helper consumes exactly five arguments through StdCallInfo.
type serviceLassoBodyReadCall struct {
 Info StdCallInfo
 Entered uint64
 Returned uint64
 Arguments [5]uintptr
 Count uint32
 Padding [4]byte
 Offset uint64
 Requested uint64
}
const serviceLassoBodyReadCallExtent=128
var _ [serviceLassoBodyReadCallExtent-unsafe.Sizeof(serviceLassoBodyReadCall{})]byte
var _ [unsafe.Sizeof(serviceLassoBodyReadCall{})-serviceLassoBodyReadCallExtent]byte
var _ [48-unsafe.Offsetof(serviceLassoBodyReadCall{}.Entered)]byte
var _ [unsafe.Offsetof(serviceLassoBodyReadCall{}.Entered)-48]byte
var _ [56-unsafe.Offsetof(serviceLassoBodyReadCall{}.Returned)]byte
var _ [unsafe.Offsetof(serviceLassoBodyReadCall{}.Returned)-56]byte
var _ [64-unsafe.Offsetof(serviceLassoBodyReadCall{}.Arguments)]byte
var _ [unsafe.Offsetof(serviceLassoBodyReadCall{}.Arguments)-64]byte
var _ [104-unsafe.Offsetof(serviceLassoBodyReadCall{}.Count)]byte
var _ [unsafe.Offsetof(serviceLassoBodyReadCall{}.Count)-104]byte
var _ [112-unsafe.Offsetof(serviceLassoBodyReadCall{}.Offset)]byte
var _ [unsafe.Offsetof(serviceLassoBodyReadCall{}.Offset)-112]byte
var _ [120-unsafe.Offsetof(serviceLassoBodyReadCall{}.Requested)]byte
var _ [unsafe.Offsetof(serviceLassoBodyReadCall{}.Requested)-120]byte
var _ [8-unsafe.Alignof(serviceLassoBodyReadCall{})]byte
var _ [unsafe.Alignof(serviceLassoBodyReadCall{})-8]byte
var _ [40-unsafe.Sizeof(serviceLassoBodyReadCall{}.Arguments)]byte
var _ [unsafe.Sizeof(serviceLassoBodyReadCall{}.Arguments)-40]byte
var _ [0-unsafe.Offsetof(StdCallInfo{}.Fn)]byte
var _ [unsafe.Offsetof(StdCallInfo{}.Fn)-0]byte
var _ [8-unsafe.Offsetof(StdCallInfo{}.N)]byte
var _ [unsafe.Offsetof(StdCallInfo{}.N)-8]byte
var _ [16-unsafe.Offsetof(StdCallInfo{}.Args)]byte
var _ [unsafe.Offsetof(StdCallInfo{}.Args)-16]byte
var _ [24-unsafe.Offsetof(StdCallInfo{}.R1)]byte
var _ [unsafe.Offsetof(StdCallInfo{}.R1)-24]byte
var _ [32-unsafe.Offsetof(StdCallInfo{}.R2)]byte
var _ [unsafe.Offsetof(StdCallInfo{}.R2)-32]byte
var _ [40-unsafe.Offsetof(StdCallInfo{}.Err)]byte
var _ [unsafe.Offsetof(StdCallInfo{}.Err)-40]byte
type serviceLassoBodyReadStorage struct {
 Endpoint uint64
 Completed uint64
 Used uint64
 Attempted uint64
 Returned uint64
 Failure uint64
}
var serviceLassoBodyReadOriginal serviceLassoBodyReadStorage
const serviceLassoBodyReadExtent=48
var _ [serviceLassoBodyReadExtent-unsafe.Sizeof(serviceLassoBodyReadOriginal)]byte
var _ [unsafe.Sizeof(serviceLassoBodyReadOriginal)-serviceLassoBodyReadExtent]byte
