//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// Actual snapshot phase R10/row R11. MODULEINFO is an observation of loaded
// headers, not independent PE validation or loaded entry/provider authority.
TEXT ·serviceLassoObserveCreatorModuleExtent<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_extent_before
 CMPQ R10, $1
 JNE creator_extent_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorMappingsAfter(SB), R8
 LEAQ ·serviceLassoCreatorExtentsAfterOriginal(SB), BX
 JMP creator_extent_row
creator_extent_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorMappingsBefore(SB), R8
 LEAQ ·serviceLassoCreatorExtentsBeforeOriginal(SB), BX
creator_extent_row:
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_extent_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_extent_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_extent_refused
 CMPQ R11, CX
 JAE creator_extent_refused
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorImageMappingExtent, AX
 JO creator_extent_refused
 ADDQ AX, R8
 JC creator_extent_refused
 CMPQ serviceLassoCreatorImageMappingStorage_Returned(R8), $1
 JNE creator_extent_refused
 CMPQ serviceLassoCreatorImageMappingStorage_Failure(R8), $0
 JNE creator_extent_refused
 MOVQ serviceLassoCreatorModuleStorage_Process(DX), CX
 CMPQ CX, serviceLassoCreatorImageMappingStorage_Process(R8)
 JNE creator_extent_refused
 MOVQ R8, R10
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorExtentBytes, AX
 JO creator_extent_refused
 ADDQ AX, BX
 JC creator_extent_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorExtentBytes, AX
creator_extent_fresh:
 CMPQ 0(DX), $0
 JNE creator_extent_refused
 ADDQ $8, DX
 SUBQ $8, AX
 JNE creator_extent_fresh
 MOVQ $1, serviceLassoCreatorExtentStorage_Attempted(BX)
 MOVQ serviceLassoCreatorImageMappingStorage_Process(R10), CX
 MOVQ CX, serviceLassoCreatorExtentStorage_Process(BX)
 MOVQ serviceLassoCreatorImageMappingStorage_Address(R10), DX
 MOVQ DX, serviceLassoCreatorExtentStorage_Address(BX)
 LEAQ serviceLassoCreatorExtentStorage_Call(BX), AX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
 MOVQ DX, serviceLassoNativeCallStorage_Arguments+8(AX)
 LEAQ serviceLassoCreatorExtentStorage_Information(BX), CX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
 MOVQ $24, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ ·serviceLassoGetModuleInformation(SB), CX
 MOVQ CX, StdCallInfo_Fn(AX)
 MOVQ $4, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_extent_failed
 MOVQ serviceLassoCreatorExtentStorage_Call+StdCallInfo_R1(BX), AX
 CALL ·serviceLassoNativeBOOL<ABI0>(SB)
 TESTQ AX, AX
 JE creator_extent_failed
 LEAQ serviceLassoCreatorExtentStorage_Information(BX), DX
 MOVQ serviceLassoModuleInformation_Base(DX), CX
 TESTQ CX, CX
 JE creator_extent_failed
 CMPQ CX, serviceLassoCreatorExtentStorage_Address(BX)
 JNE creator_extent_failed
 MOVL serviceLassoModuleInformation_SizeOfImage(DX), R8
 TESTQ R8, R8
 JE creator_extent_failed
 ADDQ CX, R8
 JC creator_extent_failed
 MOVQ R8, serviceLassoCreatorExtentStorage_ImageEnd(BX)
 MOVQ serviceLassoModuleInformation_Entry(DX), R9
 TESTQ R9, R9
 JE creator_extent_valid
 CMPQ R9, CX
 JB creator_extent_failed
 CMPQ R9, R8
 JAE creator_extent_failed
creator_extent_valid:
 // Zero entry remains observable for images with no entry; actual image-role
 // and held-PE consumer must establish its own exact supported correspondence.
 MOVQ $1, serviceLassoCreatorExtentStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_extent_failed:
 MOVQ $1, serviceLassoCreatorExtentStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorExtentStorage_Returned(BX)
creator_extent_refused:
 XORQ AX, AX
 RET
