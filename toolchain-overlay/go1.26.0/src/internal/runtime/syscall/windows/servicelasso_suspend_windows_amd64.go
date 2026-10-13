//go:build windows && amd64

package windows

//go:cgo_import_dynamic internal/runtime/syscall/windows.serviceLassoSleep Sleep%1 "kernel32.dll"
var serviceLassoSleep uintptr

// Separate preentry and initialized-Go native occurrences. No new closeable
// handle/event, preliminary wait, retry, timeout renewal or recovery is added.
var serviceLassoPreentrySuspensionOriginal serviceLassoNativeCallStorage
type serviceLassoInitializedSuspensionStorage struct {
 Function uintptr
 Argument uintptr
 R1 uintptr
 R2 uintptr
 Error uintptr
 Prepared uint64
 SyscallNAttempted uint64
 Returned uint64
 Failure uint64
}
var serviceLassoInitializedSuspensionOriginal serviceLassoInitializedSuspensionStorage

// Ordinary initialized Go only. The os caller uses syscall.SyscallN, whose
// genuine runtime cgocall enters syscall state before native suspension. This
// function neither calls direct StdCall nor claims native suspension occurred.
func ServiceLassoBeginInitializedSuspension()(uintptr,uintptr,bool) {
 o:=&serviceLassoInitializedSuspensionOriginal
 if o.Function!=0||o.Argument!=0||o.R1!=0||o.R2!=0||o.Error!=0||o.Prepared!=0||o.SyscallNAttempted!=0||o.Returned!=0||o.Failure!=0{return 0,0,false}
 o.Function=serviceLassoSleep;o.Argument=uintptr(^uint32(0))
 if o.Function==0 {o.Failure=1;return 0,0,false}
 o.Prepared=1
 return o.Function,o.Argument,true
}
// Source caller records its SyscallN attempt immediately before the expression.
// This is NOT actual Sleep entry: intervening runtime/transition failure remains
// unknown until an independently retained low-level native-entry observation.
func ServiceLassoMarkInitializedSyscallAttempt() bool {
 o:=&serviceLassoInitializedSuspensionOriginal
 if o.Prepared!=1||o.SyscallNAttempted!=0||o.Returned!=0||o.Failure!=0{return false}
 o.SyscallNAttempted=1;return true
}
// Sleep is VOID. An actual return is an unsupported provider-contract event;
// retain raw registers/error without projecting any of them as success/BOOL.
// No safe live continuation or permission to return private Args follows.
func ServiceLassoRetainUnexpectedSuspensionReturn(r1,r2,errorNumber uintptr) {
 o:=&serviceLassoInitializedSuspensionOriginal
 if o.Prepared!=1||o.SyscallNAttempted!=1||o.Returned!=0{return}
 o.R1=r1;o.R2=r2;o.Error=errorNumber;o.Returned=1;o.Failure=2
}
