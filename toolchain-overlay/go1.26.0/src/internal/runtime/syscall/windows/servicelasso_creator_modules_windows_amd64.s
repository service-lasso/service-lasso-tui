//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// R10=0 selects before, R10=1 after. Distinct source-owned originals/arrays;
// this selection is occurrence ownership only, not loader-stability authority.
TEXT ·serviceLassoObserveCreatorModules<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_modules_before
 CMPQ R10, $1
 JNE creator_modules_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), BX
 LEAQ ·serviceLassoCreatorModulesAfter(SB), R9
 JMP creator_modules_preflight
creator_modules_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), BX
 LEAQ ·serviceLassoCreatorModulesBefore(SB), R9
creator_modules_preflight:
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorModuleExtent, R8
creator_modules_owner_fresh:
 CMPQ 0(DX), $0
 JNE creator_modules_refused
 ADDQ $8, DX
 SUBQ $8, R8
 JNE creator_modules_owner_fresh
 MOVQ R9, DX
 MOVL $const_serviceLassoCreatorModuleBytes, R8
creator_modules_array_fresh:
 CMPQ 0(DX), $0
 JNE creator_modules_refused
 ADDQ $8, DX
 SUBQ $8, R8
 JNE creator_modules_array_fresh
 MOVQ $1, serviceLassoCreatorModuleStorage_Attempted(BX)
 MOVQ R9, serviceLassoCreatorModuleStorage_Modules(BX)
 LEAQ ·serviceLassoCreatorOriginal(SB), DX
 CMPQ serviceLassoCreatorNativeStorage_Attempted(DX), $1
 JNE creator_modules_failed
 CMPQ serviceLassoCreatorNativeStorage_Returned(DX), $1
 JNE creator_modules_failed
 CMPQ serviceLassoCreatorNativeStorage_OpenCall+serviceLassoNativeCallStorage_Returned(DX), $1
 JNE creator_modules_failed
 MOVQ serviceLassoCreatorNativeStorage_Process(DX), CX
 MOVQ CX, serviceLassoCreatorModuleStorage_Process(BX)
 TESTQ CX, CX
 JE creator_modules_failed
 CMPQ CX, $-1
 JE creator_modules_failed
 CMPQ CX, serviceLassoCreatorNativeStorage_OpenCall+StdCallInfo_R1(DX)
 JNE creator_modules_failed
 LEAQ serviceLassoCreatorModuleStorage_Call(BX), AX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
 MOVQ R9, serviceLassoNativeCallStorage_Arguments+8(AX)
 // Native byte operand is DWORD; MOVL zero-extends its full unsigned domain.
 MOVL $const_serviceLassoCreatorModuleBytes, CX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
 LEAQ serviceLassoCreatorModuleStorage_Required(BX), CX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ $3, serviceLassoNativeCallStorage_Arguments+32(AX)
 MOVQ ·serviceLassoEnumProcessModulesEx(SB), CX
 MOVQ CX, StdCallInfo_Fn(AX)
 MOVQ $5, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_modules_failed
 MOVQ serviceLassoCreatorModuleStorage_Call+StdCallInfo_R1(BX), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 TESTQ AX, AX
 JE creator_modules_failed
 MOVL serviceLassoCreatorModuleStorage_Required(BX), CX
 TESTQ CX, CX
 JE creator_modules_failed
 MOVL $const_serviceLassoCreatorModuleBytes, DX
 CMPQ CX, DX
 JA creator_modules_failed
 TESTQ $7, CX
 JNE creator_modules_failed
 SHRQ $3, CX
 MOVQ CX, serviceLassoCreatorModuleStorage_Count(BX)
 MOVQ $1, serviceLassoCreatorModuleStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_modules_failed:
 MOVQ $1, serviceLassoCreatorModuleStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorModuleStorage_Returned(BX)
creator_modules_refused:
 XORQ AX, AX
 RET
