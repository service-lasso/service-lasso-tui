//go:build windows && amd64

#include "go_asm.h"
#include "textflag.h"

// R10 selects before0/after1; R11 is the exact actual snapshot row index.
// This primitive does not authenticate the snapshot or loaded bytes.
// Fixed output cannot overwrite the retained creator original. Outer native
// entry preserves BX/SI/DI/BP/SP and charges wrapper/native stack and backing.
TEXT ·serviceLassoObserveCreatorImageMapping<ABI0>(SB),NOSPLIT|NOFRAME,$0
 CMPQ R10, $0
 JE creator_image_before
 CMPQ R10, $1
 JNE creator_image_refused
 LEAQ ·serviceLassoCreatorModulesAfterOriginal(SB), DX
 LEAQ ·serviceLassoCreatorModulesAfter(SB), R9
 LEAQ ·serviceLassoCreatorMappingsAfter(SB), BX
 JMP creator_image_row
creator_image_before:
 LEAQ ·serviceLassoCreatorModulesBeforeOriginal(SB), DX
 LEAQ ·serviceLassoCreatorModulesBefore(SB), R9
 LEAQ ·serviceLassoCreatorMappingsBefore(SB), BX
creator_image_row:
 CMPQ serviceLassoCreatorModuleStorage_Attempted(DX), $1
 JNE creator_image_refused
 CMPQ serviceLassoCreatorModuleStorage_Returned(DX), $1
 JNE creator_image_refused
 CMPQ serviceLassoCreatorModuleStorage_Failure(DX), $0
 JNE creator_image_refused
 CMPQ R9, serviceLassoCreatorModuleStorage_Modules(DX)
 JNE creator_image_refused
 MOVQ serviceLassoCreatorModuleStorage_Count(DX), CX
 MOVL $const_serviceLassoCreatorModuleCapacity, AX
 CMPQ CX, AX
 JA creator_image_refused
 CMPQ R11, CX
 JAE creator_image_refused
 LEAQ ·serviceLassoCreatorOriginal(SB), AX
 MOVQ serviceLassoCreatorNativeStorage_Process(AX), R8
 CMPQ R8, serviceLassoCreatorModuleStorage_Process(DX)
 JNE creator_image_refused
 MOVQ (R9)(R11*8), R10
 // Positive native DWORD roster bounds this product well within signed64.
 MOVQ R11, AX
 IMULQ $const_serviceLassoCreatorImageMappingExtent, AX
 JO creator_image_refused
 ADDQ AX, BX
 JC creator_image_refused
 MOVQ BX, DX
 MOVQ $const_serviceLassoCreatorImageMappingExtent, R8
creator_image_fresh:
 CMPQ 0(DX), $0
 JNE creator_image_refused
 ADDQ $8, DX
 SUBQ $8, R8
 JNE creator_image_fresh
 MOVQ $1, serviceLassoCreatorImageMappingStorage_Attempted(BX)
 MOVQ R10, serviceLassoCreatorImageMappingStorage_Address(BX)
 TESTQ R10, R10
 JE creator_image_failed
 LEAQ ·serviceLassoCreatorOriginal(SB), DX
 CMPQ serviceLassoCreatorNativeStorage_Attempted(DX), $1
 JNE creator_image_failed
 CMPQ serviceLassoCreatorNativeStorage_Returned(DX), $1
 JNE creator_image_failed
 CMPQ serviceLassoCreatorNativeStorage_OpenCall+serviceLassoNativeCallStorage_Returned(DX), $1
 JNE creator_image_failed
 MOVQ serviceLassoCreatorNativeStorage_Process(DX), CX
 MOVQ CX, serviceLassoCreatorImageMappingStorage_Process(BX)
 TESTQ CX, CX
 JE creator_image_failed
 CMPQ CX, $-1
 JE creator_image_failed
 CMPQ CX, serviceLassoCreatorNativeStorage_OpenCall+StdCallInfo_R1(DX)
 JNE creator_image_failed
 CMPQ serviceLassoCreatorNativeStorage_TimesCall+serviceLassoNativeCallStorage_Returned(DX), $1
 JNE creator_image_failed
 CMPL serviceLassoCreatorNativeStorage_TimesCall+StdCallInfo_R1(DX), $0
 JE creator_image_failed
 CMPQ serviceLassoCreatorNativeStorage_LifeCall+serviceLassoNativeCallStorage_Returned(DX), $1
 JNE creator_image_failed
 CMPL serviceLassoCreatorNativeStorage_LifeCall+StdCallInfo_R1(DX), $258
 JNE creator_image_failed
 // The prior zero-wait is an observation, not a creator lifetime lease.
 LEAQ serviceLassoCreatorImageMappingStorage_Call(BX), AX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments(AX)
 MOVQ serviceLassoCreatorImageMappingStorage_Address(BX), CX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments+8(AX)
 LEAQ serviceLassoCreatorImageMappingStorage_Information(BX), CX
 MOVQ CX, serviceLassoNativeCallStorage_Arguments+16(AX)
 MOVQ $48, serviceLassoNativeCallStorage_Arguments+24(AX)
 MOVQ ·serviceLassoVirtualQueryEx(SB), CX
 MOVQ CX, StdCallInfo_Fn(AX)
 MOVQ $4, StdCallInfo_N(AX)
 CALL ·serviceLassoNativeCall<ABI0>(SB)
 CMPQ AX, $1
 JNE creator_image_failed
 // SIZE_T is full64; raw error remains independent even on nonzero result.
 CMPQ serviceLassoCreatorImageMappingStorage_Call+StdCallInfo_R1(BX), $48
 JNE creator_image_failed
 LEAQ serviceLassoCreatorImageMappingStorage_Information(BX), DX
 CMPL serviceLassoMemoryBasicInformation_State(DX), $0x1000
 JNE creator_image_failed
 CMPL serviceLassoMemoryBasicInformation_Type(DX), $0x1000000
 JNE creator_image_failed
 MOVQ serviceLassoMemoryBasicInformation_AllocationBase(DX), CX
 TESTQ CX, CX
 JE creator_image_failed
 MOVQ serviceLassoMemoryBasicInformation_BaseAddress(DX), R8
 CMPQ CX, R8
 JA creator_image_failed
 CMPQ R8, serviceLassoCreatorImageMappingStorage_Address(BX)
 JA creator_image_failed
 MOVQ serviceLassoMemoryBasicInformation_RegionSize(DX), R9
 TESTQ R9, R9
 JE creator_image_failed
 ADDQ R8, R9
 JC creator_image_failed
 MOVQ R9, serviceLassoCreatorImageMappingStorage_RegionEnd(BX)
 CMPQ serviceLassoCreatorImageMappingStorage_Address(BX), R9
 JAE creator_image_failed
 // MEM_IMAGE can include modified COW pages. No protection mask, loaded-byte
 // equality, provider/writer exclusion or wire-read grant follows this result.
 MOVQ $1, serviceLassoCreatorImageMappingStorage_Returned(BX)
 MOVQ $1, AX
 RET
creator_image_failed:
 MOVQ $1, serviceLassoCreatorImageMappingStorage_Failure(BX)
 MOVQ $1, serviceLassoCreatorImageMappingStorage_Returned(BX)
creator_image_refused:
 XORQ AX, AX
 RET
