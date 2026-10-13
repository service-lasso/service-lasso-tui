//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Private assembly-only register contract: AX points to an independently
// source-owned fresh serviceLassoNativeCallStorage. No compiler Go wrapper,
// Go G/TLS load, scheduler, Go allocation, lazy provider or callback is entered.
// Existing StdCall genuinely uses native GS/TEB for LastError; that native
// thread state and its read/write lifetime remain part of the caller's proof.
// AX==1 means only that this genuine native call returned; it says nothing
// about API success, authentication, bootstrap entry or custody disposition.
TEXT ·serviceLassoNativeCall<ABI0>(SB),NOSPLIT|NOFRAME,$0
	TESTQ AX, AX
	JE refused
	CMPQ serviceLassoNativeCallStorage_Entered(AX), $0
	JNE refused
	CMPQ serviceLassoNativeCallStorage_Returned(AX), $0
	JNE refused
	CMPQ StdCallInfo_Fn(AX), $0
	JE refused
	CMPQ StdCallInfo_Args(AX), $0
	JNE refused
	CMPQ StdCallInfo_R1(AX), $0
	JNE refused
	CMPQ StdCallInfo_R2(AX), $0
	JNE refused
	CMPQ StdCallInfo_Err(AX), $0
	JNE refused
	CMPQ StdCallInfo_N(AX), $const_MaxArgs
	JA refused
	LEAQ serviceLassoNativeCallStorage_Arguments(AX), CX
	MOVQ CX, StdCallInfo_Args(AX)
	MOVQ $1, serviceLassoNativeCallStorage_Entered(AX)
	SUBQ $16, SP
	MOVQ AX, 0(SP)
	CALL ·StdCall<ABIInternal>(SB)
	MOVQ 0(SP), AX
	ADDQ $16, SP
	MOVQ $1, serviceLassoNativeCallStorage_Returned(AX)
	MOVQ $1, AX
	RET
refused:
	XORQ AX, AX
	RET

// Strong root relocation once reached from the complete genuine entry closure.
// This address helper alone does not make the default entry reachable or ready.
TEXT ·serviceLassoNativeRootAddress<ABI0>(SB),NOSPLIT|NOFRAME,$0
	LEAQ ·serviceLassoNativeRoot(SB), AX
	RET

// Win64 BOOL is the low signed32 result, independent of unused high RAX bits.
// AX is a copied raw result; the SAME native call's raw R1 is never changed.
// All source BOOL consumers use this exact assembly projection. This private
// CALL/RET adds a real stack borrower to the complete entry/fit accounting.
TEXT ·serviceLassoNativeBOOL<ABI0>(SB),NOSPLIT|NOFRAME,$0
	TESTL AX, AX
	SETNE AL
	MOVBLZX AL, AX
	RET

// Test-only Go ABI0 bridge, unreachable in the production default-entry graph.
// Tests call the SAME assembly projection used by actual native consumers.
TEXT ·serviceLassoNativeBOOLForTest(SB),NOSPLIT|NOFRAME,$0-9
	MOVQ raw+0(FP), AX
	CALL ·serviceLassoNativeBOOL<ABI0>(SB)
	MOVB AX, ret+8(FP)
	RET
