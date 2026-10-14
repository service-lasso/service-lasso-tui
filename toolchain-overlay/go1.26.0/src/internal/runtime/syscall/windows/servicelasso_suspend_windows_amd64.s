//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Private preentry ABI0 primitive only. Complete default entry supplies native
// register/SP preservation and held provider/stack/TEB/source qualification.
// It must not return to useful execution if this primitive returns. AX only
// reports refusal/unexpected return, never a successful terminal disposition.
TEXT ·serviceLassoSuspendPreentry<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoPreentrySuspensionOriginal(SB), AX
	CMPQ serviceLassoNativeCallStorage_Entered(AX), $0
	JNE suspension_refused
	CMPQ serviceLassoNativeCallStorage_Returned(AX), $0
	JNE suspension_refused
	CMPQ StdCallInfo_Fn(AX), $0
	JNE suspension_refused
	CMPQ StdCallInfo_N(AX), $0
	JNE suspension_refused
	CMPQ StdCallInfo_Args(AX), $0
	JNE suspension_refused
	CMPQ StdCallInfo_R1(AX), $0
	JNE suspension_refused
	CMPQ StdCallInfo_R2(AX), $0
	JNE suspension_refused
	CMPQ StdCallInfo_Err(AX), $0
	JNE suspension_refused
	CMPQ serviceLassoNativeCallStorage_Arguments(AX), $0
	JNE suspension_refused
	MOVQ ·serviceLassoSleep(SB), CX
	TESTQ CX, CX
	JE suspension_refused
	MOVQ CX, StdCallInfo_Fn(AX)
	MOVQ $1, StdCallInfo_N(AX)
	MOVL $0xffffffff, CX
	MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
	CALL ·serviceLassoNativeCall<ABI0>(SB)
	// Genuine Sleep(INFINITE) has no supported return. NativeCall preserves
	// unexpected raw R1/R2/TEB error and Returned in the distinct original.
suspension_refused:
	XORQ AX, AX
	RET
