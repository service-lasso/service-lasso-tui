//go:build windows && amd64

package windows

import "unsafe"

// Source-owned loader imports. Their actual provider/forwarder/IAT/loaded-code
// and writer lifetimes belong to complete input/native qualification. Neither
// a nonzero slot nor an API name authenticates the provider.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoPipeServerProcess GetNamedPipeServerProcessId%2 "kernel32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoPipeInformation GetNamedPipeInfo%5 "kernel32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoOpenProcess OpenProcess%3 "kernel32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoProcessTimes GetProcessTimes%5 "kernel32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoWaitForSingleObject WaitForSingleObject%2 "kernel32.dll"
var serviceLassoPipeServerProcess uintptr
var serviceLassoPipeInformation uintptr
var serviceLassoOpenProcess uintptr
var serviceLassoProcessTimes uintptr
var serviceLassoWaitForSingleObject uintptr

type serviceLassoEndpointNativeStorage struct {
 ServerCall serviceLassoNativeCallStorage
 InformationCall serviceLassoNativeCallStorage
 Endpoint uint64
 ServerProcessID uint32
 Flags uint32
 OutboundBytes uint32
 InboundBytes uint32
 MaximumInstances uint32
 Attempted uint64
 Returned uint64
}

// Separate pointer-free original backing, never written by the owner root
// transport descriptor. No init/function runs merely because it is declared.
var serviceLassoEndpointOriginal serviceLassoEndpointNativeStorage

type serviceLassoCreatorNativeStorage struct {
 OpenCall serviceLassoNativeCallStorage
 TimesCall serviceLassoNativeCallStorage
 LifeCall serviceLassoNativeCallStorage
 ServerProcessID uint64
 Process uint64
 CreationTime uint64
 ExitTime uint64
 KernelTime uint64
 UserTime uint64
 Attempted uint64
 Returned uint64
}

// A successful read-only open owns this actual native handle occurrence until
// the complete caller's lifetime settlement. No failure closes or relabels it.
var serviceLassoCreatorOriginal serviceLassoCreatorNativeStorage

// Native TOKEN_STATISTICS field order, separately typed from any Go token
// wrapper. ModifiedId is an observation for the later before/access/after join.
// https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_statistics
type serviceLassoTokenStatistics struct {
 TokenID uint64
 AuthenticationID uint64
 ExpirationTime int64
 TokenType uint32
 ImpersonationLevel uint32
 DynamicCharged uint32
 DynamicAvailable uint32
 GroupCount uint32
 PrivilegeCount uint32
 ModifiedID uint64
}
var _ [56-unsafe.Sizeof(serviceLassoTokenStatistics{})]byte
var _ [unsafe.Sizeof(serviceLassoTokenStatistics{})-56]byte

//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoOpenProcessToken OpenProcessToken%3 "advapi32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoGetTokenInformation GetTokenInformation%5 "advapi32.dll"
var serviceLassoOpenProcessToken uintptr
var serviceLassoGetTokenInformation uintptr
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoGetKernelObjectSecurity GetKernelObjectSecurity%5 "advapi32.dll"
var serviceLassoGetKernelObjectSecurity uintptr

// Native SDK TOKEN_USER_MAX_SIZE, with independent trailing Go alignment.
// SID is a numeric native output view, never a Go pointer or trusted locator.
type serviceLassoTokenUserBody struct {
 SID uint64
 Attributes uint32
 Padding uint32
 SIDBytes [8+15*4]byte
}
const serviceLassoTokenUserCapacity = 16+8+15*4
var _ [88-unsafe.Sizeof(serviceLassoTokenUserBody{})]byte
var _ [unsafe.Sizeof(serviceLassoTokenUserBody{})-88]byte
var _ [16-unsafe.Offsetof(serviceLassoTokenUserBody{}.SIDBytes)]byte
var _ [unsafe.Offsetof(serviceLassoTokenUserBody{}.SIDBytes)-16]byte

type serviceLassoTokenNativeStorage struct {
 OpenCall serviceLassoNativeCallStorage
 BeforeCall serviceLassoNativeCallStorage
 UserCall serviceLassoNativeCallStorage
 Process uint64
 Token uint64
 Before serviceLassoTokenStatistics
 BeforeLength uint32
 User serviceLassoTokenUserBody
 UserLength uint32
 Attempted uint64
 Returned uint64
}
var serviceLassoTokenOriginal serviceLassoTokenNativeStorage

