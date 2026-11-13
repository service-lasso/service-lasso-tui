//go:build windows && amd64

package os

import (
 lower "internal/runtime/syscall/windows"
 "syscall"
)

// Source-qualified initialized-Go primitive, currently without an activation
// callsite. SyscallN -> runtime.syscall_syscalln -> cgocall/entersyscall moves
// the G into genuine syscall state and uses the native/system stack. It does
// not leave an indefinitely running G outside GC/scheduler safepoints.
//
// The complete os.init caller may activate this only under the supported held
// Sleep provider/nonreturn relation and full retained startup/outer-observer
// custody. If this function returns, no supported suspension was established:
// no private Args return, useful success, retry, spin or exit is authorized.
func serviceLassoAttemptTerminalSuspension() {
 function,argument,prepared:=lower.ServiceLassoBeginInitializedSuspension()
 if !prepared{return}
 if !lower.ServiceLassoMarkInitializedSyscallAttempt(){return}
 r1,r2,errorNumber:=syscall.SyscallN(function,argument)
 lower.ServiceLassoRetainUnexpectedSuspensionReturn(r1,r2,uintptr(errorNumber))
}