type serviceLassoSecuritySizingStorage struct {
 EndpointCall serviceLassoNativeCallStorage
 ProcessCall serviceLassoNativeCallStorage
 TokenCall serviceLassoNativeCallStorage
 Endpoint uint64
 Process uint64
 Token uint64
 EndpointRequired uint32
 ProcessRequired uint32
 TokenRequired uint32
 Attempted uint64
 Returned uint64
}
// Length outputs remain observations. Later fill needs an independently
// financed source-owned arena; these lengths never authorize native allocation.
var serviceLassoSecuritySizingOriginal serviceLassoSecuritySizingStorage

type serviceLassoSecurityFillStorage struct {
 EndpointCall serviceLassoNativeCallStorage
 ProcessCall serviceLassoNativeCallStorage
 TokenCall serviceLassoNativeCallStorage
 AfterCall serviceLassoNativeCallStorage
 EndpointBuffer uint64
 ProcessBuffer uint64
 TokenBuffer uint64
 EndpointCapacity uint64
 ProcessCapacity uint64
 TokenCapacity uint64
 EndpointRequired uint32
 ProcessRequired uint32
 TokenRequired uint32
 AfterLength uint32
 After serviceLassoTokenStatistics
 Attempted uint64
 Returned uint64
}
// Fill is a native effect primitive. Its complete caller must establish the
// containing reserve for these exact generated regions before entering it.
// Returned observations cannot establish that reserve retroactively.
var serviceLassoSecurityFillOriginal serviceLassoSecurityFillStorage

type serviceLassoTokenUserInterpretationStorage struct {
 NativeOriginal uint64
 InputBase uint64
 InputExtent uint64
 RawSID uint64
 SID uint64
 SIDExtent uint64
 Failure uint64
 Attempted uint64
 Returned uint64
}
// Private bounded preTLS interpretation, separately observed by the later
// independent reader. A valid SID representation is never an identity grant.
var serviceLassoTokenUserInterpretationOriginal serviceLassoTokenUserInterpretationStorage

type serviceLassoEndpointInterpretationStorage struct {
 NativeOriginal uint64
 UserOriginal uint64
 InputBase uint64
 InputExtent uint64
 OwnerSID uint64
 OwnerExtent uint64
 GroupSID uint64
 GroupExtent uint64
 DACL uint64
 DACLExtent uint64
 ACESID uint64
 ACESIDExtent uint64
 Failure uint64
 Attempted uint64
 Returned uint64
}
var serviceLassoEndpointInterpretationOriginal serviceLassoEndpointInterpretationStorage

// A stable bracket is an observation about these exact retained calls only.
// It neither authenticates the process nor interprets TokenAccessInformation.
type serviceLassoTokenBracketStorage struct {
 BeforeOriginal uint64
 AfterOriginal uint64
 Token uint64
 Failure uint64
 Attempted uint64
 Returned uint64
}
var serviceLassoTokenBracketOriginal serviceLassoTokenBracketStorage

// Distinct native current-child subjects. These genuine observations are
// joined to the owner's birth/image/Job originals by the complete caller;
// PID/TID equality alone is never an entry grant.
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoCurrentProcess GetCurrentProcess%0 "kernel32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoCurrentProcessID GetCurrentProcessId%0 "kernel32.dll"
//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoCurrentThreadID GetCurrentThreadId%0 "kernel32.dll"
var serviceLassoCurrentProcess uintptr
var serviceLassoCurrentProcessID uintptr
var serviceLassoCurrentThreadID uintptr
type serviceLassoChildNativeStorage struct {
 ProcessCall serviceLassoNativeCallStorage
 ProcessIDCall serviceLassoNativeCallStorage
 ThreadIDCall serviceLassoNativeCallStorage
 TimesCall serviceLassoNativeCallStorage
 Process uint64
 ProcessID uint64
 ThreadID uint64
 CreationTime uint64
 ExitTime uint64
 KernelTime uint64
 UserTime uint64
 Attempted uint64
 Returned uint64
}
var serviceLassoChildOriginal serviceLassoChildNativeStorage
